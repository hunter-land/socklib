#include "utility.hpp"
#include "socks.hpp"
#include <string>
#include <sstream>
#include <thread>

std::string str(const sks::domain& d) {
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

std::string str(const sks::type& t) {
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

std::string str(const std::chrono::milliseconds& ms) {
	return std::to_string(ms.count()) + "ms";
}
std::string str(const std::chrono::system_clock::duration& scd) {
	return std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(scd).count()) + "ns";
}

std::string str(const std::errc& ec) {
	std::system_error se(std::make_error_code(ec));
	return std::to_string((std::underlying_type<std::errc>::type)ec) + " - " + se.what();
}

sks::address bindableAddress(sks::domain d, uint8_t index) {
	switch (d) {
		case sks::IPv4:
			return sks::address("127.0.0.1:0", d);
		case sks::IPv6:
			return sks::address("[::1]:0", d);
		case sks::unix:
			{
				std::stringstream ss;
				ss << std::this_thread::get_id();
				std::string unixAddressPath = "testing." + ss.str() + std::to_string(index) + ".unix";
				return sks::address(unixAddressPath, d);
			}
	}
	throw std::runtime_error("No bindable address for domain.");
}
