#pragma once
#include <socks/socks.hpp>
#include <socks/addrs.hpp>
#include <ostream>

/**	
 *	Contains steps used by testing
 */

//Get an address which can be bound to by domain
sks::address bindableAddress(sks::domain d, uint8_t index = 0);

//"Given the system supports (.*)"
void GivenTheSystemSupports(std::ostream& log, sks::domain d);
//"Given the system supports (.*) in the (.*) domain"
void GivenTheSystemSupports(std::ostream& log, sks::domain d, sks::type t);

struct socketAndAddress {
	sks::address addr = sks::address("", sks::unix);
	sks::socket socket = sks::socket(sks::unix, sks::stream);
};

//Get two sockets which are ready to communicate with each other (connected for stream and seq types, bound for others)
std::pair<socketAndAddress, socketAndAddress> GetRelatedSockets(std::ostream& log, sks::domain d, sks::type t);
