#pragma once
#include "socks.hpp"
#include <string>

std::string str(sks::domain d);
std::string str(sks::type t);

sks::address bindableAddress(sks::domain d, uint8_t index = 0);
