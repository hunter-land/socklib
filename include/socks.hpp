#pragma once
#include "macros.hpp"
extern "C" {
	#ifdef __SKS_AS_POSIX__ //POSIX, for normal people
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
#include <chrono>

#include "addrs.hpp" //Addresses and domains

namespace sks {
	struct versionInfo {
		uint16_t major;
		uint16_t minor;
		uint16_t build;
	};

	#ifdef _WIN32 //Classic
		#ifndef __MINGW32__ //MinGW is nicer
			#ifdef IS_SKS_SOURCE //Define this if ONLY when building the library
				__declspec(dllexport)
			#else
				__declspec(dllimport)
			#endif
		#endif
	#endif
	const extern versionInfo version;

	enum boolOption {
		debug = SO_DEBUG,
		broadcast = SO_BROADCAST,
		reuseAddr = SO_REUSEADDR,
		keepAlive = SO_KEEPALIVE,

		outOfBandInLine = SO_OOBINLINE,
		dontRoute = SO_DONTROUTE,
	};
	enum intOption {
		sendBufferSize = SO_SNDBUF,
		receiveBufferSize = SO_RCVBUF,
		receiveLowWaterMark = SO_RCVLOWAT,
		sendLowWaterMark = SO_SNDLOWAT,
	};
	enum optionLevel {
		socketLevel = SOL_SOCKET,
	};
	//Unique option types
	//SO_LINGER (linger struct)
	//SO_RCVTIMEO use receiveTimeout(...)
	//SO_SNDTIMEO use sendTimeout(...)

	class socket {
	protected:
		bool m_validFD = false; //this is used for move constructor and deconstruction, otherwise we risk closing a different file descriptor unexpectedly.
		int m_sockFD; //underlying socket file descriptor, only read if m_validFD is true.
		domain m_domain; //domain this socket is operating on, cannot be switched (assigned at construction)
		type m_type; //type of socket this is, cannot be switched (assigned at construction)
		int m_protocol; //specific protocol of this socket, cannot be switched (assigned at construction)

		socket(int sockFD, domain d, type t, int protocol);
		friend std::pair<socket, socket> createUnixPair(type t, int protocol);
		friend std::vector<std::reference_wrapper<socket>> writeReadySockets(std::vector<std::reference_wrapper<socket>>& sockets, std::chrono::milliseconds timeout);
		friend std::vector<std::reference_wrapper<socket>> readReadySockets(std::vector<std::reference_wrapper<socket>>& sockets, std::chrono::milliseconds timeout);
	public:
		socket(domain d, type t, int protocol = 0);
		socket(const socket& s) = delete; //socket cannot be construction-copied
		socket(socket&& s); //socket can be construction-moved
		~socket();

		socket& operator=(const socket& s) = delete; //socket cannot be assignment-copied
		socket& operator=(socket&& s); //socket can be assignment-moved (provides ability to use std::swap)

		bool operator==(const socket& r) const;
		bool operator!=(const socket& r) const;

		//Critical setup functions
		void bind(const address& address);
		void listen(int backlog = 0xFF);
		socket accept();
		void connect(const address& address);

		//Critical usage functions
		void send(const std::vector<uint8_t>& data, int flags = 0);
		void send(const uint8_t* data, size_t len, int flags = 0);
		void send(const std::vector<uint8_t>& data, const address& to, int flags = 0);
		void send(const uint8_t* data, size_t len, const address& to, int flags = 0);
		std::vector<uint8_t> receive(size_t bufSize = 0x10000, int flags = 0);
		size_t receive(uint8_t* buf, size_t bufSize, int flags = 0);
		std::vector<uint8_t> receive(address& from, size_t bufSize = 0x10000, int flags = 0);
		size_t receive(address& from, uint8_t* buf, size_t bufSize, int flags = 0);

		//Critical utility functions
		void sendTimeout(std::chrono::microseconds timeout);
		std::chrono::microseconds sendTimeout() const;
		void receiveTimeout(std::chrono::microseconds timeout);
		std::chrono::microseconds receiveTimeout() const;
		bool writeReady(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) const;
		bool readReady(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) const; //NOTE: Returns true if the remote socket is closed; check if receive returns a vector of size 0
		size_t bytesReady() const;
		//set/get option bool
		void socketOption(boolOption option, bool value, optionLevel level = socketLevel);
		bool socketOption(boolOption option, optionLevel level = socketLevel) const;
		//set/get option int
		void socketOption(intOption option, int value, optionLevel level = socketLevel);
		int socketOption(intOption option, optionLevel level = socketLevel) const;

		//Important utility functions
		address connectedAddress() const;
		address localAddress() const;

		//Compatibility functions
		int socketFD(bool takeOwnership = false);
		int socketFD() const; //Equivalent to socketFD(false)
	};

	std::pair<socket, socket> createUnixPair(type t, int protocol = 0);

	//readReady and writeReady for a group of sockets.
	std::vector<std::reference_wrapper<socket>> writeReadySockets(std::vector<std::reference_wrapper<socket>>& sockets, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
	std::vector<std::reference_wrapper<socket>> readReadySockets(std::vector<std::reference_wrapper<socket>>& sockets, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

};
