#include "socket.hh"
#include "util.hh"

#include <cstdlib>
#include <iostream>
#include<string>
#include<vector>
using namespace std;

void get_URL(const string &host, const string &path) {
    // Your code here.

    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.

    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).
	TCPSocket tcp;
	Address destination(host,"http");
	tcp.connect(destination);
	string requestline;
	vector<string> headerlines;
	requestline="GET ";
	requestline.append(path);
	requestline+=" ";
	requestline+="HTTP/1.1\r\n";
	string host_header="Host: cs144.keithw.org\r\n";
	string Connection="Connection: close\r\n";
	headerlines.push_back(host_header);
	headerlines.push_back(Connection);
//edit the http message here
	string message;
	message.append(requestline);
	for(string cur:headerlines){
		message.append(cur);
	}
	message.append("\r\n");
//send the message
	tcp.write(message,true);
	string outpin;
	do{
		tcp.read(outpin,100);
		if(outpin.size()){
			cout<<outpin;
		}
	}while(outpin.size());
	tcp.close();
    
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  // For sticklers: don't try to access argv[0] if argc <= 0.
        }

        // The program takes two command-line arguments: the hostname and "path" part of the URL.
        // Print the usage message unless there are these two arguments (plus the program name
        // itself, so arg count = 3 in total).
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        // Get the command-line arguments.
        const string host = argv[1];
        const string path = argv[2];

        // Call the student-written function.
        get_URL(host, path);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
