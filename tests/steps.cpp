#include "steps.hpp"
#include "testing.hpp"
#include <socks/socks.hpp>
#include <socks/addrs.hpp>
#include "utility.hpp"
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
	log << "Checking system for *any* " << d << " support" << std::endl;
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
		default:
			throw std::runtime_error("Unknown domain");
	}
	
	return GivenTheSystemSupports(log, d, type);
}
void GivenTheSystemSupports(std::ostream& log, sks::domain d, sks::type t) {
	log << "Checking system for " << t << " support in the " << d << " domain" << std::endl;
	try {
		sks::socket sock(d, t);
		log << "System supports " << d << " " << t << "s" << std::endl;
	} catch (std::system_error& se) {
		if (se.code() == std::make_error_code(std::errc::address_family_not_supported) ||
		    se.code() == std::make_error_code(std::errc::invalid_argument)) {
			//log << t << " in the " << d << " is not supported on this system (" << se.what() << ")" << std::endl;
			log << t << " in the " << d << " is required for this test." << std::endl;
			throw testing::ignore;
		} else {
			log << "Could not check for socket support" << std::endl;
			throw se;
		}
	} catch (std::exception& e) {
		log << "Could not check for socket support (Non-system error" << std::endl;
		throw e;
	}
}
