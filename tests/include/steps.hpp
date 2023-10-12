#pragma once
#include <ostream>
#include "socks.hpp"

void assertSystemSupports(std::ostream& log, sks::domain d, sks::type t);

std::pair<sks::socket, sks::socket> getRelatedSockets(std::ostream& log, sks::domain d, sks::type t);

void socketCanSendDataToSocket(std::ostream& log, sks::socket& sender, sks::socket& receiver, const std::string& message, sks::type t);
