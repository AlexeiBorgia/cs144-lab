#include "wrapping_integers.hh"
#include <iostream>
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;
constexpr int debug=0;
//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint64_t unwrapped =n+static_cast<uint64_t>(isn.raw_value());
    uint32_t wrapped=static_cast<uint32_t>(unwrapped);
    return WrappingInt32(wrapped);
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    if(debug==1)cout<<"wrap: "<<n<<" "<<"isn: "<<isn;
    uint64_t n64=static_cast<uint64_t>(n.raw_value());
    uint64_t _isn=static_cast<uint64_t>(isn.raw_value());
    uint64_t multi=static_cast<uint64_t>(1)<<32;
    uint64_t begin=0;
    begin=((n64<_isn)?(n64+multi-_isn):(n64-_isn));
    uint64_t ret=0;
    if(debug){
    	cout<<"begin: "<<begin<<endl;
    }
    if(begin>=checkpoint){
    	return begin;
    }else{
		uint64_t m=((checkpoint-begin)>>32);
		uint64_t r=(((checkpoint-begin)<<32)>>32);
		if(r<(multi>>1))ret=begin+m*multi;
		else ret=begin+(m+1)*multi;
	}
	return ret;
}
