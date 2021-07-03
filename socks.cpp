#define _POSIX_C_SOURCE 200112L //Needed to get addrinfo and related functions <netdb.h>

#include "include/socks.hpp"
extern "C" {	
	//Sockets
	#ifndef _WIN32
		#include <sys/socket.h>
	#else
		//#define NOMINMAX //This doesn't even work
		#define WIN32_LEAN_AND_MEAN //I hate this macro name; its not the 90s anymore
		#include <winsock2.h>
	#endif
	
	#include <errno.h> //Error numbers
	#include <string.h> //strerror
	#include <sys/types.h> //sockopt
	
	//Socket related/helpers
	#ifndef _WIN32
		#include <netinet/in.h> //sockaddr_in
		#include <sys/un.h> //sockaddr_un
		#include <arpa/inet.h> //inet_pton
		#include <unistd.h> //close
		#include <sys/ioctl.h> //ioctl
		#include <sys/time.h> //timeval
		#include <poll.h>
		#include <netdb.h> //addrinfo
	#else
		#pragma comment(lib, "Ws2_32.lib")
		#include <ws2tcpip.h> //should have inet_pton but only does sometimes: Schrödinger's header
		#include <io.h>
		#include <ioapiset.h>
		#include <afunix.h>
		#undef max //Haha, windows still sucks
		#undef min
		#define sa_family_t ADDRESS_FAMILY
		#define errno WSAGetLastError() //As "recommended" by windows
	#endif
}
#include <algorithm>
#include <stdexcept>

namespace sks {
	const uint16_t version_major = 0; //For incompatible API changes
	const uint16_t version_minor = 9; //For added functionality in a backwards compatible manner
	const uint16_t version_patch = 0; //For backwards compatible bug fixes
	
	std::string to_string(domain d) {
		switch (d) {
			case unix:
				return "UNIX";
			case ipv4:
				return "IPv4";
			case ipv6:
				return "IPv6";
			//case ax25:
			//	return "AX.25";
			default:
				return "";
		}
	}
	std::string to_string(type t) {
		switch (t) {
			case stream:
				return "stream";
			case dgram:
				return "datagram";
			case seq:
				return "seq";
			case rdm:
				return "rdm";
			case raw:
				return "raw";
			default:
				return "";
		}
	}
	bool connectionless(type t) {
		switch (t) {
			case stream:
			case seq:
				return false;
			case dgram:
			case rdm:
			case raw:
			default:
				return true;
		}
	}
	std::string to_string(sockaddress sa) {
		
		socklen_t slen;
		sockaddr_storage saddr = satosa(sa, &slen);
		
		char hostName[256];
		int r = getnameinfo((sockaddr*)&saddr, slen, hostName, 256, nullptr, 0, NI_NUMERICHOST); //NI_NUMERICHOST
		if (r != 0) {
			//Malformed address
			return std::string("Malformed address: ") + errorstr(r);
		}
		//hostName is now a numeric string representation of the address (no port)
		
		//Format the address itself
		//	unix:	<path>
		//	ipv4:	<addr>:<port>
		//	ipv6:	[<addr>]:<port>
		//RFC 5952
		std::string addrf;
		switch (sa.d) {
			case ipv6:
				addrf += '[';
				addrf += hostName;
				addrf += ']';
				break;
			default:
				addrf = hostName;
				break;
		}
		return addrf + (sa.port != 0 ? (std::string(":") + std::to_string(sa.port)) : "");
	}
	
