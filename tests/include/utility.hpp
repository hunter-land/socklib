#pragma once
#include "socks.hpp"
#include <string>
#include <chrono>

std::string str(const sks::domain& d);
std::string str(const sks::type& t);
std::string str(const std::chrono::milliseconds& ms);

sks::address bindableAddress(sks::domain d, uint8_t index = 0);
