#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;
constexpr int debug=0;
size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { 
	return _sender.bytes_in_flight();
 }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { 
return _since_last_received; 
}

void TCPConnection::segment_received(const TCPSegment &seg) {
	if(active()==false&&_sender.next_seqno_absolute()>0)return;
	if(debug==1){
		cout<<"cesare debug: seg received\n";
		cout<<"syn: "<<(seg.header().syn==true)<<endl;
		cout<<"seqno: "<<seg.header().seqno<<endl;
		cout<<"ack: "<<(seg.header().ack==true)<<endl;
		if(seg.header().ack==true){
			cout<<"ackno: "<<seg.header().ackno<<endl;
		}
	}
	_since_last_received=0;
	if(seg.header().rst==true){//receive abortion
	
		_sender.stream_in().set_error();
		_receiver.stream_out().set_error();
		return;
	}
	
	
	//for receiver
	_receiver.segment_received(seg);
	//try to recged as server
	if(_receiver.stream_out().input_ended()==true){//inbound receive fin
		if(_sender.stream_in().input_ended()==false){//we havent sent fin
			_linger_after_streams_finish=false;
		}
	}
	
	//for sender
	if(seg.header().ack==true){//update sender's sending window
		_sender.ack_received(seg.header().ackno,seg.header().win);
		
	}else{//no ack feedback from peer
		if(_sender.next_seqno_absolute()==0){
			connect();//try to connect
			return;
		}
	}
	//if nothing for sender to send and receiver need to be replied
	if(_receiver.ackno().has_value()&&_sender.segments_out().empty()&&seg.length_in_sequence_space()){
		_sender.send_empty_segment();
	}
		pop_out();
	
}

bool TCPConnection::stream_active() const { 
	bool r=(_receiver.ackno().has_value()&&_receiver.stream_out().input_ended()==false&&_receiver.stream_out().error()==false);
	bool w=true;
	if(_sender.next_seqno_absolute()==0)w=false;
	else if(_sender.stream_in().eof()==true&&_sender.next_seqno_absolute()==_sender.stream_in().bytes_written()+2&&_sender.bytes_in_flight()==0)w=false;//cpl
	else if(_sender.stream_in().error()==true)w=false;
	return r||w;

 }
bool TCPConnection::active() const{
	if(_receiver.stream_out().error()&&_sender.stream_in().error())return false;//rst
	if(stream_active()==true)return true;
	if(_linger_after_streams_finish==true&&lingertimeout==false)return true;
	return false;
}
size_t TCPConnection::write(const string &data) {
	if(active()==false&&_sender.next_seqno_absolute()>0)return 0;
   	ByteStream& outbound=_sender.stream_in();
   	if(outbound.error()==true)return 0;
   	size_t ret=outbound.write(data);
   	_sender.fill_window();
   	pop_out();
   	return ret;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
	if(active()==false&&_sender.next_seqno_absolute()>0)return;
	_since_last_received+=ms_since_last_tick;
	if(stream_active()==false){//lingering
		if(_since_last_received>=10*_cfg.rt_timeout)lingertimeout=true;
		return;
	}
	_sender.tick(ms_since_last_tick);//retransmission
	if(_sender.cnt>TCPConfig::MAX_RETX_ATTEMPTS){
		abortion();
	}
	pop_out();
}
void TCPConnection::abortion(){//sent out rst message
	while(!_sender._segments_out.empty()){
		_sender._segments_out.pop();
	}
	_sender.send_empty_segment();
	TCPSegment seg=_sender._segments_out.front();
	_sender._segments_out.pop();
	seg.header().rst=true;
	seg.header().ack=true;
	seg.header().ackno=_receiver.ackno().value();
	_segments_out.push(seg);//send now
	_sender.stream_in().set_error();
	_receiver.stream_out().set_error();
	return;
}
void TCPConnection::end_input_stream() {
	_sender.stream_in().end_input();
	_sender.fill_window();
	pop_out();
}
void TCPConnection::pop_out(){//check the out_put queue,pop out pending segment   ctest --output-on-failure -V -R 't_active_close' 
				//sudo tshark -Pw /tmp/debug.raw -i tun144
	while(!_sender.segments_out().empty()){    
		TCPSegment seg=_sender.segments_out().front();
		_sender.segments_out().pop();
		//now fill the ack and win_size
		if(_receiver.ackno().has_value()==true){
			seg.header().ack=true;
			seg.header().ackno=_receiver.ackno().value();
		}else{
			seg.header().ack=false;
		}
		seg.header().win=_receiver.window_size();
		if(debug){
			cout<<"packet send: \n";
			cout<<"seqno: "<<seg.header().seqno<<endl;
			cout<<"syn: "<<(seg.header().syn==true)<<endl;
			cout<<"ack: "<<(seg.header().ack==true)<<endl;
			if(seg.header().ack){
				cout<<"ackno: "<<seg.header().ackno<<endl;
			}
		}
		//push the new segment to the queue
		_segments_out.push(seg);//send now
	}
}
void TCPConnection::connect() {
	if(_sender.next_seqno_absolute()==0){//never send a syn out there
		_sender.fill_window();
		pop_out();
	}
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
        	cerr << "Warning: Unclean shutdown of TCPConnection\n";
		abortion();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
