#pragma once
#include <array>
#include <string>
#include <vector>
extern "C" {
	#if __has_include(<unistd.h>) & __has_include(<sys/socket.h>)
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <sys/un.h>
		/*#if __has_include (<netax25/axlib.h>)
			#include <netax25/ax25.h>
			#define has_ax25
		#endif*/
	#elif defined _WIN32
		#include <afunix.h> //Unix sockets address (They renamed everything WHY)

		#define sockaddr_un SOCKADDR_UN
		#define sun_path Path
		#define sun_family Family
		#define sa_family_t ADDRESS_FAMILY
	#endif
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
		IPv6 = AF_INET6,
		#ifdef has_ax25
		ax25 = AF_AX25, //Maybe one day, when I can test it );
		#endif
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
	#ifdef has_ax25
	class ax25Address;
	#endif

	//generic address class	
	class address {
	protected:
		domain m_domain; //Domain of this address (each domain should have its own, specific, child-class)
		union {
			IPv4Address* IPv4;
			IPv6Address* IPv6;
			unixAddress* unix;
			#ifdef has_ax25
			ax25Address* ax25;
			#endif
			addressBase* base;
		} m_addresses;
	public:
		address(std::string addrstr, domain d = (domain)0);
		address(sockaddr_storage from, socklen_t len);
		address(const addressBase& addr); //Construct from any specific sub-type OR similar type (address copy-constructor uses this due to casting)
	
		operator sockaddr_storage() const;
		socklen_t size() const;
		explicit operator IPv4Address() const;
		explicit operator IPv6Address() const;
		explicit operator unixAddress() const;
		#ifdef has_ax25
		explicit operator ax25Address() const;
		#endif
		
		bool operator==(const address& r) const;
		bool operator!=(const address& r) const;
		
		domain addressDomain() const;
		std::string name() const;
	};

	//address base-class/interface
	class addressBase {
	protected:
		
		addressBase();
	public:
		virtual ~addressBase();

		virtual operator sockaddr_storage() const = 0;
		virtual socklen_t size() const = 0;
	
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
		socklen_t size() const; //Length associated with above (sockaddr_in cast)
		operator sockaddr_storage() const;
		
		bool operator==(const IPv4Address& r) const;
		bool operator!=(const IPv4Address& r) const;
	
		std::array<uint8_t, 4> addr() const;
		uint16_t port() const;
		std::string name() const;
	};
	
	class IPv6Address : public addressBase {
	protected:
		std::array<uint16_t, 8> m_addr; //128-bit address
		uint16_t m_port = 0;
		uint32_t m_flowInfo = 0;
		uint32_t m_scopeId = 0;
		std::string m_name;
	public:
		IPv6Address(uint16_t port = 0); //Construct an any address
		IPv6Address(const std::string addrstr); //Parse address from string
		IPv6Address(const sockaddr_in6 addr); //Construct from C struct
		operator sockaddr_in6() const; //Cast to C struct
		socklen_t size() const; //Length associated with above (sockaddr_in6 cast)
		operator sockaddr_storage() const;
		
		bool operator==(const IPv6Address& r) const;
		bool operator!=(const IPv6Address& r) const;
		
		std::array<uint16_t, 8> addr() const;
		std::array<uint16_t, 3> sitePrefix() const;
		uint16_t subnetId() const;
		std::array<uint16_t, 4> interfaceId() const;
		uint16_t port() const;
		uint32_t flowInfo() const;
		uint32_t scopeId() const;
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
		socklen_t size() const; //Length associated with above (sockaddr_un cast)
		operator sockaddr_storage() const;
		
		bool operator==(const unixAddress& r) const;
		bool operator!=(const unixAddress& r) const;
		
		std::string name() const;
		bool named() const;
	};
	
	//Planning for ax25 address, cannot develop/test due to limited support and little documentation
	#ifdef has_ax25
	class ax25Address : public addressBase {
	protected:
		std::array<uint8_t, 7> m_call;
		std::vector<std::array<uint8_t, 7>> m_digis; //digipeaters (if any)

		std::string m_name;
	public:
		ax25Address(const std::string addrstr); //Parse address from string
		ax25Address(const full_sockaddr_ax25 addr, const socklen_t len); //Construct from C struct
		operator full_sockaddr_ax25() const; //Cast to C struct
		socklen_t size() const; //Length associated with above (full_sockaddr_ax25 cast)
		operator sockaddr_storage() const;
		
		bool operator==(const ax25Address& r) const;
		bool operator!=(const ax25Address& r) const;
		
		std::string callsign() const;
		uint8_t ssid() const;
		std::string name() const;
	};
	#endif
};
