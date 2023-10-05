#include "utility.hpp"
#include "socks.hpp"
#include <string>

std::string str(sks::domain d) {
	switch (d) {
		case sks::IPv4:
			return "IPv4";
		case sks::IPv6:
			return "IPv6";
		case sks::unix:
			return "unix";
		default:
			throw std::logic_error("UNKNOWN (" + std::to_string(d) + ")");
	}
}

std::string str(sks::type t) {
	switch (t) {
		case sks::stream:
			return "stream";
		case sks::dgram:
			return "datagram";
		case sks::seq:
			return "sequenced datagram";
		case sks::rdm:
			return "reliable datagram";
		case sks::raw:
			return "raw";
		default:
			throw std::logic_error("UNKNOWN TYPE (" + std::to_string(t) + ")");
	}
}

sks::address bindableAddress(sks::domain d, uint8_t index) {
	switch (d) {
		case sks::IPv4:
			return sks::address("127.0.0.1:0", d);
		case sks::IPv6:
			return sks::address("[::1]:0", d);
		case sks::unix:
			{
				std::string unixAddressPath = "testing." + std::to_string(index) + ".unix";
				return sks::address(unixAddressPath, d);
			}
	}
	throw std::runtime_error("No bindable address for domain.");
}
