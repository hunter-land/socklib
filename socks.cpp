#include "socks.hpp"
extern "C" {
	#ifndef _WIN32
		#include <sys/socket.h>
	#else
		#include <winsock2.h>
	#endif
	
	#include <errno.h> //Error numbers
	#include <string.h> //strerror
	#include <sys/types.h> //sockopt
	
	#ifndef _WIN32
		#include <netinet/in.h> //sockaddr_in
		#include <sys/un.h> //sockaddr_un
		#include <arpa/inet.h> //inet_pton
		#include <unistd.h> //close
		#include <sys/ioctl.h> //ioctl
		#include <sys/time.h> //timeval
	#else
		#pragma comment(lib, "Ws2_32.lib")
		#include <ws2tcpip.h> //should have inet_pton but only does sometimes: Schrödinger's header
		#include <io.h>
		#include <ioapiset.h>
	#endif
}
#include <stdexcept>

namespace sks {
	
	std::string to_string(domain d) {
		switch (d) {
			case unix:
				return "UNIX";
			case ipv4:
				return "IPv4";
			case ipv6:
				return "IPv6";
			default:
				return "";
		}
	}
	std::string to_string(protocol p) {
		switch (p) {
			case tcp:
				return "tcp";
			case udp:
				return "udp";
			case seq:
				return "seq";
			default:
				return "";
		}
	}
	std::string to_string(sockaddress sa) {
		return sa.addrstring + (sa.port != 0 ? (std::string(":") + std::to_string(sa.port)) : "");
	}
	
	std::string errorstr(int e) {
		std::string s;
		#ifndef _WIN32
			char* errstr = strerror(e);
		#else
			char errstr[256];
			strerror_s(errstr, 256, e);
		#endif
		s = errstr;
		return s;
	}
	std::string errorstr(serror e) {
		std::string s;
		
		switch (e.type) {
			case BSD:
				s = errorstr(e.erno);
				break;
			case USER:
				switch (e.erno) {
					case BADPSR:
						s = "Bad Pre-Send function return (!=0)";
						break;
					case BADPRR:
						s = "Bad Post-Receive function return (!=0)";
						break;
				}
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
				}
				break;
		}
		
		return s;
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
	
	//Converting sockaddress to sockaddr
	sockaddr_storage satosa(const sockaddress& s) {
		sockaddr_storage saddr;
		
		saddr.ss_family = s.d;
		switch (s.d) {
			case ipv4:
				{
					sockaddr_in* a = (sockaddr_in*)&saddr;
					
					memcpy(&a->sin_addr, s.addr, sizeof(a->sin_addr));
					
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
					
					a->sin_port = ntohs( s.port );
				}
				break;
			case ipv6:
				{
					sockaddr_in6* a = (sockaddr_in6*)&saddr;
					
					memcpy(&a->sin6_addr, s.addr, sizeof(a->sin6_addr));
					
					bool addrzero = true;
					for (size_t i = 0; i < sizeof(s.addr) && i < sizeof(a->sin6_addr) && addrzero; i++) {
						addrzero &= s.addr[i] == 0;
					}
					
					if (addrzero) {
						if (s.addrstring.size() > 0) { //Get address from string
							inet_pton( s.d, s.addrstring.c_str(), &a->sin6_addr );
						} else {
							a->sin6_addr = in6addr_any;
							//a->sin6_addr = IN6ADDR_ANY_INIT;
						}
					}
					
					a->sin6_port = ntohs( s.port );
				}
				break;
			case unix:
				{
					sockaddr_un* a = (sockaddr_un*)&saddr;
					
					a->sun_path[0] = 0;
					strncpy(a->sun_path, s.addrstring.c_str(), sizeof(a->sun_path));
					a->sun_path[sizeof(a->sun_path) / sizeof(a->sun_path[0]) - 1] = 0; //Ensure null-termination
				}
				break;
		}
		
		return saddr;
	}
	//Converting sockaddr to sockaddress
	sockaddress satosa(const sockaddr_storage* const saddr) {
		sockaddress s;
		
		s.d = (domain)saddr->ss_family;
		switch (s.d) {
			case ipv4:
				{
					sockaddr_in* a = (sockaddr_in*)saddr;
					
					memcpy(s.addr, &a->sin_addr, sizeof(a->sin_addr));
					s.port = ntohs( a->sin_port );
				}
				break;
			case ipv6:
				{
					sockaddr_in6* a = (sockaddr_in6*)saddr;
					
					memcpy(s.addr, &a->sin6_addr, sizeof(a->sin6_addr));
					s.port = ntohs( a->sin6_port );
				}
				break;
			case unix:
				{
					sockaddr_un* a = (sockaddr_un*)saddr;
					
					s.addrstring = a->sun_path;
					memset(&s.addr, 0, sizeof(s.addr));
					s.port = 0;
				}
				break;
		}
		
		size_t addrstrlen = std::max(INET6_ADDRSTRLEN, INET_ADDRSTRLEN);
		char addrstr[addrstrlen];
		const char* e = inet_ntop(s.d, &s.addr, addrstr, addrstrlen);
		if (e != nullptr) {
			s.addrstring = addrstr;
		}
		
		return s;
	}
	
