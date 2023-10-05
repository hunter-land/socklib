#pragma once
#include <ostream>
#include "socks.hpp"

void givenSystemSupports(std::ostream& log, sks::domain d, sks::type t);

std::pair<sks::socket, sks::socket> getRelatedSockets(std::ostream& log, sks::domain d, sks::type t);
