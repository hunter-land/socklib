#pragma once
#include <array>
#include <string>
#include <vector>
#include "macros.hpp"
extern "C" {
	#ifdef __SKS_AS_POSIX__
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <sys/un.h>
		#ifdef __SKS_HAS_AX25__
			#include <netax25/ax25.h>
		#endif
	#elif defined __SKS_AS_WINDOWS__
		#include <ws2tcpip.h> //WinSock 2 and socklen_t
		#define UNIX_PATH_MAX 180
		typedef struct sockaddr_un {
			ADDRESS_FAMILY sun_family; /* AF_UNIX */
			char sun_path[UNIX_PATH_MAX]; /*  */
		} SOCKADDR_UN, *PSOCKADDR_UN;
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
		#ifdef __SKS_HAS_AX25__
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
	#ifdef __SKS_HAS_AX25__
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
			#ifdef __SKS_HAS_AX25__
			ax25Address* ax25;
			#endif
			addressBase* base = nullptr;
		} m_addresses;
	public:
		address();
		address(const std::string& addrstr, domain d = (domain)0);
		address(const sockaddr_storage& from, socklen_t len);
		address(const addressBase& addr); //Construct from any specific sub-type OR similar type
		address(const address& addr);
		address(address&& addr);
		~address();

		address& operator=(const address& addr);
		address& operator=(address&& addr);
		void assign(const sockaddr_storage& from, socklen_t len);

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
		bool operator<(const address& r) const;
		
		domain addressDomain() const;
		std::string name() const;
	};

	//address base-class/interface
	class addressBase {
	protected:
		addressBase();
	public:
		addressBase(const addressBase& r);
		addressBase(addressBase&& r);
		virtual ~addressBase();

		addressBase& operator=(const addressBase& r);

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
		IPv4Address(const std::string& addrstr); //Parse address from string
		IPv4Address(const sockaddr_in& addr); //Construct from C struct
		operator sockaddr_in() const; //Cast to C struct
		socklen_t size() const override; //Length associated with above (sockaddr_in cast)
		operator sockaddr_storage() const override;
		
		bool operator==(const IPv4Address& r) const;
		bool operator!=(const IPv4Address& r) const;
		bool operator<(const IPv4Address& r) const;
	
		std::array<uint8_t, 4> addr() const;
		uint16_t port() const;
		std::string name() const override;
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
		IPv6Address(const std::string& addrstr); //Parse address from string
		IPv6Address(const sockaddr_in6& addr); //Construct from C struct
		operator sockaddr_in6() const; //Cast to C struct
		socklen_t size() const override; //Length associated with above (sockaddr_in6 cast)
		operator sockaddr_storage() const override;
		
		bool operator==(const IPv6Address& r) const;
		bool operator!=(const IPv6Address& r) const;
		bool operator<(const IPv6Address& r) const;
		
		std::array<uint16_t, 8> addr() const;
		std::array<uint16_t, 3> sitePrefix() const;
		uint16_t subnetId() const;
		std::array<uint16_t, 4> interfaceId() const;
		uint16_t port() const;
		uint32_t flowInfo() const;
		uint32_t scopeId() const;
		std::string name() const override;
	};
	
	class unixAddress : public addressBase {
	protected:
		//null-terminated path name
		//unnamed (size == 0)
		//abstract (m_addr[0] == '\0')
		std::vector<char> m_addr;
	public:
		unixAddress(const std::string& addrstr); //Parse address from string
		unixAddress(const sockaddr_un& addr, const socklen_t len); //Construct from C struct
		operator sockaddr_un() const; //Cast to C struct
		socklen_t size() const override; //Length associated with above (sockaddr_un cast)
		operator sockaddr_storage() const override;
		
		bool operator==(const unixAddress& r) const;
		bool operator!=(const unixAddress& r) const;
		bool operator<(const unixAddress& r) const;
		
		std::string name() const override;
		bool named() const;
	};
	
	//Planning for ax25 address, cannot develop/test due to limited support and little documentation
	#ifdef __SKS_HAS_AX25__
	typedef std::array<uint8_t, sizeof(ax25_address)> ax25Call;
	class ax25Address : public addressBase {
	protected:
		ax25Call m_call;
		std::vector<ax25Call> m_digis; //digipeaters (if any)

		std::string m_name; //DEST [via (digitpeater)+] (YOUR first hop should be first digipeater, peer's first hop should be last digipeater)
	public:
		ax25Address(const std::string& addrstr); //Parse address from string
		ax25Address(const full_sockaddr_ax25& addr, const socklen_t len); //Construct from C struct
		operator full_sockaddr_ax25() const; //Cast to C struct
		socklen_t size() const override; //Length associated with above (full_sockaddr_ax25 cast)
		operator sockaddr_storage() const override;
		
		bool operator==(const ax25Address& r) const;
		bool operator!=(const ax25Address& r) const;
		bool operator<(const ax25Address& r) const;
		
		std::string callsign() const;
		uint8_t ssid() const;
		std::string name() const override;
	};
	#endif
};
