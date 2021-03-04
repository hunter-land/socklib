#pragma once
extern "C" {
	#ifndef _WIN32 //POSIX, for normal people
		#include <sys/socket.h>
		#include <netinet/in.h> //for IPV6_V6ONLY option and others
	#else //Whatever-this-is, for windows people
		#include <winsock2.h>
		#include <netioapi.h> //for IPV6_V6ONLY option
	#endif
}
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <stdexcept>

namespace sks {
	enum domain {
		unix = AF_UNIX,
		ipv4 = AF_INET,
		ipv6 = AF_INET6
	};
	std::string to_string(domain d);
	enum protocol {
		tcp = SOCK_STREAM,
		udp = SOCK_DGRAM,
		seq = SOCK_SEQPACKET,
		rdm = SOCK_RDM,
		raw = SOCK_RAW
	};
	std::string to_string(protocol p);
	bool connectionless(protocol p); //Check if a type/protocol is connected or connectionless
	enum option {
		broadcast = SO_BROADCAST,	//bool
		keepalive = SO_KEEPALIVE,	//bool
		ipv6only = IPV6_V6ONLY,		//bool
		listening = SO_ACCEPTCONN,	//bool
		debug = SO_DEBUG,			//bool
		noroute = SO_DONTROUTE,		//bool
		oobil = SO_OOBINLINE, 		//bool
		reuseaddr = SO_REUSEADDR,	//bool
		error = SO_ERROR			//int
	};
	//Notes of functions to add
	// Get domain
	// Get protocol/type
	// Get (sub-)protocol
	// Check cmsg(3) for packet timestamp info
	
	enum errortype { //Error type/source
		BSD, //C-Socket error (errno)
		CLASS, //Class/C++ error
		USER //User-defined error
	};
	enum classerrors { //Socket class error
		NOBYTES = 1, //Variable has no data
		INVALID, //Socket is in invalid state
		CLOSED, //Connection has been closed proper
		UNBOUND, //Socket was not bound when it should have been
		OPTTYPE, //option selection does not match data type
		NOLISTEN //Socket is not a listener when trying to accept
	};
	//TODO: Fix and update
	enum usererrors { //User error
		BADPSR = 1, //Non-zero return of pre-send
		BADPRR //Non-zero return of post-recv
	};
	struct serror { //Socket error
		errortype type; //Error type/source
		int erno; //Error value
	};
	
	std::string errorstr(int e);
	std::string errorstr(serror e);
	std::string errorstr(errortype e);
	
	struct sockaddress {
		//Default sockaddress
		sockaddress(); //Zeros the addr[] field
		//Attempt to get a sockaddress based on a string input (url, ip:port, etc)
		sockaddress(std::string str, uint16_t port = 0, domain d = (domain)AF_UNSPEC, protocol p = (protocol)0);
		
		domain d;
		uint8_t addr[16]; //Network byte order
		std::string addrstring = "";
		uint16_t port = 0;
	};
	std::string to_string(sockaddress sa);
	//Converting sockaddress to sockaddr
	sockaddr_storage satosa(const sockaddress& s, socklen_t* saddrlen = nullptr);
	//Converting sockaddr to sockaddress
	sockaddress satosa(const sockaddr_storage* const saddr, size_t slen = sizeof(sockaddr_storage));
	
	struct packet {
		//Remote address (TX/RX)
		sockaddress rem;
		//Data
		std::vector<uint8_t> data;
	};
	
	typedef std::function<int(packet&)> packfunc;
	
	class socket_base {
	protected:
		int m_sockid = -1; //Socket fd value
		domain m_domain; //Domain of this socket
		protocol m_protocol; //Type of this socket
		int m_p; //Specific protocol of this socket
		bool m_valid = false; //Is socket connected (connected protos), bound (connection-less protos), or listening (connected protos)
		bool m_listening = false; //Is socket listening (connected protocols only)
		bool m_bound = false; //Is socket bound (explicitly only; not from connect(...) calls)
		sockaddress m_loc_addr; //Socket's local address
		sockaddress m_rem_addr; //Socket's remote/peer address
		
