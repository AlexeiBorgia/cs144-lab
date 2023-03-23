#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include<iostream>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    ,timer()
    , _initial_retransmission_timeout{retx_timeout}
    ,rto(_initial_retransmission_timeout)
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return sent_index-acked_index; }

void TCPSender::fill_window() {
	if(closed!=0)return;
	if((sent_index)==0){//send the syn
		//cout<<"SYN: "<<rto<<endl;
		TCPSegment seg;
		seg.header().seqno=wrap(sent_index,_isn);
		seg.header().syn=1;
		seg.header().fin=0;
		sent_index=1;
		_segments_out.push(seg);
		_pending_segs.push_back(seg);
		if(timer.running()==false){
			timer.start(rto);
		}
		return ;
	}
	size_t remains=window_size-(sent_index-acked_index);//outlet for user
	if(remains==0)return;
	size_t toread=_stream.buffer_size()+_stream.input_ended();
	if(toread==0)return;
	if(toread>remains||_stream.input_ended()==false){
		toread=(toread<remains)?toread:remains;
		size_t segsize=TCPConfig::MAX_PAYLOAD_SIZE;
		size_t num=toread/segsize;  //
		size_t remain=toread%segsize;//
		for(size_t i=0;i<num;i++){
			TCPSegment seg;
			string output=_stream.read(segsize);
			seg.header().seqno=wrap(sent_index,_isn);
			seg.header().syn=0;
			seg.header().fin=0;
			sent_index+=segsize;
			Buffer out(std::move(output));
			seg.payload()=out;
			_segments_out.push(seg);
			_pending_segs.push_back(seg);
		}
		if(remain>0){
			TCPSegment seg;
			string output=_stream.read(remain);
			seg.header().seqno=wrap(sent_index,_isn);
			seg.header().syn=0;
			seg.header().fin=0;
			sent_index+=remain;
			Buffer out(std::move(output));
			seg.payload()=out;
			_segments_out.push(seg);
			_pending_segs.push_back(seg);
		}
	
	}
	else{//encounter eof
		size_t rsize=_stream.buffer_size();
		closed=1;
		if(rsize==0){
			TCPSegment seg;
			seg.header().seqno=wrap(sent_index,_isn);
			seg.header().syn=0;
			seg.header().fin=1;
			sent_index+=1;
			_segments_out.push(seg);
			_pending_segs.push_back(seg);
		}else{
			size_t segsize=TCPConfig::MAX_PAYLOAD_SIZE;
			size_t num=rsize/segsize;  
			size_t remain=rsize%segsize;
			size_t Num=(remain==0)?num-1:num;
			for(size_t i=0;i<Num;i++){
				TCPSegment seg;
				string output=_stream.read(segsize);
				seg.header().seqno=wrap(sent_index,_isn);
				seg.header().syn=0;
				seg.header().fin=0;
				sent_index+=segsize;
				Buffer out(std::move(output));
				seg.payload()=out;
				_segments_out.push(seg);
				_pending_segs.push_back(seg);
			}
			if(remain>0){
				TCPSegment seg;
				string output=_stream.read(remain);
				seg.header().seqno=wrap(sent_index,_isn);
				seg.header().syn=0;
				seg.header().fin=1;
				sent_index+=(remain+1);
				Buffer out(std::move(output));
				seg.payload()=out;
				_segments_out.push(seg);
				_pending_segs.push_back(seg);
			}else{//append fin to a full segment
				TCPSegment seg;
				string output=_stream.read(segsize);
				seg.header().seqno=wrap(sent_index,_isn);
				seg.header().syn=0;
				seg.header().fin=1;
				sent_index+=(segsize+1);
				Buffer out(std::move(output));
				seg.payload()=out;
				_segments_out.push(seg);
				_pending_segs.push_back(seg);
			}
		}
	}
	if(timer.running()==false){
		timer.start(rto);
	}
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t _window_size) {
	if(closed==2)return;
	size_t acked=unwrap(ackno,_isn,acked_index);
	if(acked>sent_index)return;
	if(_window_size+acked>end_index){
		end_index=_window_size+acked;
	}
	if(acked<=acked_index){//not free space
		window_size=end_index-acked_index;
	}
	if(acked>acked_index){//free some pending space,new message acked
		strive=0;
		while(true){//update acked_index
	 		TCPSegment v=_pending_segs.front();
	 		int vsize=v.length_in_sequence_space();
	 		if(acked_index+vsize<=acked){
	 			_pending_segs.pop_front();
	 			acked_index+=vsize;
	 			if(acked_index==sent_index)break;
	 		}else{
	 			break;
	 		}
	 	}
	 	window_size=end_index-acked_index;
	 	if(closed==1&&acked_index==sent_index){
	 		timer.stop();
	 		closed=2;//our fin is received
	 		return;
	 	}
	 	timer.stop();
	 	rto=_initial_retransmission_timeout;
		cnt=0;
		if(sent_index>acked_index){//pending datas
			timer.start(rto);
		}
	}
	if(window_size==0&&closed==0){//strive for output space,otherwise its completely closed
		end_index++;
		window_size++;
		strive=1;
	}
	fill_window();
	
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
		//cout<<"tick now: "<<ms_since_last_tick<<endl;
		//cout<<"rto: "<<rto<<endl;
	if(timer.tick(ms_since_last_tick)==true){
		//cout<<"tick now: "<<ms_since_last_tick<<endl;
		TCPSegment v=_pending_segs.front();
		_segments_out.push(v);
		if(strive==0){
			rto*=2;
			cnt++;
		}
		timer.start(rto);
	}
}

unsigned int TCPSender::consecutive_retransmissions() const { return cnt; }

void TCPSender::send_empty_segment() {
	TCPSegment v;
	v.header().seqno=wrap(sent_index,_isn);
	v.header().syn=0;
	v.header().fin=0;
	_segments_out.push(v);
}