	//Constructors and destructors
	socket_base::socket_base(domain d, protocol t, int p) {
		m_domain = d;
		m_protocol = t;
		m_sockid = socket(d, t, p);
		if (m_sockid == -1) {
			//throw exception here
			std::string msg("Failed to construct socket: ");
			msg += errorstr(errno);
			throw std::runtime_error(msg);
		}
		//std::cout << "Created socket #" << m_sockid << std::endl;
		
		//This is un-necessary and problematic:
		/*#ifndef _WIN32 //POSIX, for normal people
			int tru = 1; //setsockopt needs this to be pointed to, so we make the variable here
			setsockopt(m_sockid, SOL_SOCKET, SO_REUSEADDR, &tru, sizeof(tru));
		#else //Whatever-this-is, for windows people
			DWORD tru = 1;
			setsockopt(m_sockid, SOL_SOCKET, SO_REUSEADDR, (char*)&tru, sizeof(tru));
		#endif*/
	}
	socket_base::socket_base(int sockfd, sockaddr *saddr, protocol t) {
		//std::cout << "Wrapping socket #" << sockfd << std::endl;
		m_sockid = sockfd;
		m_protocol = t;
		m_domain = (domain)saddr->sa_family;
		
		setlocinfo();
		setreminfo();
		
		m_valid = true;
	}
	socket_base::socket_base(socket_base&& s) {
		std::swap(m_sockid, s.m_sockid);
		m_domain = std::move(s.m_domain);
		m_protocol = std::move(s.m_protocol);
		m_valid = std::move(s.m_valid);
		m_listening = std::move(s.m_listening);
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
				_close(m_sockid);
			}
		#endif
		if (m_domain == unix) {
			unlink(m_loc_addr.addrstring.c_str());
		}
	}
	
	sockaddress socket_base::locaddr() {
		return m_loc_addr;
	}
	sockaddress socket_base::remaddr() {
		return m_rem_addr;
	}
	
	int socket_base::setbroadcastoption(bool b) {
		#ifndef _WIN32 //POSIX, for normal people
			int r = setsockopt(m_sockid, SOL_SOCKET, SO_BROADCAST, (int*)&b, sizeof(b));
		#else //Whatever-this-is, for windows people
			int r = setsockopt(m_sockid, SOL_SOCKET, SO_BROADCAST, (char*)&b, sizeof(b));
		#endif
		
		return r;
	}
	bool socket_base::broadcastoption() {
		bool b;
		socklen_t len = sizeof(b);
		#ifndef _WIN32 //POSIX, for normal people
			getsockopt(m_sockid, SOL_SOCKET, SO_BROADCAST, (int*)&b, &len);
		#else //Whatever-this-is, for windows people
			getsockopt(m_sockid, SOL_SOCKET, SO_BROADCAST, (char*)&b, &len);
		#endif
		return b;
	}
	int socket_base::setkeepaliveoption(bool b) {
		#ifndef _WIN32 //POSIX, for normal people
			int r = setsockopt(m_sockid, SOL_SOCKET, SO_KEEPALIVE, (int*)&b, sizeof(b));
		#else //Whatever-this-is, for windows people
			int r = setsockopt(m_sockid, SOL_SOCKET, SO_KEEPALIVE, (char*)&b, sizeof(b));
		#endif
		
		return r;
	}
	bool socket_base::keepaliveoption() {
		bool b;
		socklen_t len = sizeof(b);
		#ifndef _WIN32 //POSIX, for normal people
			getsockopt(m_sockid, SOL_SOCKET, SO_KEEPALIVE, (int*)&b, &len);
		#else //Whatever-this-is, for windows people
			getsockopt(m_sockid, SOL_SOCKET, SO_KEEPALIVE, (char*)&b, &len);
		#endif
		return b;
	}
	
	//Timeout values
	int socket_base::setrxtimeout(std::chrono::microseconds time) {
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
			return r;
		}
		
		m_rxto = time;
		return 0;
	}
	int socket_base::setrxtimeout(uint64_t time_usec) {
		return setrxtimeout(std::chrono::microseconds(time_usec));
	}
	int socket_base::settxtimeout(std::chrono::microseconds time) {
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
			return r;
		}
		
		m_txto = time;
		return 0;
	}
	int socket_base::settxtimeout(uint64_t time_usec) {
		return settxtimeout(std::chrono::microseconds(time_usec));
	}
	std::chrono::microseconds socket_base::rxtimeout() {
		return m_rxto;
	}
	std::chrono::microseconds socket_base::txtimeout() {
		return m_txto;
	}
	
	int socket_base::bind() {
		sockaddress sa;
		sa.d = m_domain;
		memset(&sa.addr, 0, sizeof(sa.addr));
		sa.port = 0;
		
		return bind(sa);
	}
	int socket_base::bind(unsigned short port) {
		sockaddress sa;
		sa.d = m_domain;
		memset(&sa.addr, 0, sizeof(sa.addr));
		sa.port = port;
		
		return bind(sa);
	}
	int socket_base::bind(std::string addr) {
		sockaddress sa;
		sa.d = m_domain;
		sa.addrstring = addr;
		sa.port = INADDR_ANY;
		
		return bind(sa);
	}
	int socket_base::bind(std::string addr, unsigned short port) {
		sockaddress sa;
		sa.d = m_domain;
		sa.addrstring = addr;
		sa.port = port;
		
		return bind(sa);
	}
	int socket_base::bind(sockaddress sa) {
		sockaddr_storage saddr = satosa(sa);
		int slen = sizeof(saddr);
				
		if (::bind(m_sockid, (sockaddr*)&saddr, slen) == -1) {
			return errno;
		} else {
			setlocinfo();
			if (m_protocol == udp) {
				m_valid = true;
			}
			m_bound = true;
			return 0;
		}
	}
	
	serror socket_base::listen(int backlog) {
		serror e;
		e.erno = 0;
		
		if (!m_bound) {
			e.type = CLASS;
			e.erno = UNBOUND;
			return e;
		}
		if (::listen(m_sockid, backlog) == -1) {
			e.type = BSD;
			e.erno = errno;
			return e;
		} else {
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
			throw std::runtime_error(msg);
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
			throw std::runtime_error(msg);
		}
		socket_base s(newsockid, (sockaddr*)&saddr, m_protocol);
		s.setpre(pre());
		s.setpost(post());
		return s;
	}
	
	int socket_base::connect(sockaddress sa) {
		sockaddr_storage saddr = satosa(sa);
		socklen_t slen = sizeof(saddr);
		
		if (::connect(m_sockid, (sockaddr*)&saddr, slen) == -1) {
			return errno;
		}
		
		setlocinfo();
		setreminfo();
		m_valid = true;
		return 0;
	}
	
	//Data handling functions
	serror socket_base::recvfrom(packet& pkt, int flags, uint32_t n) {
		serror e;
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
		pkt.rem = satosa(&saddr);
		
		//Do post-recv function
		if (m_postrecv != nullptr) {
			int r = m_postrecv(pkt);
			if (r != 0) {
				//User's program has returned an error code
				e.type = USER;
				e.erno = r;
				return e;
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
		e.erno = 0;
		
		if (m_valid == false) {
			e.type = CLASS;
			e.erno = INVALID;
			return e;
		}
		
		if (m_presend != nullptr) {
			int r = m_presend(pkt);
			if (r != 0) {
				//Failure; Abort
				e.type = USER;
				e.erno = r;
				return e;
			}
		}
				
		if (m_protocol == tcp) {
			pkt.rem = m_rem_addr;
		}
		
		sockaddr_storage saddr = satosa(pkt.rem);
		socklen_t slen = sizeof(saddr);
		
		#ifndef _WIN32
			ssize_t br = ::sendto(m_sockid, (void*)pkt.data.data(), pkt.data.size(), flags, (sockaddr*)&saddr, slen);
		#else
			int br = ::sendto(m_sockid, (char*)pkt.data.data(), pkt.data.size(), flags, (sockaddr*)&saddr, slen);
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
		return sendto(pkt, flags);
	}
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
	
	//Pre and Post functions
	void socket_base::setpre(std::function<int(packet&)> f) {
		m_presend = f;
	}
	void socket_base::setpost(std::function<int(packet&)> f) {
		m_postrecv = f;
	}
	std::function<int(packet&)> socket_base::pre() {
		return m_presend;
	}
	std::function<int(packet&)> socket_base::post() {
		return m_postrecv;
	}
	
	//Protected functions:
	void socket_base::setlocinfo() {
		sockaddr_storage saddr;
		socklen_t slen = sizeof(saddr);
		
		if (getsockname(m_sockid, (sockaddr*)&saddr, &slen) == -1) {
			return;
		}
		
		m_loc_addr = satosa(&saddr);
	}
	void socket_base::setreminfo() {
		sockaddr_storage saddr;
		socklen_t slen = sizeof(saddr);
		
		if (getpeername(m_sockid, (sockaddr*)&saddr, &slen) == -1) {
			return;
		}
		
		m_rem_addr = satosa(&saddr);
	}
}