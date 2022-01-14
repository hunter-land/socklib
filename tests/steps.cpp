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
			return sks::address("[::]:0", d);
		case sks::unix:
			return sks::address("testing.unix", d);
	}
	throw std::runtime_error("No bindable address for domain.");
}

bool GivenTheSystemSupports(std::ostream& log, sks::domain d) {
	//TODO: This...
	
	//If supported return true
	//else throw ignore
	//throw testing::ignore;
	return true;
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
