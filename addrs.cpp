#include "include/addrs.hpp"
#include "include/errors.hpp"
#include <cstring>
#include <regex>
extern "C" {
	#include <sys/socket.h>
	#include <sys/un.h>
	#include <netdb.h>
	#include <arpa/inet.h> //inet -> string
	
	#if defined __has_include
		#if __has_include (<linux/ax25.h>)
			#include <linux/ax25.h> //Many systems don't have these header, and neither do I so I can't even test/develop for it, but I'd like to
			//Update 12/27/2021: I am currently re-compiling with ax25 support on my Linux machine, so lets set some things up with guesswork meanwhile
			#include <netax25/ax25.h>
			#include <netax25/axlib.h>
			#include <netax25/axconfig.h>
			
			#define has_ax25
		#endif
	#endif
}

namespace sks {

	address::address(std::string addrstr, domain d) {
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
			default: //No/unknown domain given, do guess-and-check address resolving
				try {
					m_addresses.IPv6 = new IPv6Address(addrstr);
					m_domain = IPv6;
					break;
				} catch (std::exception& e) {} //Do nothing, just try the next one
				try {
					m_addresses.IPv4 = new IPv4Address(addrstr);
					m_domain = IPv4;
					break;
				} catch (std::exception& e) {}
				throw std::runtime_error("Could not parse string to address. You may need to specify domain.");
		}
	}
	address::address(sockaddr_storage from, socklen_t len) {
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
			default:
				//Domain not supported
				throw sysErr(EFAULT);
		}
	}
	address::address(const addressBase& addr) : address((sockaddr_storage)addr, addr.size()) {}
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
	domain address::addressDomain() const {
		return m_domain;
	}
	std::string address::name() const {
		return m_addresses.base->name();
	}

	addressBase::addressBase() {}
	addressBase::~addressBase() {}

	IPv4Address::IPv4Address(uint16_t port) : IPv4Address("0.0.0.0:" + std::to_string(port)) {} //Construct an any address
	IPv4Address::IPv4Address(const std::string addrstr) { //Parse address from string
		m_name = addrstr;
		//Parse it!
		/*Accepted formats:
		 *	x.x.x.x
		 *		accepted by getaddrinfo
		 *	x.x.x.x:port
		 *		\d{1-3}\.\d{1-3}\.\d{1-3}\.\d{1-3}:\d{1-5}
		 *	http://google.com
		 *		if "://" exists, split on it { "http" "google.com" }; accepted by getaddrinfo
		 */
		
		std::string as = addrstr;
		std::string scheme = ""; //if any scheme (URL only)
		if (addrstr.find("://") != std::string::npos) {
			size_t delimIndex = addrstr.find("://");
			as = addrstr.substr(delimIndex + 3);
			scheme = addrstr.substr(0, delimIndex);
		} else if (regex_match(addrstr, std::regex("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}:\\d{1,5}")) || regex_match(addrstr, std::regex("localhost:\\d{1,5}"))) {
			//addr:port format
			size_t colonIndex = addrstr.find(':');
			m_port = std::stoul(addrstr.substr(colonIndex + 1));
			as = addrstr.substr(0, colonIndex);
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
			//throw std::system_error(std::make_error_code((std::errc)error), "sockaddress::sockaddress");
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
	IPv4Address::IPv4Address(const sockaddr_in addr) { //Construct from C struct
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
	std::array<uint8_t, 4> IPv4Address::addr() const {
		return m_addr;
	}
	uint16_t IPv4Address::port() const {
		return m_port;
	}
	std::string IPv4Address::name() const {
		return m_name;
	}
	
	IPv6Address::IPv6Address(uint16_t port) : IPv6Address("[::]:" + std::to_string(port)) {} //Construct an any address
	IPv6Address::IPv6Address(const std::string addrstr) { //Parse address from string
		m_name = addrstr;
		//Parse it!
		/*Accepted formats:
		 *	f:f:f:f:f:f:f:f
		 *	f:f:f::f:f
		 *	::f
		 *	[<addr>]:port
		 *	http://google.com
		 *		if "://" exists, split on it { "http" "google.com" } accepted by getaddrinfo
		 */
		
		std::string as = addrstr;
		size_t obIndex = addrstr.find('[');
		size_t cbIndex = addrstr.find("]:");
		std::string scheme = ""; //if any scheme (URL only)
		if (addrstr.find("://") != std::string::npos) {
			size_t delimIndex = addrstr.find("://");
			as = addrstr.substr(delimIndex + 3);
			scheme = addrstr.substr(0, delimIndex);
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
		memcpy(m_addr.data(), &((sockaddr_in6*)results->ai_addr)->sin6_addr, 16);
		if (m_port == 0) {
			m_port = ntohs(((sockaddr_in6*)results->ai_addr)->sin6_port);
		}
		
		//Delete
		freeaddrinfo(results);
	}
	IPv6Address::IPv6Address(const sockaddr_in6 addr) { //Construct from C struct
		memcpy(m_addr.data(), &addr.sin6_addr, 16);
		m_port = ntohs(addr.sin6_port);
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
		addr.sin6_port = htons(m_port);
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
	std::string IPv6Address::name() const {
		return m_name;
	}

	unixAddress::unixAddress(const std::string addrstr) { //Parse address from string
		//pathnames only (up to sizeof(sockaddr_un.sun_pathlen))
		size_t max = sizeof(sockaddr_un::sun_path);
		if (addrstr.size() + 1 > max) {
			throw std::runtime_error("pathname too long for unix address");
		}
		m_addr = std::vector<char>(addrstr.begin(), addrstr.end());
	}
	unixAddress::unixAddress(const sockaddr_un addr, const socklen_t len) { //Construct from C struct
		if ((size_t)len > offsetof(sockaddr_un, sun_path) + 1 && addr.sun_path[0] != '\0') {
			//pathname
			size_t pathlen = len - offsetof(struct sockaddr_un, sun_path) - 1;
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
			return offsetof(struct sockaddr_un, sun_path) + m_addr.size() + 1;
		}
	}
	unixAddress::operator sockaddr_storage() const {
		sockaddr_un addrl = (sockaddr_un)(*this); //Cast to correct struct
		socklen_t addrll = this->size(); //Cast to get length of correct struct
		sockaddr_storage addrs;
		memcpy(&addrs, &addrl, addrll);
		return addrs;
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

	//ax25Address (Not supported, hopefully in the future)
	#ifdef has_ax25
	ax25Address::ax25Address(const std::string addrstr) { //Parse address from string
		m_name = addrstr;
		
		m_call.resize(ax25_address::ax25_call); //Make space
		ax25_aton_entry(addrstr.c_str(), m_call.data()); //Put in callsign + SSID
		
		m_ndigits = 0; //TODO: What is this for, and when should it be set/what should it be set to?
		
		
		
		//m_call = addrstr; //TODO: Remove any call suffix/ssid (ie "-1", "-2", etc)
		//m_ssid = ??; //TODO: Figure out what this needs to be
		//Yes, yes I do.
		//Furthermore, I may need to add support for `full_sockaddr_ax25` instead of just `sockaddr_ax25`
		//As stated elsewhere, I cannot actually develop or test this due to documentation and lack of testing hardware
	}
	ax25Address::ax25Address(const sockaddr_ax25 addr) { //Construct from C struct
		m_call.resize(ax25_address::ax25_call);
		memcpy(m_call.data(), &addr.sax25_call, m_call.size()); //Copy call
		m_ndigis = addr.sax25_ndigits;
	}
	ax25Address::operator sockaddr_ax25() const { //Cast to C struct
		sockaddr_ax25 addr;
		addr.sax25_family = AF_AX25;
		
		//TODO: What should I do about callsign here?
		//int error = ax25_aton_entry(m_name, (char*)&addr.sax25_call);
		//if (error != 0) {
		//	throw std::runtime_error("Could not get ax25 address from " + addrstr);
		//}
		addr.sax25_ndigis = m_ndigits;
	}
	ax25Address::socklen_t size() const { //Length associated with above (sockaddr_ax25 cast)
		return sizeof(sockaddr_ax25);
	}
	ax25Address::operator sockaddr_storage() const {
		sockaddr_ax25 addrl = (sockaddr_ax25)(*this); //Cast to correct struct
		socklen_t addrll = this->size(); //Cast to get length of correct struct
		sockaddr_storage addrs;
		memcpy(&addrs, &addrl, addrll);
		return addrs;
	}
	std::string ax25Address::call() const {
		return m_call;
	}
	char ax25Address::ssid() const {
		return m_ssid;
	}
	std::string ax25Address::name() const {
		return m_name;
	}
	#endif
};
