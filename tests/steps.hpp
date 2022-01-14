#pragma once
#include <socks/socks.hpp>
#include <socks/addrs.hpp>
#include <ostream>

/**	
 *	Contains steps used by testing
 */

//Get an address which can be bound to by domain
sks::address bindableAddress(sks::domain d);

//"Given the system supports (.*)"
bool GivenTheSystemSupports(std::ostream& log, sks::domain d);

//"When I create the socket"
sks::socket WhenICreateTheSocket(std::ostream& log, sks::domain d, sks::type t, int protocol = 0);
