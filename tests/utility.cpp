#include <socks/socks.hpp>
#include <socks/addrs.hpp>
#include <string>
#include <ostream>

std::string typeString(sks::type t) {
	switch (t) {
		case sks::stream:
			return "stream";
		case sks::dgram:
			return "dgram";
		case sks::seq:
			return "seq";
		case sks::rdm:
			return "rdm";
		case sks::raw:
			return "raw";
		default:
			return "BAD TYPE";
	}
}
std::ostream& operator<<(std::ostream& os, const sks::type t) {
	os << typeString(t);
}

std::string domainString(sks::domain d) {
	switch (d) {
		case sks::IPv4:
			return "IPv4";
		case sks::IPv6:
			return "IPv6";
		case sks::unix:
			return "unix";
		default:
			return "BAD DOMAIN";
	}
}
std::ostream& operator<<(std::ostream& os, const sks::domain d) {
	os << domainString(d);
}

//Convert "any"s to "loopback"s where applicable (0.0.0.0 -> 127.0.0.1, etc)
sks::address anyToLoop(sks::address addr) {
	std::array<uint16_t, 16> zeroed; //Zero'd data big enough for largest address comparison
	zeroed.fill(0);
	switch (addr.addressDomain()) {
		case sks::IPv4:
			{
				sks::IPv4Address a = (sks::IPv4Address)addr;
				if (memcmp(a.addr().data(), zeroed.data(), a.addr().size()) == 0) {
					//Is an any addr
					return sks::IPv4Address("127.0.0.1:" + std::to_string(a.port()));
				} else {
					//Is not "any" addr, return unchanged
					return addr;
				}
			}
			break;
		case sks::IPv6:
			{
				sks::IPv6Address a = (sks::IPv6Address)addr;
				if (memcmp(a.addr().data(), zeroed.data(), a.addr().size()) == 0) {
					//Is an any addr
					return sks::IPv6Address("[::]:" + std::to_string(a.port()));
				} else {
					//Is not "any" addr, return unchanged
					return addr;
				}
			}
			break;
		default:
			//No any/loopback here
			return addr;
	}
}
