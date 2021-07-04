#pragma once
extern "C" {
	#ifndef _WIN32 //POSIX, for normal people
		#include <sys/socket.h>
		#include <netinet/in.h> //for IPV6_V6ONLY option and others
	#else //Whatever-this-is, for windows people
		#include <winsock2.h>
		#include <netioapi.h> //for IPV6_V6ONLY option
		#include <ws2tcpip.h> //socklen_t
	#endif
}
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>
#include <stdexcept>
//This is a bit jank, I know, but it is because of the domain enum's `unix` name
//The compiler thinks I am setting a number to a number if I don't undefine
//This might cause issues, but shouldn't since both __unix__ and __unix are available and should be perferred anyway
#ifdef unix
#undef unix
#endif

namespace sks {
	extern const uint16_t version_major; //For incompatible API changes
	extern const uint16_t version_minor; //For added functionality in a backwards compatible manner
	extern const uint16_t version_patch; //For backwards compatible bug fixes
	
	enum domain {
		unix = AF_UNIX,
		ipv4 = AF_INET,
		ipv6 = AF_INET6
		//ax25 = AF_AX25
	};
	std::string to_string(domain d);
	enum type {
		stream = SOCK_STREAM,	//Garuntees order and delivery of bytes; TCP in the IP world
		dgram = SOCK_DGRAM,		//Garuntees message will be valid if received; UDP in the IP world
		seq = SOCK_SEQPACKET,	//Garuntees order and delivery of bytes in message form (Similar to reliable data-grams)
		rdm = SOCK_RDM,			//Garuntees deliver of bytes in message form, but without ordering
		raw = SOCK_RAW			//Raw access to socket layer; Experimental in this context
	};
	std::string to_string(type p);
	bool connectionless(type p); //Check if a type is connected or connectionless
	enum option {
		broadcast = SO_BROADCAST,	//bool
		keepalive = SO_KEEPALIVE,	//bool
		ipv6only = IPV6_V6ONLY,		//bool
		listening = SO_ACCEPTCONN,	//bool
		debug = SO_DEBUG,			//bool
		noroute = SO_DONTROUTE,		//bool
		oobil = SO_OOBINLINE, 		//bool
		reuseaddr = SO_REUSEADDR	//bool
	};
	
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
		NOLISTEN, //Socket is not a listener when trying to accept
		FEWBYTES //Not all bytes have been sent
	};
	struct serror { //Socket error
		errortype type; //Error type/source
		int erno; //Error value (If type == USER, value is from packet filter)
		
		union {
			size_t pf_index; //Index of packetfilter (Valid only when type == USER)
			#ifndef _WIN32
				ssize_t bytesSent; //How many bytes WERE sent (Valid only when type == CLASS and erno == FEWBYTES)
			#else
				int bytesSent; //How many bytes WERE sent (Valid only when type == CLASS and erno == FEWBYTES)
			#endif
		};
	};
	std::ostream& operator<<(std::ostream& os, const serror se);
	
	std::string errorstr(int e);
	std::string errorstr(serror e);
	std::string to_string(serror e);
	std::string to_string(errortype e);
	
	struct sockaddress {
		//Default sockaddress
		sockaddress(); //Zeros the addr[] field
		//Attempt to get a sockaddress based on a string input (url, ip, etc)
		sockaddress(std::string str, uint16_t port = 0, domain d = (domain)AF_UNSPEC, type t = (type)0);
		
		domain d;
		uint8_t addr[16]; //Network byte order (Used for ipv4 and ipv6 addresses)
		std::string addrstring = ""; //String representation of address (Used for unix addresses)
		uint16_t port = 0;
	};
	std::string to_string(sockaddress sa);
	//Converting sockaddress to sockaddr
	sockaddr_storage satosa(const sockaddress& s, socklen_t* saddrlen = nullptr);
	//Converting sockaddr to sockaddress
	sockaddress satosa(const sockaddr_storage* const saddr, socklen_t slen = sizeof(sockaddr_storage));
	
	struct packet {
		//Remote address (TX/RX)
		sockaddress rem;
		//Data
		std::vector<uint8_t> data;
	};
	
	typedef std::function<int(packet&)> packetfilter;
	typedef packetfilter packfunc; //Backwards compatibility with older version, do not use in new code
	
	class socket_base {
	protected:
		int m_sockid = -1; //Socket fd value
		domain m_domain; //Domain of this socket
		type m_type; //Type of this socket
		int m_protocol; //Specific protocol of this socket
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
		socket_base(sks::domain d, sks::type t = sks::type::stream, int p = 0);
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
		socket_base& operator=( socket_base&& s ); //Move assignment is okay
		
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
		//Returns error (if any)
		serror setrxtimeout(std::chrono::microseconds time);
		serror setrxtimeout(uint64_t time_usec);
		//Set TX timeout
		//Returns error (if any)
		serror settxtimeout(std::chrono::microseconds time);
		serror settxtimeout(uint64_t time_usec);
		//Get RX timeout
		std::chrono::microseconds rxtimeout();
		//Get TX timeout
		std::chrono::microseconds txtimeout();
		
		//Bind to any port and any IP
		//Returns error (if any)
		serror bind();
		//Bind to given port and any IP
		//Returns error (if any)
		serror bind(unsigned short port);
		//Bind to any port and given IP
		//Returns error (if any)
		serror bind(std::string addr);
		//Bind to given port and given IP
		//Returns error (if any)
		serror bind(std::string addr, unsigned short port);
		//Bind to given socket address
		//Returns error (if any)
		serror bind(sockaddress sa);
		//Start listening to connection requests
		serror listen(int backlog = 0xFF);
		//Accept pending connection request (from listen(...))
		//Returns connected socket
		socket_base accept();
		//Connect to remote socket
		//Returns error (if any)
		serror connect(sockaddress sa);
		//Connect to remote socket
		//Returns error (if any)
		serror connect(std::string remote, uint16_t port);
		
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
		//Returns the negative BSD error (if any), otherwise positive
		int availdata();
		//Returns no error if reading can be done, otherwise returns error
		serror canread(int timeoutms = 0);
		//Returns the negative BSD error (if any), otherwise positive
		serror canwrite(int timeoutms = 0);
		//Get the domain of this socket
		domain getDomain();
		//Get the protocol/type of this socket
		type getType();
		//Get the protocol of this socket
		int getProtocol();
		
		//Set the pre-send function
		void setpre(packetfilter f, size_t index);
		//Set the post-recv function
		void setpost(packetfilter f, size_t index);
		//Get the pre-send function at index
		packetfilter pre(size_t index);
		//Get the post-recv function at index
		packetfilter post(size_t index);
		//Set function to run on socket deconstruction (Useful for any filters which are state-ful)
		//void setcleanup(std::function<void()> f, size_t index);
		//
	};

	class runtime_error : public std::runtime_error {
	public:
		serror se;
		runtime_error(const char* str, serror e);
		runtime_error(std::string str, serror e);
	};
};
