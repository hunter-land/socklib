#include "addrs.hpp"
#include "errors.hpp"
#include "initialization.hpp"
#include <cstring>
#include <regex>
extern "C" {
	#if __has_include(<unistd.h>) & __has_include(<sys/socket.h>) //SHOULD be true if POSIX, false otherwise
		#include <sys/socket.h>
		#include <sys/un.h>
		#include <netdb.h>
		#include <arpa/inet.h> //inet -> string
	
		#if defined __has_include
			#if __has_include (<netax25/axlib.h>)
				#include <netax25/ax25.h> //ax25 support and structs
				#include <netax25/axlib.h> //ax25 manipulations, such as ax25_aton
			
				//#define has_ax25
			#endif
		#endif
		#define __AS_POSIX__
	#elif defined _WIN32
		#include <ws2tcpip.h> //WinSock 2
		
		#define sa_family_t ADDRESS_FAMILY
		#define errno WSAGetLastError() //Acceptable if only reading socket errors, per https://docs.microsoft.com/en-us/windows/win32/winsock/error-codes-errno-h-errno-and-wsagetlasterror-2
		#define __AS_WINDOWS__
	#endif
}

namespace sks {

	address::address(const std::string& addrstr, domain d) {
		//If we are not given a specific domain to check, try to parse as: IPv6, IPv4
		switch (d) {
			case IPv4:
				m_addresses.IPv4 = new IPv4Address(addrstr);
				m_domain = IPv4;
				break;
			case IPv6:
				m_addresses.IPv6 = new IPv6Address(addrstr);
				m_domain = IPv6;
				break;
			case unix:
				m_addresses.unix = new unixAddress(addrstr);
				m_domain = unix;
				break;
			#ifdef has_ax25
				case ax25:
					m_addresses.ax25 = new ax25Address(addrstr);
					m_domain = ax25;
					break;
			#endif
			default: //No/unknown domain given, do guess-and-check address resolving
				try {
					m_addresses.IPv6 = new IPv6Address(addrstr);
					m_domain = IPv6;
					break;
				} catch (const std::exception& e) {} //Do nothing, just try the next one
				try {
					m_addresses.IPv4 = new IPv4Address(addrstr);
					m_domain = IPv4;
					break;
				} catch (const std::exception& e) {}
				#ifdef has_ax25
					try {
						m_addresses.ax25 = new ax25Address(addrstr);
						m_domain = ax25;
						break;
					} catch (const std::exception& e) {}
				#endif
				throw std::runtime_error("Could not parse string to address. You may need to specify domain.");
		}
	}
	address::address(const sockaddr_storage& from, socklen_t len) {
		assign(from, len);
	}
	address::address(const addressBase& addr) {
		assign((sockaddr_storage)addr, addr.size());
	}
	address::address(const address& addr) {
		assign((sockaddr_storage)addr, addr.size());
	}
	address::address(address&& addr) {
		m_domain = addr.m_domain;
		m_addresses = addr.m_addresses;
		addr.m_addresses.base = nullptr; //We now own the data, remove addr's pointer
	}
	address::~address() {
		delete m_addresses.base;
	}
	address& address::operator=(const address& addr) {
		assign((sockaddr_storage)addr, addr.size());
		return *this;
	}
	address& address::operator=(address&& addr) {
		m_domain = addr.m_domain;
		m_addresses = addr.m_addresses;
		addr.m_addresses.base = nullptr; //We now own the data, remove addr's pointer
		return *this;
	}
	void address::assign(const sockaddr_storage& from, socklen_t len) {
		delete m_addresses.base;
		m_domain = (domain)from.ss_family;
		switch (from.ss_family) {
			case IPv4:
				m_addresses.IPv4 = new IPv4Address(*(sockaddr_in*)&from);
				break;
			case IPv6:
				m_addresses.IPv6 = new IPv6Address(*(sockaddr_in6*)&from);
				break;
			case unix:
				m_addresses.unix = new unixAddress(*(sockaddr_un*)&from, len);
				break;
			#ifdef has_ax25
				case ax25:
					m_addresses.ax25 = new ax25Address(*(full_sockaddr_ax25*)&from, len);
					break;
			#endif
			default:
				//Domain not supported
				throw sysErr(EFAULT);
		}
	}
	address::operator sockaddr_storage() const {
		return (sockaddr_storage)*m_addresses.base;
	}
	socklen_t address::size() const {
		return m_addresses.base->size();
	}
	address::operator IPv4Address() const {
		if (m_domain != IPv4) {
			throw std::runtime_error("Cannot convert address domain");
		}
		return *m_addresses.IPv4;
	}
	address::operator IPv6Address() const {
		if (m_domain != IPv6) {
			throw std::runtime_error("Cannot convert address domain");
		}
		return *m_addresses.IPv6;
	}
	address::operator unixAddress() const {
		if (m_domain != unix) {
			throw std::runtime_error("Cannot convert address domain");
		}
		return *m_addresses.unix;
	}
	#ifdef has_ax25
	address::operator ax25Address() const {
		if (m_domain != ax25) {
			throw std::runtime_error("Cannot convert address domain");
		}
		return *m_addresses.ax25;
	}
	#endif
	bool address::operator==(const address& r) const {
		if (m_domain == r.m_domain) {
			switch (m_domain) {
				case IPv4:
					return *m_addresses.IPv4 == *r.m_addresses.IPv4;
				case IPv6:
					return *m_addresses.IPv6 == *r.m_addresses.IPv6;
				case unix:
					return *m_addresses.unix == *r.m_addresses.unix;
				#ifdef has_ax25
				case ax25:
					return *m_addresses.ax25 == *r.m_addresses.ax25;
				#endif
			}
		}
		//Different domains, no match
		return false;
	}
	bool address::operator!=(const address& r) const {
		return !(*this == r);
	}
	bool address::operator<(const address& r) const {
		if (m_domain != r.m_domain) {
			return m_domain < r.m_domain;
		}
		switch (m_domain) {
			case IPv4:
				return *m_addresses.IPv4 < *r.m_addresses.IPv4;
			case IPv6:
				return *m_addresses.IPv6 < *r.m_addresses.IPv6;
			case unix:
				return *m_addresses.unix < *r.m_addresses.unix;
			#ifdef has_ax25
			case ax25:
				return *m_addresses.ax25 == *r.m_addresses.ax25;
			#endif
		}
		//Unknown domain, error
		throw std::logic_error("Unknown domain");
	}
	domain address::addressDomain() const {
		return m_domain;
	}
	std::string address::name() const {
		return m_addresses.base->name();
	}

