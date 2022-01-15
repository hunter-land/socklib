#include <socks/socks.hpp>
#include <socks/addrs.hpp>
#include <string>
#include <ostream>

std::string typeString(sks::type t);
std::ostream& operator<<(std::ostream& os, const sks::type t);

std::string domainString(sks::domain d);
std::ostream& operator<<(std::ostream& os, const sks::domain d);

//Convert "any"s to "loopback"s where applicable (0.0.0.0 -> 127.0.0.1, etc)
sks::address anyToLoop(sks::address addr);
