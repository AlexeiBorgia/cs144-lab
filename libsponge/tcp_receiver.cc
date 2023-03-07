#include "tcp_receiver.hh"
#include<iostream>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
  	auto header=seg.header();
  	bool syn=header.syn;
  	//syn state
  	if(syn==true&&header.fin==false){//receive new syn
  		if(state!=0)goto parsed;
  		_isn=header.seqno.raw_value();
  		state=1;
  		string s=seg.payload().copy();
  		if(s.size()==0) goto parsed;
  		uint64_t index=unwrap(header.seqno+1,WrappingInt32(_isn),_reassembler.next_index+1)-1;
  		_reassembler.push_substring(s,index,0);
  		goto parsed;
  	}
  	if(syn==false&&header.fin==false){//in process message
  		if(state!=1)goto parsed;
  		string s=seg.payload().copy();
  		uint64_t index=unwrap(header.seqno,WrappingInt32(_isn),_reassembler.next_index+1)-1;
  		_reassembler.push_substring(s,index,0);
  		goto parsed;
  	}
  	if(syn==false&&header.fin==true){//get finish 
  		if(state!=1)goto parsed;
  		uint64_t index=unwrap(header.seqno,WrappingInt32(_isn),_reassembler.next_index+1)-1;
  		//std::cout<<"begin index: "<<index<<endl;                                                     //
  		string s=seg.payload().copy();
  		_reassembler.push_substring(s,index,1);
  		if(_reassembler.stream_out().input_ended()==true){
  			state=2;
  		}
  		goto parsed;
  	}
  	if(syn==true&&header.fin==true){//just one word
  		if(state!=0)goto parsed;
  		_isn=header.seqno.raw_value();
  		state=1;
  		uint64_t index=unwrap(header.seqno+1,WrappingInt32(_isn),_reassembler.next_index+1)-1;//playload's index
  		string s=seg.payload().copy();
  		//if(s.size()==0)goto parsed;
  		_reassembler.push_substring(s,index,1);
  		if(_reassembler.stream_out().input_ended()==true){
  			state=2;
  		}
  		goto parsed;
  	}
  	parsed:
  	return;
  	
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
		 if(state==0)return {};
		 return wrap(_reassembler.next_index+stream_out().input_ended()+1,WrappingInt32(_isn));//aseq,ISN
		
}

size_t TCPReceiver::window_size() const { 
	size_t bufsize=_reassembler.stream_out().buffer_size();
	size_t ret=_capacity-bufsize;
	return ret;
 }
