#pragma once
#include <array>
#include <string>
#include <vector>
extern "C" {
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <sys/un.h>
	//#include <linux/ax25.h> //Many systems don't have these header, and neither do I so I can't even test/develop for it, but I'd like to
	//#include <netax25/ax25.h>
	//#include <netax25/axlib.h>
	//#include <netax25/axconfig.h>
}
//This is a bit jank, I know, but it is because of the domain enum's `unix` name
//The compiler thinks I am setting a number to a number if I don't undefine
//This might cause issues, but shouldn't since both __unix__ and __unix are available and should be perferred anyway
#ifdef unix
#undef unix
#endif

namespace sks {
	//Address domains
	enum domain {
		unix = AF_UNIX,
		IPv4 = AF_INET,
		IPv6 = AF_INET6
		//ax25 = AF_AX25 //Maybe one day, when I can test it );
	};
	enum type {
		stream = SOCK_STREAM,	//Garuntees order and delivery of bytes; TCP in the IP world
		dgram = SOCK_DGRAM,		//Garuntees message will be valid if received; UDP in the IP world
		seq = SOCK_SEQPACKET,	//Garuntees order and delivery of bytes in message form (Similar to reliable data-grams)
		rdm = SOCK_RDM,			//Garuntees deliver of bytes in message form, but without ordering (DEPRICATED)
		raw = SOCK_RAW			//Raw access to socket layer; Experimental in this context
	};

	
	class addressBase;
	class IPv4Address;
	class IPv6Address;
	class unixAddress;
	//class ax25Address;

	//generic address class
	class address {
	protected:
		addressBase* m_base;
	public:
		address(std::string addrstr, domain d = (domain)0);
		address(sockaddr_storage from, socklen_t len);
		address(const address& addr);
		~address();

		address& operator=(const address& addr);

		operator sockaddr_storage() const;
		operator socklen_t() const;

		domain addressDomain() const;
		std::string name() const;
	};

	//address base-class
	class addressBase {
	protected:
		domain m_domain; //Domain of this address (each domain should have its own, specific, child-class)
		
		addressBase();
	public:
		virtual operator sockaddr_storage() const = 0;
		virtual operator socklen_t() const = 0;
	
		domain addressDomain() const;
		//Get a human-readable representation of the address
		//This string should be usable to construct an identical address
		//Must be implemented by child classes
		virtual std::string name() const = 0;
	};
	//bool createAddress(const std::string addrstr, addressBase& to); //(try to) create an address based on string alone
	//void createAddress(const sockaddr_storage from, const socklen_t len, addressBase& to); //Convert sockaddr to address
	//Both createAddress functions create either an IPv4 or IPv6 address (No unix, use unix directly if needed)


	class IPv4Address : public addressBase {
	protected:
		std::array<uint8_t, 4> m_addr; //32-bit address
		uint16_t m_port = 0;
		std::string m_name;
	public:
		IPv4Address(uint16_t port = 0); //Construct an any address
		IPv4Address(const std::string addrstr); //Parse address from string
		IPv4Address(const sockaddr_in addr); //Construct from C struct
		operator sockaddr_in() const; //Cast to C struct
		operator socklen_t() const; //Length associated with above (sockaddr_in cast)
		operator sockaddr_storage() const;
	
		std::array<uint8_t, 4> addr() const;
		uint16_t port() const;
		std::string name() const;
	};
	
	class IPv6Address : public addressBase {
	protected:
		std::array<uint16_t, 8> m_addr; //128-bit address
		uint16_t m_port = 0;
		std::string m_name;
	public:
		IPv6Address(uint16_t port = 0); //Construct an any address
		IPv6Address(const std::string addrstr); //Parse address from string
		IPv6Address(const sockaddr_in6 addr); //Construct from C struct
		operator sockaddr_in6() const; //Cast to C struct
		operator socklen_t() const; //Length associated with above (sockaddr_in6 cast)
		operator sockaddr_storage() const;
		
		std::array<uint16_t, 8> addr() const;
		std::array<uint16_t, 3> sitePrefix() const;
		uint16_t subnetId() const;
		std::array<uint16_t, 4> interfaceId() const;
		uint16_t port() const;
		std::string name() const;
	};
	
	class unixAddress : public addressBase {
	protected:
		//null-terminated path name
		//unnamed (size == 0)
		//abstract (m_addr[0] == '\0')
		std::vector<char> m_addr;
	public:
		unixAddress(const std::string addrstr); //Parse address from string
		unixAddress(const sockaddr_un addr, const socklen_t len); //Construct from C struct
		operator sockaddr_un() const; //Cast to C struct
		operator socklen_t() const; //Length associated with above (sockaddr_un cast)
		operator sockaddr_storage() const;
		
		std::string name() const;
		bool named() const;
	};
	
	//Planning for ax25 address, cannot develop/test due to limited support and little documentation
	/*
	class ax25Address : public addressBase {
	protected:
		std::string m_call; //Callsign KI7SKS or similar
		char m_ssid;
		int m_ndigis;
		std::string m_name;
	public:
		ax25Address(const std::string addrstr); //Parse address from string
		ax25Address(const sockaddr_ax25); //Construct from C struct
		operator sockaddr_ax25() const; //Cast to C struct
		operator socklen_t() const; //Length associated with above (sockaddr_ax25 cast)
		
		std::string call() const;
		std::string callsign() const = call;
		char ssid() const;
		std::string name() const;
	}
	*/
};