	std::string errorstr(int e) {
		std::string s;
		#ifndef _WIN32
			char* errstr = strerror(e);
		#else
			char errstr[256];
			//strerror_s(errstr, 256, e);

			wchar_t *ws = NULL;
			FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, e,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPWSTR)&ws, 0, NULL);
			snprintf(errstr, 256, "%S", ws);
			LocalFree(ws);
		#endif
		s = errstr;
		return s;
	}
	std::string errorstr(serror e) {
		std::string s;
		if (e.erno == 0) {
			return errorstr(0);
		}
		
		switch (e.type) {
			case BSD:
				s = errorstr(e.erno);
				break;
			case USER:
				s = std::string("Packetfilter [") + std::to_string(e.pf_index) + "] failed with code " + std::to_string(e.erno);
				break;
			case CLASS:
				switch (e.erno) {
					case NOBYTES:
						s = "0 bytes returned";
						break;
					case INVALID:
						s = "Socket is invalid";
						break;
					case CLOSED:
						s = "Socket connection is closed";
						break;
					case UNBOUND:
						s = "Socket is not bound";
						break;
					case OPTTYPE:
						s = "Type does not match option's type";
						break;
					case NOLISTEN:
						s = "Socket is not listening";
						break;
					default:
						s = "Unknown class error";
						break;
				}
				break;
			default:
				s = "Unkown error type";
				break;
		}
		
		return s;
	}
	std::string to_string(serror e) {
		return errorstr(e);
	}
	std::string errorstr(errortype e) {
		switch (e) {
			case BSD:
				return "C (BSD) socket error";
			case CLASS:
				return "SKS socket error";
			case USER:
				return "User-defined function error";
		}
		
		return "";
	}
	
	sockaddress::sockaddress() {
		memset(&addr, 0, sizeof(addr));
	}
	sockaddress::sockaddress(std::string str, uint16_t port, domain d, type t) : sockaddress() {
		addrinfo hint;
		memset(&hint, 0, sizeof(hint));
		hint.ai_family = d;
		hint.ai_socktype = t;
		hint.ai_flags = AI_CANONNAME | AI_PASSIVE;
		
		addrinfo* results = nullptr;
		int e = getaddrinfo(str.c_str(), NULL, &hint, &results);
		if (e != 0) {
			//Errno error
			#ifndef _WIN32
				if (e == EAI_SYSTEM) {
					e = errno;
				}
			#endif
			return;
		}
		if (results == nullptr) {
			return;
		}
		
		*this = satosa((sockaddr_storage*)results->ai_addr, results->ai_addrlen);
		addrstring = results->ai_canonname;
		this->port = port;
		freeaddrinfo(results);
	}
	
	//Converting sockaddress to sockaddr
	sockaddr_storage satosa(const sockaddress& s, socklen_t* saddrlen) {
		sockaddr_storage saddr;
		memset(&saddr, 0, sizeof(saddr));
		
		saddr.ss_family = s.d;
		switch (s.d) {
			case ipv4:
				{
					sockaddr_in* a = (sockaddr_in*)&saddr;
					
					memcpy(&a->sin_addr, s.addr, sizeof(a->sin_addr));
					//a->sin_addr.s_addr = htonl(a->sin_addr.s_addr);
					
					bool addrzero = true;
					for (size_t i = 0; i < sizeof(s.addr) && i < sizeof(a->sin_addr) && addrzero; i++) {
						addrzero &= s.addr[i] == 0;
					}
					
					if (addrzero) {
						if (s.addrstring.size() > 0) { //Get address from string
							inet_pton( s.d, s.addrstring.c_str(), &a->sin_addr );
						} else {
							a->sin_addr.s_addr = INADDR_ANY;
						}
					}
					
					a->sin_port = htons( s.port );
					
					//Set address length
					if (saddrlen != nullptr) {
						*saddrlen = sizeof(sockaddr_in);
					}
				}
				break;
			case ipv6:
				{
					sockaddr_in6* a = (sockaddr_in6*)&saddr;
					
					memcpy(&a->sin6_addr, s.addr, sizeof(a->sin6_addr));
					//HtoN not needed for ipv6
					
					bool addrzero = true;
					for (size_t i = 0; i < sizeof(s.addr) && i < sizeof(a->sin6_addr) && addrzero; i++) {
						addrzero &= s.addr[i] == 0;
					}
					
					if (addrzero) {
						if (s.addrstring.size() > 0) { //Get address from string
							inet_pton( s.d, s.addrstring.c_str(), &a->sin6_addr );
						} else {
							a->sin6_addr = in6addr_any;
						}
					}
					
					a->sin6_port = htons( s.port );
					
					//Set address length
					if (saddrlen != nullptr) {
						*saddrlen = sizeof(sockaddr_in6);
					}
				}
				break;
			case unix:
				{
					sockaddr_un* a = (sockaddr_un*)&saddr;
					
					a->sun_path[0] = 0;
					size_t n = std::min(sizeof(a->sun_path) - 1, s.addrstring.size());
					memcpy(a->sun_path, s.addrstring.data(), n);
					a->sun_path[n] = 0; //Ensure null-termination
					
					//Set address length
					if (saddrlen != nullptr) {
						//*saddrlen = sizeof(sa_family_t) + n;
						*saddrlen = sizeof(*a);
					}
				}
				break;
		}
		
		return saddr;
	}
	//Converting sockaddr to sockaddress
	sockaddress satosa(const sockaddr_storage* const saddr, socklen_t slen) {
		sockaddress s;
		
		s.d = (domain)saddr->ss_family;
		size_t addrstrlen = 0;
		switch (s.d) {
			case ipv4:
				{
					sockaddr_in* a = (sockaddr_in*)saddr;
					addrstrlen = INET_ADDRSTRLEN;
					
					memcpy(s.addr, &a->sin_addr, sizeof(a->sin_addr));
					//a->sin_addr.s_addr = ntohl(a->sin_addr.s_addr);
					s.port = ntohs( a->sin_port );
				}
				break;
			case ipv6:
				{
					sockaddr_in6* a = (sockaddr_in6*)saddr;
					addrstrlen = INET6_ADDRSTRLEN;
					
					memcpy(s.addr, &a->sin6_addr, sizeof(a->sin6_addr));
					//NtoH not needed for ipv6
					s.port = ntohs( a->sin6_port );
				}
				break;
			case unix:
				{
					sockaddr_un* a = (sockaddr_un*)saddr;
					
					if (slen > sizeof(sa_family_t)) {  //Socket is not unnamed (we can read sun_path)
						//if sun_path[0] is a null-byte, the socket is an abstract type. Read n = slen - sizeof(sa_family_t) bytes
						if (a->sun_path[0] == 0) {
							size_t n = slen - sizeof(sa_family_t);
							//s.addrstring.resize(n);
							//memcpy(s.addrstring.data(), a->sun_path, n);
							s.addrstring = std::string(a->sun_path, n);
						} else {
							s.addrstring = a->sun_path;
						}
					}
					memset(&s.addr, 0, sizeof(s.addr));
					s.port = 0;
				}
				break;
		}
		
		//This section only succeeds if ipv6 or ipv4 is the domain
		char* addrstr = (char*)std::malloc(addrstrlen);
		const char* e = inet_ntop(s.d, &s.addr, addrstr, addrstrlen);
		if (e != nullptr) {
			s.addrstring = addrstr;
		}
		std::free(addrstr);
		
		return s;
	}
	//Change `any` addresses to a `loopback` equivalent (0.0.0.0 -> 127.0.0.1)
	void anytoloop(sockaddr_storage& saddr) {
		switch (saddr.ss_family) {
			case ipv4:
				{
					sockaddr_in* a = (sockaddr_in*)&saddr;
					if (a->sin_addr.s_addr == INADDR_ANY) {
						a->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
					}
				}
				break;
			case ipv6:
				{
					sockaddr_in6* a = (sockaddr_in6*)&saddr;
					if (memcmp(&a->sin6_addr, &in6addr_any, sizeof(a->sin6_addr)) == 0) {
						a->sin6_addr = in6addr_loopback;
					}
				}
				break;
		}
	}
	
	
	
	//Constructors and destructors
	socket_base::socket_base(sks::domain d, sks::type t, int p) {
		m_domain = d;
		m_type = t;
		m_protocol = p;
		m_sockid = socket(d, t, p);
		if (m_sockid == -1) {
			//throw exception here
			std::string msg("Failed to construct socket: ");
			msg += errorstr(errno);
			throw sks::runtime_error(msg, serror{BSD, errno});
		}
		//std::cout << "Created socket #" << m_sockid << std::endl;
	}
	socket_base::socket_base(int sockfd) {
		//std::cout << "Wrapping socket #" << sockfd << std::endl;
		m_sockid = sockfd;
		
		int t;
		socklen_t tlen = sizeof(t);
		int e;
		
		#ifndef _WIN32
			e = getsockopt(m_sockid, SOL_SOCKET, SO_TYPE, &t, &tlen);
		#else
			e = getsockopt(m_sockid, SOL_SOCKET, SO_TYPE, (char*)&t, &tlen);
		#endif
		
		if (e == 0) {
			m_type = (sks::type)t;
		}
		
		sockaddr_storage lsaddr = setlocinfo();
		sockaddr_storage rsaddr = setreminfo();
		
		m_domain = (sks::domain)lsaddr.ss_family;
		
		//Check if socket is bound
		for (size_t i = sizeof(lsaddr.ss_family); i < sizeof(sockaddr_storage); i++) {
			if (((uint8_t*)&lsaddr)[i] != 0) {
				m_bound = true;
				if (connectionless(m_type)) {
					m_valid = true;
				}
				break;				
			}
		}
		
		//If using a connected protocol, check for an active connection
		if (!connectionless(m_type)) {
			for (size_t i = sizeof(rsaddr.ss_family); i < sizeof(sockaddr_storage); i++) {
				if (((uint8_t*)&rsaddr)[i] != 0) {
					m_valid = true;
					break;				
				}
			}
		}
		
		#ifndef _WIN32
			e = getsockopt(m_sockid, SOL_SOCKET, SO_ACCEPTCONN, &t, &tlen);
		#else
			e = getsockopt(m_sockid, SOL_SOCKET, SO_ACCEPTCONN, (char*)&t, &tlen);
		#endif
		
		if (e == 0) {
			m_listening = t;
			m_valid = true;
		}		
	}
	socket_base::socket_base(socket_base&& s) {
		//std::cout << "Swapping socket #" << s.m_sockid << " with socket #" << m_sockid << std::endl; 
		std::swap(m_sockid, s.m_sockid);
		m_domain = std::move(s.m_domain);
		m_type = std::move(s.m_type);
		m_protocol = std::move(s.m_protocol);
		
		m_valid = std::move(s.m_valid);
		m_listening = std::move(s.m_listening);
		m_bound = std::move(s.m_bound);
		
		m_loc_addr = std::move(s.m_loc_addr);
		m_rem_addr = std::move(s.m_rem_addr);

		m_presend = std::move(s.m_presend);
		m_postrecv = std::move(s.m_postrecv);

		m_rxto = std::move(s.m_rxto);
		m_txto = std::move(s.m_txto);
	}
	socket_base::~socket_base() {
		//std::cout << "Destructing socket #" << m_sockid << std::endl;
		m_valid = false;
		m_listening = false;
		shutdown(m_sockid, 0);
		#ifndef _WIN32 //POSIX, for normal people
			close(m_sockid);
		#else //Whatever-this-is, for windows people
			if (m_sockid >= 0) { //Windows decided to throw an exception if you try to close an invalid/already-closed fd, so thats cool
				closesocket(m_sockid);
			}
		#endif
		if (m_domain == unix) {
			unlink(m_loc_addr.addrstring.c_str());
		}
	}
	socket_base& socket_base::operator=( socket_base&& s ) {
		socket_base(std::move(s));
		return *this;
	}
	
	//Local and remote addresses
	sockaddress socket_base::locaddr() {
		return m_loc_addr;
	}
	sockaddress socket_base::remaddr() {
		return m_rem_addr;
	}
	
	//Options
	serror socket_base::setoption(sks::option o, int value, int level) {
		serror se;
		se.type = BSD;
		se.erno = 0;
		
		#ifndef _WIN32 //POSIX, for normal people
			int r = setsockopt(m_sockid, level, o, (int*)&value, sizeof(value));
		#else //Whatever-this-is, for windows people
			int r = setsockopt(m_sockid, level, o, (char*)&value, sizeof(value));
		#endif
		if (r != 0) {
			se.erno = errno;
		}
		
		return se;
	}
	int socket_base::getoption(sks::option o, serror* se, int level) {
		serror* see = se;
		if (se == nullptr) {
			see = new serror;
		}
		see->type = BSD;
		
		int v;
		socklen_t vlen = sizeof(v);
		#ifndef _WIN32 //POSIX, for normal people
			see->erno = getsockopt(m_sockid, level, o, (int*)&v, &vlen);
		#else //Whatever-this-is, for windows people
			see->erno = getsockopt(m_sockid, level, o, (char*)&v, &vlen);
		#endif
		
		if (se == nullptr) {
			delete see;
		}
		
		return v;
	}
	
	//Timeout values
	serror socket_base::setrxtimeout(std::chrono::microseconds time) {
		#ifndef _WIN32 //POSIX, for normal people
			struct timeval tv;
			tv.tv_sec = (time_t)( std::chrono::duration_cast<std::chrono::seconds>(time).count() );
			tv.tv_usec = (suseconds_t)( std::chrono::duration_cast<std::chrono::microseconds>(time - std::chrono::duration_cast<std::chrono::seconds>(time)).count() );
			
			int r = setsockopt(m_sockid, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		#else //Whatever-this-is, for windows people
			DWORD milli = (DWORD)( std::chrono::duration_cast<std::chrono::milliseconds>(time).count() );
			int r = setsockopt(m_sockid, SOL_SOCKET, SO_RCVTIMEO, (char*)&milli, sizeof(milli));
		#endif
		
		if (r != 0) {
			return { BSD, errno };
		}
		
		m_rxto = time;
		return { CLASS, 0 };
	}
	serror socket_base::setrxtimeout(uint64_t time_usec) {
		return setrxtimeout(std::chrono::microseconds(time_usec));
	}
	serror socket_base::settxtimeout(std::chrono::microseconds time) {
		#ifndef _WIN32 //POSIX, for normal people
			struct timeval tv;
			tv.tv_sec = (time_t)( std::chrono::duration_cast<std::chrono::seconds>(time).count() );
			tv.tv_usec = (suseconds_t)( std::chrono::duration_cast<std::chrono::microseconds>(time - std::chrono::duration_cast<std::chrono::seconds>(time)).count() );
			
			int r = setsockopt(m_sockid, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
		#else //Whatever-this-is, for windows people
			DWORD milli = (DWORD)( std::chrono::duration_cast<std::chrono::milliseconds>(time).count() );
			int r = setsockopt(m_sockid, SOL_SOCKET, SO_SNDTIMEO, (char*)&milli, sizeof(milli));
		#endif
		
		if (r != 0) {
			return { BSD, errno };
		}
		
		m_txto = time;
		return { CLASS, 0 };
	}
	serror socket_base::settxtimeout(uint64_t time_usec) {
		return settxtimeout(std::chrono::microseconds(time_usec));
	}
	std::chrono::microseconds socket_base::rxtimeout() {
		return m_rxto;
	}
	std::chrono::microseconds socket_base::txtimeout() {
		return m_txto;
	}
	
	//Setup
	serror socket_base::bind() {
		sockaddress sa;
		sa.d = m_domain;
		sa.port = 0;
		
		return bind(sa);
	}
	serror socket_base::bind(unsigned short port) {
		sockaddress sa;
		sa.d = m_domain;
		sa.port = port;
		
		return bind(sa);
	}
	serror socket_base::bind(std::string addr) {
		sockaddress sa;
		sa.d = m_domain;
		sa.addrstring = addr;
		sa.port = 0;
		
		return bind(sa);
	}
	serror socket_base::bind(std::string addr, unsigned short port) {
		sockaddress sa;
		sa.d = m_domain;
		sa.addrstring = addr;
		sa.port = port;
		
		return bind(sa);
	}
	serror socket_base::bind(sockaddress sa) {
		socklen_t slen = sizeof(sockaddr_storage);
		sockaddr_storage saddr = satosa(sa, &slen);
		
		if (::bind(m_sockid, (sockaddr*)&saddr, slen) == -1) {
			return { BSD, errno };
		} else {
			setlocinfo();
			if (connectionless(m_type)) {
				m_valid = true;
			}
			m_bound = true;
			return { CLASS, 0 };
		}
	}
	serror socket_base::listen(int backlog) {
		serror e;
		e.type = CLASS;
		e.erno = 0;
		
		if (!m_bound) {
			e.type = CLASS;
			e.erno = UNBOUND;
			return e;
		}
		if (::listen(m_sockid, backlog) == -1) {
			//Failure
			e.type = BSD;
			e.erno = errno;
			return e;
		} else {
			//Success
			m_valid = true;
			m_listening = true;
			return e;
		}
	}
	socket_base socket_base::accept() {
		if (m_listening == false) {
			//If we are not listening, we are unable to accept a connection
			//Since we cannot accept a connection, we cannot fulfill the return type
			//We must error
			std::string msg("socket_base@");
			msg += std::to_string((uintptr_t)this);
			msg += " is not a listener.";
			throw sks::runtime_error(msg, serror{ CLASS, NOLISTEN });
		}
		sockaddr_storage saddr;
		socklen_t saddrlen = sizeof(saddr);
		int newsockid = ::accept(m_sockid, (sockaddr*)&saddr, &saddrlen);
		if (newsockid == -1) {
			//For some reason, we failed to accept a connection
			//We cannot fulfill return type
			//We must error
			std::string msg("Failed to accept connection: ");
			msg += errorstr(errno);
			throw sks::runtime_error(msg, serror{ BSD, errno });
		}
		socket_base s(newsockid);

		//Pass on pre-send and post-recv functions
		for (auto pair : m_presend) {
			s.setpre(pair.second, pair.first);
		}
		for (auto pair : m_postrecv) {
			s.setpost(pair.second, pair.first);
		}
		return s;
	}
	serror socket_base::connect(std::string remote, uint16_t port) {
		return connect(sockaddress(remote, port, m_domain, m_type));
	}
	serror socket_base::connect(sockaddress sa) {
		socklen_t slen = sizeof(sockaddr_storage);
		sockaddr_storage saddr = satosa(sa, &slen);

		anytoloop(saddr);
		
		if (::connect(m_sockid, (sockaddr*)&saddr, slen) == -1) {
			return { BSD, errno };
		}
		
		setlocinfo();
		setreminfo();
		
		m_valid = true;
		return { CLASS, 0 };
	}
	
	//Data handling functions
	serror socket_base::recvfrom(packet& pkt, int flags, uint32_t n) {
		serror e;
		e.type = CLASS;
		e.erno = 0;
		
		if (m_valid == false || n == 0) {
			e.type = CLASS;
			if (m_valid == false) {
				e.erno = INVALID;
			} else {
				e.erno = NOBYTES;
			}
			return e;
		}
		
		sockaddr_storage saddr;
		socklen_t slen = sizeof(saddr);
		memset(&saddr, 0, slen);
		pkt.data.resize(n);
		//uint8_t buf[n]; //0x100
		
		//std::cout << "Reading up to " << n << " bytes ";
		#ifndef _WIN32
			ssize_t br = ::recvfrom(m_sockid, (void*)pkt.data.data(), n, flags, (sockaddr*)&saddr, &slen);
		#else
			int br = ::recvfrom(m_sockid, (char*)pkt.data.data(), n, flags, (sockaddr*)&saddr, &slen);
		#endif
		//std::cout << "(Read " << br << " bytes)" << std::endl;
		if (br <= 0) {
			if (br == 0) {
				m_valid = false; //If we read 0 bytes, the connection has been closed proper
				e.type = CLASS;
				e.erno = CLOSED;
			} else {
				//seterror(errno);
				e.type = BSD;
				e.erno = errno;
				if (e.erno == 104) {
					m_valid = false; //Connection closed by peer
				}
			}
			return e;
		}
		pkt.data.resize(br);
		
		//Fill out packet.rem
		pkt.rem = satosa(&saddr, slen);
		
		//Do post-recv function(s)
		for (auto it = m_postrecv.rbegin(); it != m_postrecv.rend(); it++) {
			if (it->second != nullptr) {
				
				int r = it->second(pkt);
				if (r != 0) {
					//User's program has returned an error code
					e.type = USER;
					e.erno = r;
					e.pf_index = it->first;
					return e;
				}
			}
		}
		
		return e;
	}
	serror socket_base::recvfrom(std::vector<uint8_t>& data, int flags, uint32_t n) {
		packet pkt;
		sks::serror e = recvfrom(pkt, flags, n);
		data = pkt.data;
		return e;
	}
	serror socket_base::sendto(packet pkt, int flags) {
		serror e;
		e.type = CLASS;
		e.erno = 0;
		
		if (m_valid == false) {
			e.type = CLASS;
			e.erno = INVALID;
			return e;
		}
		
		//Do pre-send function(s)
		for (auto it = m_presend.begin(); it != m_presend.end(); it++) {
			if (it->second != nullptr) {
				
				int r = it->second(pkt);
				if (r != 0) {
					//Failure; Abort
					e.type = USER;
					e.erno = r;
					e.pf_index = it->first;
					return e;
				}
			}
		}
		
		socklen_t slen = sizeof(sockaddr_storage);
		sockaddr_storage saddr = satosa(pkt.rem, &slen);
		anytoloop(saddr);
		//std::cout << "Converted any to loop in sendto" << std::endl;
		sockaddr* saddrptr = (sockaddr*)&saddr;
		if (!connectionless(m_type)) { //Connected sockets should have a NULL address (otherwise sendto *might* return errors such as EISCONN)
			saddrptr = NULL;
			slen = 0;
		}
		
		#ifndef _WIN32
			ssize_t br = ::sendto(m_sockid, (void*)pkt.data.data(), pkt.data.size(), flags, saddrptr, slen);
		#else
			int br = ::sendto(m_sockid, (char*)pkt.data.data(), pkt.data.size(), flags, saddrptr, slen);
		#endif
		
		if (br == -1) {
			e.type = BSD;
			e.erno = errno;
			return e;
		}
		
		return e;
	}
	serror socket_base::sendto(std::vector<uint8_t> data, int flags) {
		packet pkt;
		pkt.data = data;
		pkt.rem = m_rem_addr;
		return sendto(pkt, flags);
	}
	
	//Checks
	bool socket_base::valid() {
		return m_valid;
	}
	int socket_base::availdata() {
		if (m_valid == false) {
			return 0;
		}
		
		#ifndef _WIN32
			int d;
			int e = ioctl(m_sockid, FIONREAD, &d);
		#else
			unsigned long d;
			int e = ioctlsocket(m_sockid, FIONREAD, &d);
		#endif

		if (e != 0) {
			return -errno;
		}
		
		return d;
	}
	serror socket_base::canread(int timeoutms) {
		serror e;
		e.type = BSD;
		e.erno = 0;
		
		pollfd pfd;
		pfd.fd = m_sockid;
		pfd.events = POLLIN;
		
		#ifndef _WIN32
			int r = poll(&pfd, 1, timeoutms);
			switch (r) { //>0 = success, 0 = timeout, -1 = error
				case 0: //Timeout
					e.erno = EAGAIN;
					break;
				case -1: //Error
					e.erno = errno;
					break;
				default: //Success
					break;
			}
		#else
			int r = WSAPoll(&pfd, 1, timeoutms);
			switch (r) { //>0 = success, 0 = timeout, SOCKET_ERROR = error
				case 0: //Timeout
					e.erno = WSAEWOULDBLOCK;
					break;
				case -1: //Error
					e.erno = WSAGetLastError();
					break;
				default: //Success
					break;
			}
		#endif
		
		return e; //(pfd.revents & POLLIN) > 0
	}
	serror socket_base::canwrite(int timeoutms) {
		serror e;
		e.type = BSD;
		e.erno = 0;
		
		pollfd pfd;
		pfd.fd = m_sockid;
		pfd.events = POLLOUT;
		
		#ifndef _WIN32
			int r = poll(&pfd, 1, timeoutms);
			switch (r) { //>0 = success, 0 = timeout, -1 = error
				case 0: //Timeout
					e.erno = EAGAIN;
					break;
				case -1: //Error
					e.erno = errno;
					break;
				default: //Success
					break;
			}
		#else
			int r = WSAPoll(&pfd, 1, timeoutms);
			switch (r) { //>0 = success, 0 = timeout, SOCKET_ERROR = error
				case 0: //Timeout
					e.erno = WSAEWOULDBLOCK;
					break;
				case -1: //Error
					e.erno = WSAGetLastError();
					break;
				default: //Success
					break;
			}
		#endif
		
		return e; //(pfd.revents & POLLOUT) > 0
	}
	//Info
	domain socket_base::getDomain() {
		return m_domain;
	}
	type socket_base::getType() {
		return m_type;
	}
	int socket_base::getProtocol() {
		return m_protocol;
	}
	
	//Pre and Post functions
	void socket_base::setpre(packetfilter f, size_t index) {
		m_presend[index] = f;
	}
	void socket_base::setpost(packetfilter f, size_t index) {
		m_postrecv[index] = f;
	}
	packetfilter socket_base::pre(size_t index) {
		return m_presend[index];
	}
	packetfilter socket_base::post(size_t index) {
		return m_postrecv[index];
	}
	
//Protected functions:
	//Local and remote addresses
	sockaddr_storage socket_base::setlocinfo() {
		sockaddr_storage saddr;
		socklen_t slen = sizeof(saddr);
		memset(&saddr, 0, slen); //Zero initialize it
		
		if (getsockname(m_sockid, (sockaddr*)&saddr, &slen) == -1) {
			//Set address family value and keep the rest zero'd
			return saddr;
		}
		
		m_loc_addr = satosa(&saddr, slen);
		
		return saddr;
	}
	sockaddr_storage socket_base::setreminfo() {
		sockaddr_storage saddr;
		socklen_t slen = sizeof(saddr);
		memset(&saddr, 0, slen); //Zero initialize it
		
		if (getpeername(m_sockid, (sockaddr*)&saddr, &slen) == -1) {
			//Set address family value and keep the rest zero'd
			return saddr;
		}
		
		m_rem_addr = satosa(&saddr, slen);
		
		return saddr;
	}
	
	
	
	
	
	
	runtime_error::runtime_error(const char* str, serror e) : std::runtime_error(str) {
		se = e;
	}
	runtime_error::runtime_error(std::string str, serror e) : std::runtime_error(str) {
		se = e;
	}
}