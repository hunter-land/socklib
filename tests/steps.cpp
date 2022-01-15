#include "steps.hpp"
#include "testing.hpp"
#include <socks/socks.hpp>
#include <socks/addrs.hpp>
#include <ostream>

sks::address bindableAddress(sks::domain d) {
	switch (d) {
		case sks::IPv4:
			return sks::address("127.0.0.1:0", d);
		case sks::IPv6:
			return sks::address("[::1]:0", d);
		case sks::unix:
			return sks::address("testing.unix", d);
	}
	throw std::runtime_error("No bindable address for domain.");
}

void GivenTheSystemSupports(std::ostream& log, sks::domain d) {
	//We know it is supported if we can create a socket in that domain
	sks::type type;
	switch (d) {
		case sks::IPv4:
		case sks::IPv6:
		case sks::unix:
			type = sks::stream;
			break;
		//case sks::ax25:
		//	type = sks::seq;
		//	break;
	}
	
	return GivenTheSystemSupports(log, d, type);
}
void GivenTheSystemSupports(std::ostream& log, sks::domain d, sks::type t) {
	try {
		sks::socket sock(d, t, 0);
	} catch (std::system_error& se) {
		if (se.code() == std::make_error_code(std::errc::address_family_not_supported)) {
			log << "Domain is not supported on this system" << std::endl;
			throw testing::ignore;
		}
	}
}

sks::socket WhenICreateTheSocket(std::ostream& log, sks::domain d, sks::type t, int protocol) {
	//Create and return socket
	try {
		
		sks::socket sock(d, t, 0);
		return sock;
		
	} catch (std::exception& e) {
		//Step failed, log it and report failure
		log << "Could not construct a " << t << " socket in " << d << " domain: " << e.what() << std::endl;
		throw testing::fail;
	}
}
