#pragma once
#include "socks.hpp"
#include <string>
#include <chrono>

std::string str(const sks::domain& d);
std::string str(const sks::type& t);
std::string str(const std::chrono::milliseconds& ms);
std::string str(const std::chrono::system_clock::duration& scd);
std::string str(const std::errc& ec);

sks::address bindableAddress(sks::domain d, uint8_t index = 0);
const std::vector<sks::address> addresses = { //These addresses are not intended to be used by any sockets. They are for testing address classes
	//Blank address
	sks::address(), //Blank

	//IPv4
	sks::address("0.0.0.0:0"), //Any:Any
	sks::address("10.0.0.1:0"), //Specific:Any
	sks::address("0.0.0.0:2560"), //Any:Specific
	sks::address("10.0.0.1:2560"), //Specific:Specific

	//IPv6
	sks::address("[::]:0"), //Any:Any
	sks::address("[3691:b9dd:a018:cf26:1f5e:3781:732d:69ca]:0"), //SpecificExplicit:Any
	sks::address("[3691::69ca]:0"), //SpecificImplicit:Any
	sks::address("[::]:5120"), //Any:Specific
	sks::address("[3691:b9dd:a018:cf26:1f5e:3781:732d:69ca]:5120"), //SpecificExplicit:Specific
	sks::address("[3691::69ca]:5120"), //SpecificImplicit:Specific

	//unix
	sks::address("address.unix", sks::unix),
	sks::address("../address.unix", sks::unix),
	sks::address("address.unix.unix", sks::unix),
};
