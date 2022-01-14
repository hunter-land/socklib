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
	bool supported = false;
	try {
		sks::socket sock(d, sks::raw); //Raw because we don't know what types are/aren't supported
		supported = true;
	} catch (std::exception& e) {
		log << "Domain is not supported on this system" << std::endl;
	}
	
	if (!supported) {
		throw testing::ignore;
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