		std::map<size_t, packfunc> m_presend; //Function(s) to call in ascending order before sending a packet
		std::map<size_t, packfunc> m_postrecv; //Function(s) to call in descending order after receiving a packet
		
		std::chrono::microseconds m_rxto = std::chrono::microseconds(0); //RX timeout
		std::chrono::microseconds m_txto = std::chrono::microseconds(0); //TX timeout
		
		sockaddr_storage setlocinfo(); //Update m_loc_addr
		sockaddr_storage setreminfo(); //Update m_rem_addr
	public:
		//Create a new socket
		socket_base(domain d, protocol t = protocol::tcp, int p = 0);
		//Wrap an existing C socket file descriptor with this class
		//Constructor assumes sockfd is in a valid state (able to send/recv) and not a listener
		socket_base(int sockfd);
		//Move constructor
		socket_base( socket_base&& s );
		//Close and deconstruct socket
		~socket_base();
		
		//This object is non-copyable (Would terminate connection in process due to deconstructor)
		socket_base( const socket_base& ) = delete; //No copy constructor
		socket_base& operator=( const socket_base& ) = delete; //No copy assignment
		
		//Get local address
		sockaddress locaddr();
		//Get remote address
		sockaddress remaddr();
		
		//Set given option
		//Returns error (if any)
		serror setoption(option o, int value, int level = SOL_SOCKET);
		//Get given option
		int getoption(option o, serror* e = nullptr, int level = SOL_SOCKET);
		
		//Set RX timeout
		//Returns BSD error (if any)
		int setrxtimeout(std::chrono::microseconds time);
		int setrxtimeout(uint64_t time_usec);
		//Set TX timeout
		//Returns BSD error (if any)
		int settxtimeout(std::chrono::microseconds time);
		int settxtimeout(uint64_t time_usec);
		//Get RX timeout
		std::chrono::microseconds rxtimeout();
		//Get TX timeout
		std::chrono::microseconds txtimeout();
		
		//Bind to any port and any IP
		//Returns BSD error (if any)
		int bind();
		//Bind to given port and any IP
		int bind(unsigned short port);
		//Returns BSD error (if any)
		//Bind to any port and given IP
		//Returns BSD error (if any)
		int bind(std::string addr);
		//Bind to given port and given IP
		//Returns BSD error (if any)
		int bind(std::string addr, unsigned short port);
		//Bind to given socket address
		//Returns BSD error (if any)
		int bind(sockaddress sa);
		//Start listening to connection requests
		serror listen(int backlog = 0xFF);
		//Accept pending connection request (from listen(...))
		//Returns connected socket
		socket_base accept();
		//Connect to remote socket
		//Returns BSD error (if any)
		int connect(sockaddress sa);
		//Connect to remote socket
		//Returns BSD error (if any)
		int connect(std::string remote, uint16_t port);
		
		//Read bytes from socket
		//n should be equal to or larger than expected data
		serror recvfrom(packet& pkt, int flags = 0, uint32_t n = 0x2000); //0x400 = 1KB, 0x100000 = 1MB, 0x40000000, = 1GB, 0x800000 = 8MB
		serror recvfrom(std::vector<uint8_t>& data, int flags = 0, uint32_t n = 0x2000);
		//Send bytes
		serror sendto(packet pkt, int flags = 0);
		serror sendto(std::vector<uint8_t> data, int flags = 0);
		
		//Is this socket able to send data
		bool valid();
		//How many bytes are available to be read (negative for error)
		//Returns the negative BSD error (if any)
		int availdata();
		//Returns the negative BSD error (if any)
		int canread(int timeoutms = 0);
		//Returns the negative BSD error (if any)
		int canwrite(int timeoutms = 0);
		
		//Set the pre-send function
		void setpre(packfunc f, size_t index = 0);
		//Set the post-recv function
		void setpost(packfunc f, size_t index = 0);
		//Set the pre-send function
		packfunc pre(size_t index = 0);
		//Set the post-recv function
		packfunc post(size_t index = 0);
	};

	class runtime_error : public std::runtime_error {
	public:
		serror se;
		runtime_error(const char* str, serror e);
		runtime_error(std::string str, serror e);
	};
};