#include "include/addrs.hpp"
#include "include/errors.hpp"
#include <cstring>
#include <regex>
extern "C" {
	#include <sys/socket.h>
	#include <sys/un.h>
	#include <netdb.h>
}

namespace sks {
	
	address::address() {}
	domain address::addressDomain() const {
		return m_domain;
	}
	bool createAddress(const std::string addrstr, address& to) {
		//try order:
		//ipv6
		//ipv4
		try {
			IPv6Address addr(addrstr);
			to = addr;
			return true;
		} catch (std::exception&) {}
		try {
			IPv4Address addr(addrstr);
			to = addr;
			return true;
		} catch (std::exception&) {}
		return false;
	}
	void createAddress(const sockaddr_storage from, const socklen_t len, address& to) {
		//call appropriate constructor
		switch (from.ss_family) {
			case IPv4:
				to = IPv4Address(*(sockaddr_in*)&from);
				break;
			case IPv6:
				to = IPv6Address(*(sockaddr_in6*)&from);
				break;
			case unix:
				to = unixAddress(*(sockaddr_un*)&from, len);
				break;
			default:
				//Domain not supported
				throw sysErr(EFAULT);
		}
	}
	
	IPv4Address::IPv4Address(uint16_t port) : IPv4Address("0.0.0.0:" + std::to_string(port)) {} //Construct an any address
	IPv4Address::IPv4Address(const std::string addrstr) { //Parse address from string
		m_name = addrstr;
		m_domain = IPv4;
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
			m_port = htons(((sockaddr_in*)results->ai_addr)->sin_port);
		}
		
		//Delete
		freeaddrinfo(results);
	}
	IPv4Address::IPv4Address(const sockaddr_in addr) { //Construct from C struct
		m_domain = IPv4;
		memcpy(m_addr.data(), &addr.sin_addr, 4);
		m_port = addr.sin_port;
	}
	IPv4Address::operator sockaddr_in() const { //Cast to C struct
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		memcpy(&addr.sin_addr, m_addr.data(), 4);
		addr.sin_port = m_port;
		memset(addr.sin_zero, 0, 8);
		return addr;
	}
	IPv4Address::operator socklen_t() const {
		return sizeof(sockaddr_in);
	}
	IPv4Address::operator sockaddr_storage() const {
		sockaddr_in addrl = (sockaddr_in)(*this); //Cast to correct struct
		socklen_t addrll = (socklen_t)(*this); //Cast to get length of correct struct
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
		m_domain = IPv6;
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
			m_port = htons(((sockaddr_in6*)results->ai_addr)->sin6_port);
		}
		
		//Delete
		freeaddrinfo(results);
	}
	IPv6Address::IPv6Address(const sockaddr_in6 addr) { //Construct from C struct
		m_domain = IPv6;
		memcpy(m_addr.data(), &addr.sin6_addr, 16);
		m_port = addr.sin6_port;
	}
	IPv6Address::operator sockaddr_in6() const { //Cast to C struct
		sockaddr_in6 addr;
		addr.sin6_family = AF_INET6;
		memcpy(&addr.sin6_addr, m_addr.data(), 16);
		addr.sin6_port = m_port;
		return addr;
	}
	IPv6Address::operator socklen_t() const {
		return sizeof(sockaddr_in6);
	}
	IPv6Address::operator sockaddr_storage() const {
		sockaddr_in6 addrl = (sockaddr_in6)(*this); //Cast to correct struct
		socklen_t addrll = (socklen_t)(*this); //Cast to get length of correct struct
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
		//pathnames only
		m_domain = unix;
		m_addr = std::vector<char>(addrstr.begin(), addrstr.end());
		m_addr.push_back('\0');
	}
	unixAddress::unixAddress(const sockaddr_un addr, const socklen_t len) { //Construct from C struct
		m_domain = unix;
		size_t pathlen = len - sizeof(sa_family_t);
		if (pathlen > 0 && addr.sun_path[0] != '\0') {
			//pathname
			m_addr.resize(pathlen);
			memcpy(m_addr.data(), addr.sun_path, pathlen);
			m_addr.push_back('\0');
		} else if (pathlen > 0) {
			//abstract
			m_addr.resize(pathlen);
			memcpy(m_addr.data(), addr.sun_path, pathlen);
		} else {
			//unnamed, nothing else to do
		}
	}
	unixAddress::operator sockaddr_un() const { //Cast to C struct
		sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		memcpy(addr.sun_path, m_addr.data(), m_addr.size());
		return addr;
	}
	unixAddress::operator socklen_t() const {
		if (m_addr.size() == 0) {
			//unnamed
			return sizeof(sa_family_t);
		} else if (m_addr[0] == '\0') {
			//abstract
			return m_addr.size() + sizeof(sa_family_t);
		} else {
			//pathname
			return offsetof(struct sockaddr_un, sun_path) + m_addr.size();
		}
	}
	unixAddress::operator sockaddr_storage() const {
		sockaddr_un addrl = (sockaddr_un)(*this); //Cast to correct struct
		socklen_t addrll = (socklen_t)(*this); //Cast to get length of correct struct
		sockaddr_storage addrs;
		memcpy(&addrs, &addrl, addrll);
		return addrs;
	}
	std::string unixAddress::name() const {
		if (named()) {
			//pathname, can be displayed as is
			return std::string(m_addr.begin(), m_addr.end() - 1);
		} else if (m_addr.size() == 0) {
			return "unnamed unix address";
		} else {
			return "abstract unix address";
		}
	}
	bool unixAddress::named() const {
		return m_addr.size() > 0 && m_addr[0] != '\0';
	}

	/* ax25Address (Not supported, hopefully in the future)
	ax25Address::ax25Address(const std::string addrstr) { //Parse address from string
		m_name = addrstr;
		m_call = addrstr; //TODO: Remove any call suffix/ssid (ie "-1", "-2", etc)
		m_ssid = ??; //TODO: Figure out what this needs to be
		m_ndigis = 0; //TODO: What is this for, and when should it be set/what should it be set to?
		//Yes, yes I do.
		//Furthermore, I may need to add support for `full_sockaddr_ax25` instead of just `sockaddr_ax25`
		//As stated elsewhere, I cannot actually develop or test this due to documentation and lack of testing hardware
	}
	ax25Address::ax25Address(const sockaddr_ax25) { //Construct from C struct
	
	}
	ax25Address::operator sockaddr_ax25() const { //Cast to C struct
		sockaddr_ax25 addr;
		addr.sax25_family = AF_AX25;
		int error = ax25_aton_entry(m_name, (char*)&addr.sax25_call);
		if (error != 0) {
			throw std::runtime_error("Could not get ax25 address from " + addrstr);
		}
		addr.sax25_ndigis = m_ndigis;
	}
	ax25Address::operator socklen_t() const { //Length associated with above (sockaddr_ax25 cast)
		return sizeof(sockaddr_ax25);
	}
	ax25Address::operator sockaddr_storage() const {
		sockaddr_ax25 addrl = (sockaddr_ax25)(*this); //Cast to correct struct
		socklen_t addrll = (socklen_t)(*this); //Cast to get length of correct struct
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
	*/
};
