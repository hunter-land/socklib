#pragma once
#include <ostream>
#include "socks.hpp"

void assertSystemSupports(std::ostream& log, sks::domain d, sks::type t);
void givenSystemSupports(std::ostream& log, sks::domain d, sks::type t);

std::pair<sks::socket, sks::socket> getRelatedSockets(std::ostream& log, sks::domain d, sks::type t);
