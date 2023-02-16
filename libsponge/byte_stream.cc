#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):buf(capacity,'a'),_capacity(capacity),readptr(0),writeptr(0){}
size_t ByteStream::write(const string &data) {
   int readsize=0;
   int length=data.length();
   while(readsize<length){
   	if(writeptr-readptr==_capacity)break;
   	buf[(writeptr++)%_capacity]=data[readsize++];
   }
   return readsize;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
   int _len=len;
   int bufsize=writeptr-readptr;
   int length=bufsize>_len ? len :bufsize;
   int ptr=readptr;
   string s;
   for(int i=0;i<length;i++){
   	int index=(ptr+i)%_capacity;
   	s+=buf[index];
   }
   return s;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
	int _len=len;
	int bufsize=writeptr-readptr;
	int length=bufsize>_len ? len :bufsize;
	readptr+=length;
	
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    int _len=len;
    int readsize=0;
    std::string s;
    while(readsize<_len){
    	if(writeptr==readptr)break;
    	s+=buf[(readptr++)%_capacity];
    	readcnt++;
    	readsize++;
    }
    return s;
}

void ByteStream::end_input() {
	inputend=true;
}

bool ByteStream::input_ended() const { return inputend; }

size_t ByteStream::buffer_size() const { return writeptr-readptr; }

bool ByteStream::buffer_empty() const { return writeptr==readptr; }

bool ByteStream::eof() const { return (readptr==writeptr)&&inputend==true ;}

size_t ByteStream::bytes_written() const { return writeptr; }

size_t ByteStream::bytes_read() const { return readptr; }

size_t ByteStream::remaining_capacity() const { return _capacity-buffer_size(); }
