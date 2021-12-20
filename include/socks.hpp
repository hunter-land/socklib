#pragma once
extern "C" {
	#ifndef _WIN32 //POSIX, for normal people
		#undef _POSIX_C_SOURCE //Needed here (in header) for SOCK_RDM on MacOS
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
#include <ostream>

#include "addrs.hpp" //Addresses and domains

namespace sks {
	/*extern const struct {
		uint16_t major;
		uint16_t minor;
		uint16_t build;
	} versionInfo;*/
		
	class socket {
	protected:
		bool m_validFD = false; //this is used for move constructor and deconstruction, otherwise we risk closing a different file descriptor unexpectedly.
		int m_sockFD; //underlying socket file descriptor, only read if m_validFD is true.
		domain m_domain; //domain this socket is operating on, cannot be switched (assigned at construction)
		type m_type; //type of socket this is, cannot be switched (assigned at construction)
		int m_protocol; //specific protocol of this socket, cannot be switched (assigned at construction)
		
		
		socket(int sockFD, domain d, type t, int protocol);
		friend std::pair<socket, socket> socketPair(domain d, type t, int protocol);
	public:
		socket(domain d, type t, int protocol = 0);
		socket(const socket& s) = delete; //socket cannot be construction-copied
		socket(socket&& s); //socket can be construction-moved
		~socket();
		
		socket& operator=(const socket& s) = delete; //socket cannot be assignment-copied
		socket& operator=(socket&& s) = delete; //socket cannot be assignment-moved
		
		void bind(const address& address);
		void listen(int backlog = 0xFF);
		socket accept();
		void connect(const address& address);
		
		void send(std::vector<uint8_t> data);
		void send(std::vector<uint8_t> data, address& to);
		std::vector<uint8_t> receive(size_t bufSize = 0x10000);
		std::vector<uint8_t> receive(address& from, size_t bufSize = 0x10000);
	};
	std::pair<socket, socket> socketPair(domain d, type t, int protocol = 0);
};