	addressBase::addressBase() {
		if (autoInitialize) {
			initialize(); //addresses cause initialization because WSA needs to be running to parse addresses
		}
	}
	addressBase::~addressBase() {
		if (autoInitialize) {
			deinitialize();
		}
	}

	IPv4Address::IPv4Address(uint16_t port) : IPv4Address("0.0.0.0:" + std::to_string(port)) {} //Construct an any address
	IPv4Address::IPv4Address(const std::string& addrstr) { //Parse address from string
		m_name = addrstr;
		//Parse it!
		/*Accepted formats:
		 *	x.x.x.x
		 *		accepted by getaddrinfo
		 *	x.x.x.x:port
		 *		\d{1-3}\.\d{1-3}\.\d{1-3}\.\d{1-3}:\d{1-5}
		 *	http://google.com
		 *		if "://" exists, split on it { "http" "google.com" }; accepted by getaddrinfo
		 *	http://google.com:443
		 *		if "://" and ":" exist, split on both { "http" "google.com" 443 }; getaddrinfo's port is overwritten by 443
		 */
		
		std::string as = addrstr;
		std::string scheme = ""; //if any scheme (URL only)
		if (addrstr.find("://") != std::string::npos) {
			size_t delimIndex = addrstr.find("://");
			as = addrstr.substr(delimIndex + 3);
			scheme = addrstr.substr(0, delimIndex);
		}
		if (as.find(":") != std::string::npos) {
			//addr:port format (possibly with scheme)
			size_t colonIndex = as.find(':');
			m_port = std::stoul(as.substr(colonIndex + 1));
			as = as.substr(0, colonIndex);
		}
		
		//Prepare
		addrinfo hint;
		memset(&hint, 0, sizeof(hint));
		hint.ai_family = AF_INET;
		hint.ai_flags = AI_CANONNAME;
		
		//Function
		addrinfo* results = nullptr;
		int error;
		if (scheme != "") {
			error = getaddrinfo(as.c_str(), scheme.c_str(), &hint, &results);
		} else {
			error = getaddrinfo(as.c_str(), NULL, &hint, &results);
		}
		if (error != 0) {
			//Error encountered, throw
			
			#ifndef _WIN32
				//Windows being special
				if (error == EAI_SYSTEM) {
					error = errno;
				}
			#endif
			
			//Throw
			//throw sysErr(error);
			throw std::runtime_error("Could not get IPv4 address from " + addrstr);
		}
		
		//Read result
		memcpy(m_addr.data(), &((sockaddr_in*)results->ai_addr)->sin_addr, 4);
		if (m_port == 0) {
			m_port = ntohs(((sockaddr_in*)results->ai_addr)->sin_port);
		}
		
		//Delete
		freeaddrinfo(results);
	}
	IPv4Address::IPv4Address(const sockaddr_in& addr) { //Construct from C struct
		memcpy(m_addr.data(), &addr.sin_addr, 4);
		m_port = ntohs(addr.sin_port);
		//Set m_name accordingly
		char str[64]; //64 should cover everything for IPv4
		const char* r = inet_ntop(IPv4, &addr.sin_addr, str, 64);
		if (r == nullptr) {
			throw sysErr(errno);
		}
		m_name = std::string(r) + ":" + std::to_string(m_port);
	}
	IPv4Address::operator sockaddr_in() const { //Cast to C struct
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		memcpy(&addr.sin_addr, m_addr.data(), 4);
		addr.sin_port = htons(m_port);
		memset(addr.sin_zero, 0, 8);
		return addr;
	}
	socklen_t IPv4Address::size() const {
		return sizeof(sockaddr_in);
	}
	IPv4Address::operator sockaddr_storage() const {
		sockaddr_in addrl = (sockaddr_in)(*this); //Cast to correct struct
		socklen_t addrll = this->size(); //Cast to get length of correct struct
		sockaddr_storage addrs;
		memcpy(&addrs, &addrl, addrll);
		return addrs;
	}
	bool IPv4Address::operator==(const IPv4Address& r) const {
		return m_addr == r.m_addr && m_port == r.m_port;
	}
	bool IPv4Address::operator!=(const IPv4Address& r) const {
		return !(*this == r);
	}
	bool IPv4Address::operator<(const sks::IPv4Address& r) const {
		if (m_addr != r.m_addr) {
			return m_addr < r.m_addr;
		}
		return m_port < r.m_port;
	}
	std::array<uint8_t, 4> IPv4Address::addr() const {
		return m_addr;
	}
	uint16_t IPv4Address::port() const {
		return m_port;
	}
	std::string IPv4Address::name() const {
		return m_name;
	}
	
