#include <iostream>
#include "socks.hpp"
#include <btf/driver.hpp>
#include <btf/testPermutations.hpp>
#include "tests.cpp"
#include <filesystem>

int main(int argc, char** argv) {

	btf::typeMatrix.valuesForType<sks::domain>() = {
		sks::IPv4,
		sks::IPv6,
		sks::unix,
		#ifdef has_ax25
		sks::ax25,
		#endif
	};
	btf::typeMatrix.valuesForType<sks::type>() = {
		sks::stream,
		sks::dgram,
		sks::seq,
	};

	btf::addTestPermutations("Sockets can communicate",         {"2", "smoke"}, socketsSendAndReceive);
	btf::addTestPermutations("Addresses cast correctly",        {"3", "smoke"}, addressesMatchCEquivalents);
	btf::addTestPermutations("Socket pairs can be created",     {"4"},          socketPairsCanBeCreated);
	btf::addTestPermutations("readReady() times-out correctly", {"5"},          readReadyTimesOutCorrectly);

	//Print info before run starts
	btf::preRun = [](std::vector<btf::test> testsToRun, size_t threadCount) -> void{
		std::cout << "Testing socklib " << sks::version.major << "." << sks::version.minor << "." << sks::version.build << std::endl;
		std::cout << std::endl;
	};

	btf::postTest = [](std::ostream& log, btf::test t, btf::testResult r) -> void{
		//Remove any old socket files (unix sockets)
		for (size_t i = 0; i < 2; i++) {
			std::string name = bindableAddress(sks::unix, i).name();
			bool exists = std::filesystem::exists(name);
			if (exists) {
				log << "Deleting old unix socket \"" << name << "\"" << std::endl;
				std::filesystem::remove(name);
			}
		}
	};

	//Run tests based on arguments
	int failCount = btf::basicMain(argc, argv);
	return failCount;
}
