#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity),unassemble(),unassemblesize(0),_capacity(capacity),_end_index(-1),next_index(0){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::effectinsert(const string&data,size_t index,size_t&begin_index){
	int begin=0;
	if(next_index>=data.size()+index){
		begin_index=next_index;
		return ;
	}
	if(index>next_index){ //can not connect
		begin_index=index;
		begin=0;
	}else{
		begin_index=next_index;
		begin=next_index-index;
	}
	size_t cur=begin;
	size_t cur_index=begin_index;
	while(cur<data.size()){
		if(cur_index>=static_cast<size_t>(_end_index))break;
		if(unassemble.count(cur_index)){
			cur++;
			cur_index++;
			continue;
		}else{
			unassemblesize++;
			unassemble[cur_index++]=data[cur++];
		}
	}//approach overflow
	if(unassemblesize+_output.buffer_size()>_capacity){
		size_t outpace=unassemblesize+_output.buffer_size()-_capacity;
		auto ptr=unassemble.end();
		for(size_t i=0;i<outpace;i++)ptr--;
		unassemble.erase(ptr,unassemble.end());
		unassemblesize-=outpace;
	}
}
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(eof==true){
    	_end_index=index+data.size();
    }
    size_t begin_index=0;
    effectinsert(data,index,begin_index);
    if(begin_index==next_index){
	while(next_index!=static_cast<size_t>(_end_index)&&unassemble.count(next_index)==1){
		string s;
		s+=unassemble[next_index];
		_output.write(s);
		unassemble.erase(next_index++);
		unassemblesize--;
	}
    }
    if(next_index==static_cast<size_t>(_end_index)&&_output.input_ended()==false)_output.end_input();
}


size_t StreamReassembler::unassembled_bytes() const { return unassemblesize; }

bool StreamReassembler::empty() const { return _output.buffer_size()==0&&unassemblesize==0; }