	void swapEndian(uint16_t* first, size_t n) {
		for (size_t i = 0; i < n; i++) {
			first[i] = ntohs(first[i]);
		}
	}
	
	IPv6Address::IPv6Address(uint16_t port) : IPv6Address("[::]:" + std::to_string(port)) {} //Construct an any address
	IPv6Address::IPv6Address(const std::string& addrstr) { //Parse address from string
		m_name = addrstr;
		//Parse it!
		/*Accepted formats:
		 *	f:f:f:f:f:f:f:f
		 *	f:f:f::f:f
		 *	::f
		 *	[<addr>]:port
		 *	http://google.com
		 *		if "://" exists, split on it { "http" "google.com" } accepted by getaddrinfo
		 *	http://google.com:443
		 *		if "://" and ":" exist, split on both { "http" "google.com" 443 }; getaddrinfo's port is overwritten by 443
		 */
		
		std::string as = addrstr;
		size_t obIndex = addrstr.find('[');
		size_t cbIndex = addrstr.find("]:");
		std::string scheme = ""; //if any scheme (URL only)
		if (addrstr.find("://") != std::string::npos) {
			size_t delimIndex = addrstr.find("://");
			as = addrstr.substr(delimIndex + 3);
			scheme = addrstr.substr(0, delimIndex);
			if (as.find(":") != std::string::npos) {
				//URL has explict port at the end we should parse
				size_t colonIndex = as.find(':');
				m_port = std::stoul(as.substr(colonIndex + 1));
				as = as.substr(0, colonIndex);
			}
		} else if (obIndex < cbIndex && obIndex == 0 && cbIndex != std::string::npos) {
			as = addrstr.substr(0, cbIndex); //[f:f::f
			as = as.substr(1);
			m_port = std::stoul(addrstr.substr(cbIndex + 2));
		}
		
		//Prepare
		addrinfo hint;
		memset(&hint, 0, sizeof(hint));
		hint.ai_family = AF_INET6;
		hint.ai_flags = AI_CANONNAME;
		
		//Function
		addrinfo* results = nullptr;
		int error;
		if (scheme != "") { 
			error = getaddrinfo(as.c_str(), scheme.c_str(), &hint, &results);
		} else {
			error = getaddrinfo(as.c_str(), NULL, &hint, &results);
		}
		if (error != 0) {
			//Error encountered, throw
			
			#ifndef _WIN32
				//Windows being special
				if (error == EAI_SYSTEM) {
					error = errno;
				}
			#endif
			
			//Throw
			//throw std::system_error(std::make_error_code((std::errc)error), "sockaddress::sockaddress");
			throw std::runtime_error("Could not get IPv6 address from " + addrstr);
		}
		
		//Read result
		sockaddr_in6* result = (sockaddr_in6*)results->ai_addr;
		memcpy(m_addr.data(), &result->sin6_addr, 16);
		swapEndian(m_addr.data(), m_addr.size());
		if (m_port == 0) {
			m_port = ntohs(result->sin6_port);
		}
		m_flowInfo = result->sin6_flowinfo;
		m_scopeId = result->sin6_scope_id;
		
		//Delete
		freeaddrinfo(results);
	}
	IPv6Address::IPv6Address(const sockaddr_in6& addr) { //Construct from C struct
		memcpy(m_addr.data(), &addr.sin6_addr, 16);
		swapEndian(m_addr.data(), m_addr.size());
		m_port = ntohs(addr.sin6_port);
		m_flowInfo = addr.sin6_flowinfo;
		m_scopeId = addr.sin6_scope_id;
		//Set m_name accordingly
		char str[64]; //64 should cover everything for IPv6
		const char* r = inet_ntop(IPv6, &addr.sin6_addr, str, 64);
		if (r == nullptr) {
			throw sysErr(errno);
		}
		m_name = "[" + std::string(r) + "]:" + std::to_string(m_port);
	}
	IPv6Address::operator sockaddr_in6() const { //Cast to C struct
		sockaddr_in6 addr;
		addr.sin6_family = AF_INET6;
		memcpy(&addr.sin6_addr, m_addr.data(), 16);
		swapEndian((uint16_t*)&addr.sin6_addr, 8);
		addr.sin6_port = htons(m_port);
		addr.sin6_flowinfo = m_flowInfo;
		addr.sin6_scope_id = m_scopeId;
		return addr;
	}
	socklen_t IPv6Address::size() const {
		return sizeof(sockaddr_in6);
	}
	IPv6Address::operator sockaddr_storage() const {
		sockaddr_in6 addrl = (sockaddr_in6)(*this); //Cast to correct struct
		socklen_t addrll = this->size(); //Cast to get length of correct struct
		sockaddr_storage addrs;
		memcpy(&addrs, &addrl, addrll);
		return addrs;
	}
	bool IPv6Address::operator==(const IPv6Address& r) const {
		return m_addr == r.m_addr && m_port == r.m_port;
	}
	bool IPv6Address::operator!=(const IPv6Address& r) const {
		return !(*this == r);
	}
	bool IPv6Address::operator<(const IPv6Address& r) const {
		if (m_addr != r.m_addr) {
			return m_addr < r.m_addr;
		}
		return m_port < r.m_port;
	}
	std::array<uint16_t, 8> IPv6Address::addr() const {
		return m_addr;
	}
	std::array<uint16_t, 3> IPv6Address::sitePrefix() const {
		return {m_addr[0], m_addr[1], m_addr[2]};
	}
	uint16_t IPv6Address::subnetId() const {
		return m_addr[3];
	}
	std::array<uint16_t, 4> IPv6Address::interfaceId() const {
		return {m_addr[4], m_addr[5], m_addr[6], m_addr[7]};
	}
	uint16_t IPv6Address::port() const {
		return m_port;
	}
	uint32_t IPv6Address::flowInfo() const {
		return m_flowInfo;
	}
	uint32_t IPv6Address::scopeId() const {
		return m_scopeId;
	}
	std::string IPv6Address::name() const {
		return m_name;
	}

