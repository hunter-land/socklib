#include <iostream>
#include "socks.hpp"
#include <btf/driver.hpp>
#include "tests.cpp"

int main(int argc, char** argv) {

	//Pre-test cleanup
	//TODO: If needed/Check

	//Set tests to vector
	btf::allTests = {
		{"Sockets can communicate (IPv4 stream)",      { "smoke" },      std::bind(socketsSendAndReceive, std::placeholders::_1, sks::IPv4, sks::stream)},
		{"Sockets can communicate (IPv4 TCP)",         { "2", "smoke" }, std::bind(socketsSendAndReceive, std::placeholders::_1, sks::IPv4, sks::stream)},
		{"Sockets can communicate (IPv4 UDP)",         { "2", "smoke" }, std::bind(socketsSendAndReceive, std::placeholders::_1, sks::IPv4, sks::dgram)},
		{"Sockets can communicate (IPv4 SEQ)",         { "2" },          std::bind(socketsSendAndReceive, std::placeholders::_1, sks::IPv4, sks::seq)},
		{"Sockets can communicate (IPv6 TCP)",         { "2", "smoke" }, std::bind(socketsSendAndReceive, std::placeholders::_1, sks::IPv6, sks::stream)},
		{"Sockets can communicate (IPv6 UDP)",         { "2", "smoke" }, std::bind(socketsSendAndReceive, std::placeholders::_1, sks::IPv6, sks::dgram)},
		{"Sockets can communicate (IPv6 SEQ)",         { "2" },          std::bind(socketsSendAndReceive, std::placeholders::_1, sks::IPv6, sks::seq)},
		{"Sockets can communicate (unix stream)",      { "2", "smoke" }, std::bind(socketsSendAndReceive, std::placeholders::_1, sks::unix, sks::stream)},

		{"Addresses cast correctly (IPv4)",            { "3", "smoke" }, std::bind(addressesMatchCEquivalents, std::placeholders::_1, sks::IPv4)},
		{"Addresses cast correctly (IPv6)",            { "3", "smoke" }, std::bind(addressesMatchCEquivalents, std::placeholders::_1, sks::IPv6)},
		{"Addresses cast correctly (unix)",            { "3", "smoke" }, std::bind(addressesMatchCEquivalents, std::placeholders::_1, sks::unix)},
	#ifdef has_ax25
		{"Addresses cast correctly (ax25)",            { "3", "smoke" }, std::bind(addressesMatchCEquivalents, std::placeholders::_1, sks::ax25)},
	#endif

		{"Socket pairs can be created (stream)",       { "4" },          std::bind(socketPairsCanBeCreated, std::placeholders::_1, sks::stream)},
		{"Socket pairs can be created (dgram)",        { "4" },          std::bind(socketPairsCanBeCreated, std::placeholders::_1, sks::dgram)},
		{"Socket pairs can be created (seq)",          { "4" },          std::bind(socketPairsCanBeCreated, std::placeholders::_1, sks::seq)},

		{"readReady() times-out correctly (IPv4 TCP)", { "5" },          std::bind(readReadyTimesOutCorrectly, std::placeholders::_1, sks::IPv4, sks::stream)},
		{"readReady() times-out correctly (IPv4 UDP)", { "5" },          std::bind(readReadyTimesOutCorrectly, std::placeholders::_1, sks::IPv4, sks::dgram)},
		{"readReady() times-out correctly (IPv4 SEQ)", { "5" },          std::bind(readReadyTimesOutCorrectly, std::placeholders::_1, sks::IPv4, sks::seq)},
		{"readReady() times-out correctly (IPv6 TCP)", { "5" },          std::bind(readReadyTimesOutCorrectly, std::placeholders::_1, sks::IPv6, sks::stream)},
		{"readReady() times-out correctly (IPv6 UDP)", { "5" },          std::bind(readReadyTimesOutCorrectly, std::placeholders::_1, sks::IPv6, sks::dgram)},
		{"readReady() times-out correctly (IPv6 SEQ)", { "5" },          std::bind(readReadyTimesOutCorrectly, std::placeholders::_1, sks::IPv6, sks::seq)},
		{"readReady() times-out correctly (unix TCP)", { "5" },          std::bind(readReadyTimesOutCorrectly, std::placeholders::_1, sks::unix, sks::stream)},
		{"readReady() times-out correctly (unix UDP)", { "5" },          std::bind(readReadyTimesOutCorrectly, std::placeholders::_1, sks::unix, sks::dgram)},
		{"readReady() times-out correctly (unix SEQ)", { "5" },          std::bind(readReadyTimesOutCorrectly, std::placeholders::_1, sks::unix, sks::seq)}
	};

	//Print info before run starts
	btf::preRun = [](std::vector<btf::test> testsToRun, size_t threadCount) -> void{
		std::cout << "Testing socklib " << sks::version.major << "." << sks::version.minor << "." << sks::version.build << std::endl;
		std::cout << std::endl;
	};

	//Run tests based on arguments
	int failCount = btf::basicMain(argc, argv);
	return failCount;
}
