extern "C" {
	#ifndef _WIN32 //POSIX, for normal people
		#include <sys/socket.h>
	#else //Whatever-this-is, for windows people
		#include <winsock2.h>
	#endif
}
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace sks {
	enum domain {
		unix = AF_UNIX,
		ipv4 = AF_INET,
		ipv6 = AF_INET6
	};
	enum protocol {
		tcp = SOCK_STREAM,
		udp = SOCK_DGRAM,
		seq = SOCK_SEQPACKET
	};
	enum option {
		broadcast = SO_BROADCAST,
		keepalive = SO_KEEPALIVE
	};
	
	enum errortype { //Error type/source
		BSD, //C-Socket error (errno)
		CLASS, //Class/C++ error
		USER //User-defined error
	};
	enum classerrors { //Socket class error
		NOBYTES = 1, //Variable has no data
		INVALID, //Socket is in invalid state
		CLOSED //Connection has been closed proper
	};
	enum usererrors { //User error
		BADPSR = 1, //Non-zero return
		BADPRR //Non-zero return
	};
	struct serror { //Socket error
		errortype type; //Error's domain
		int erno; //Error value
	};
	
	std::string errorstr(int e);
	std::string errorstr(serror e);
	std::string errorstr(errortype e);
	
	struct sockaddress {
		sockaddress();
		
		domain d;
		uint8_t addr[16];
		std::string addrstring = "";
		unsigned short port = 0;
	};
	
	struct packet {
		//Remote address (TX/RX)
		sockaddress rem;
		//Data
		std::vector<uint8_t> data;
	};
		
	class socket_base {
	protected:
		int m_sockid = -1;
		domain m_domain;
		protocol m_protocol;
		bool m_valid = false;
		bool m_listening = false;
		sockaddress m_loc_addr;
		sockaddress m_rem_addr;
		
		std::function<int(packet&)> m_presend = nullptr;
		std::function<int(packet&)> m_postrecv = nullptr;
		
		std::chrono::microseconds m_rxto = std::chrono::microseconds(0);
		std::chrono::microseconds m_txto = std::chrono::microseconds(0);
		
		void setlocinfo();
		void setreminfo();
	public:
		//Create a new socket
		socket_base(domain d, protocol t = protocol::tcp, int p = 0);
		//Wrap an existing C socket file descriptor with this class
		socket_base(int sockfd, sockaddr *saddr, protocol t);
		//Move constructor
		socket_base( socket_base&& s );
		//Close and deconstruct socket
		~socket_base();
		
		//This object is non-copyable (Would terminate connection in process)
		socket_base( const socket_base& ) = delete;
		socket_base& operator=( const socket_base& ) = delete;
		
		//Get local address
		sockaddress locaddr();
		//Get remote address
		sockaddress remaddr();
		
		//Set broadcast option
		//Returns BSD error (if any)
		int setbroadcastoption(bool b);
		//Get broadcast option
		bool broadcastoption();
		//Set keepalive
		//Returns BSD error (if any)
		int setkeepaliveoption(bool b);
		//Get keepalive
		bool keepaliveoption();
		
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
		
		//Start listening to connection requests
		//Returns BSD error (if any)
		int listen(int backlog = 0xFF);
		
		//Accept pending connection request (from listen(...))
		//Returns connected socket
		socket_base accept();
		
		//Connect to remote socket
		//Returns BSD error (if any)
		int connect(sockaddress sa);
		
		//Read bytes from socket (tcp)
		//std::vector<uint8_t> read(size_t n = SIZE_MAX);
		//Read bytes from socket (tcp/udp)
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
		
		//Set the pre-send function
		void setpre(std::function<int(packet&)> f);
		//Set the post-recv function
		void setpost(std::function<int(packet&)> f);
		//Set the pre-send function
		std::function<int(packet&)> pre();
		//Set the post-recv function
		std::function<int(packet&)> post();
	};
};