	unixAddress::unixAddress(const std::string& addrstr) { //Parse address from string
		//pathnames only (up to sizeof(sockaddr_un.sun_pathlen))
		size_t max = sizeof(sockaddr_un::sun_path);
		if (addrstr.size() + 1 > max) {
			throw std::runtime_error("Pathname too long for unix address");
		}
		m_addr = std::vector<char>(addrstr.begin(), addrstr.end());
	}
	unixAddress::unixAddress(const sockaddr_un& addr, const socklen_t len) { //Construct from C struct
		if ((size_t)len > offsetof(sockaddr_un, sun_path) + 1 && addr.sun_path[0] != '\0') {
			//pathname
			size_t pathlen = len - offsetof(sockaddr_un, sun_path) - 1;
			m_addr.resize(pathlen);
			memcpy(m_addr.data(), addr.sun_path, pathlen);
		} else if ((size_t)len > sizeof(sa_family_t)) {
			//abstract
			size_t namelen = len - sizeof(sa_family_t);
			m_addr.resize(namelen);
			memcpy(m_addr.data(), addr.sun_path, namelen);
		} else {
			//unnamed, nothing else to do
		}
	}
	unixAddress::operator sockaddr_un() const { //Cast to C struct
		sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		memcpy(addr.sun_path, m_addr.data(), m_addr.size());
		addr.sun_path[m_addr.size()] = 0; //NULL-terminate
		return addr;
	}
	socklen_t unixAddress::size() const {
		if (m_addr.size() == 0) {
			//unnamed
			return sizeof(sa_family_t);
		} else if (m_addr[0] == '\0') {
			//abstract
			return m_addr.size() + sizeof(sa_family_t);
		} else {
			//pathname
			return offsetof(sockaddr_un, sun_path) + m_addr.size() + 1;
		}
	}
	unixAddress::operator sockaddr_storage() const {
		sockaddr_un addrl = (sockaddr_un)(*this); //Cast to correct struct
		socklen_t addrll = this->size(); //Cast to get length of correct struct
		sockaddr_storage addrs;
		memcpy(&addrs, &addrl, addrll);
		return addrs;
	}
	bool unixAddress::operator==(const unixAddress& r) const {
		return m_addr == r.m_addr;
	}
	bool unixAddress::operator!=(const unixAddress& r) const {
		return m_addr != r.m_addr;
	}
	bool unixAddress::operator<(const sks::unixAddress& r) const {
		return m_addr < r.m_addr;
	}
	std::string unixAddress::name() const {
		if (named()) {
			//pathname, can be displayed as is
			return std::string(m_addr.begin(), m_addr.end());
		} else if (m_addr.size() == 0) {
			return "unnamed unix address";
		} else {
			return "abstract unix address";
		}
	}
	bool unixAddress::named() const {
		return m_addr.size() > 0 && m_addr[0] != '\0';
	}

	//ax25Address (Not officially supported, will be supported after the Deutscher Amateur Radio Club reworks the ax25 implementation with grant funds from the ARDC)
	#ifdef has_ax25
	ax25Address::ax25Address(const std::string addrstr) { //Parse address from string
		full_sockaddr_ax25 addr;
		socklen_t len = ax25_aton(addrstr.c_str(), &addr); //Populate
		if (len == -1) {
			throw sysErr(errno);
		}
		ax25Address(addr, len);
	}
	ax25Address::ax25Address(const full_sockaddr_ax25 addr, const socklen_t len) { //Construct from C struct
		//Get values out of `addr` and store
		memcpy(m_call.data(), addr.fsa_ax25.sax25_call.ax25_call, m_call.size()); //Callsign (shifted left once, 6 characters) and ssid

		m_digis.resize(addr.fsa_ax25.sax25_ndigis);
		for(size_t i = 0; i < m_digis.size(); i++) {
			memcpy(m_digis[i].data(), addr.fsa_digipeater[i].ax25_call, m_digis[i].size());
		}

		//Set name; Should be using ntoa but that is only for a single callsign, NOT an ax25 address (which ax25_aton IS)
		//const char* str = ax25_ntoa((&addr);
		m_name = ax25_ntoa(&addr.fsa_ax25.sax25_call);
		if (m_digis.size() > 0) {
			m_name += " VIA";
			for (size_t i = 0; i < m_digis.size(); i++) {
				m_name += " " + std::string(ax25_ntoa(&addr.fsa_digipeater[i]));
			}
		}
	}
	ax25Address::operator full_sockaddr_ax25() const { //Cast to C struct
		full_sockaddr_ax25 addr;
		addr.fsa_ax25.sax25_family = AF_AX25;
		memcpy(addr.fsa_ax25.sax25_call.ax25_call, m_call.data(), m_call.size());

		addr.fsa_ax25.sax25_ndigis = m_digis.size(); //Set ndigis
		for (size_t i = 0; i < m_digis.size(); i++) { //Populate digis (if any)
			memcpy(addr.fsa_digipeater[i].ax25_call, m_digis[i].data(), m_digis[i].size());
		}

		return addr;
	}
	socklen_t ax25Address::size() const { //Length associated with above (full_sockaddr_ax25 cast)
		return sizeof(sockaddr_ax25) + sizeof(ax25_address) * m_digis.size(); //Every digipeater entry is one ax25_address in size
	}
	ax25Address::operator sockaddr_storage() const {
		full_sockaddr_ax25 addrl = (full_sockaddr_ax25)(*this); //Cast to correct struct
		socklen_t addrll = this->size(); //Cast to get length of correct struct
		sockaddr_storage addrs;
		memcpy(&addrs, &addrl, addrll);
		return addrs;
	}
	bool ax25Address::operator==(const ax25Address& r) const {
		//TODO: This (Should different digis be considered the same, or different?
		throw std::logic_error("NOT YET SUPPORTED");
	}
	bool ax25Address::operator!=(const ax25Address& r) const {
		return !(*this == r);
	}
	bool ax25Address::operator<(const ax25Address& r) const {
		throw std::logic_error("NOT YET SUPPORTED");
	}
	std::string ax25Address::callsign() const {
		//callsign is shifted left once, correct and convert it
		ax25_address addr;
		memcpy(&addr, m_call.data(), m_call.size());
		return ax25_ntoa(&addr);
	}
	uint8_t ax25Address::ssid() const {
		uint8_t ssid = m_call[6];
		return ssid; //TODO: Unshift this somehow, maybe, if it is shifted
	}
	std::string ax25Address::name() const {
		return m_name;
	}
	#endif
};
