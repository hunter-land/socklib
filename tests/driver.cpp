#include "basic.cpp" //All basic tests (source not header)
#include <vector>
#include <functional>
#include <iostream>
#include "testing.hpp"
#include <set>
#include <regex>
#include <sstream>

//Information of a single test
struct testInfo {
	std::string name; //Human-readable name of the test
	std::vector<std::string> identifiers; //identifiers (UID, group name(s), etc)
	std::function<void(std::ostream&)> function; //the actual test function which will be ran
};

//All tests (global look-up table)
const std::vector<testInfo> tests = {
	//{ "Sockets can be created",                { "1", "smoke" }, SocketsCanBeCreated },
	
	{ "Sockets can communicate (IPv4 TCP)",    { "2", "smoke" }, std::bind(SocketsCanCommunicate, std::placeholders::_1, sks::IPv4, sks::stream) },
	{ "Sockets can communicate (IPv4 UDP)",    { "2", "smoke" }, std::bind(SocketsCanCommunicate, std::placeholders::_1, sks::IPv4, sks::dgram) },
	{ "Sockets can communicate (IPv4 SEQ)",    { "2" },          std::bind(SocketsCanCommunicate, std::placeholders::_1, sks::IPv4, sks::seq) },
	{ "Sockets can communicate (IPv6 TCP)",    { "2", "smoke" }, std::bind(SocketsCanCommunicate, std::placeholders::_1, sks::IPv6, sks::stream) },
	{ "Sockets can communicate (IPv6 UDP)",    { "2", "smoke" }, std::bind(SocketsCanCommunicate, std::placeholders::_1, sks::IPv6, sks::dgram) },
	{ "Sockets can communicate (IPv6 SEQ)",    { "2" },          std::bind(SocketsCanCommunicate, std::placeholders::_1, sks::IPv6, sks::seq) },
	{ "Sockets can communicate (unix stream)", { "2", "smoke" }, std::bind(SocketsCanCommunicate, std::placeholders::_1, sks::unix, sks::stream) },
	
	{ "Addresses cast correctly (IPv4)", { "3", "smoke" }, std::bind(AddressesMatchCEquivalents, std::placeholders::_1, sks::IPv4) },
	{ "Addresses cast correctly (IPv6)", { "3", "smoke" }, std::bind(AddressesMatchCEquivalents, std::placeholders::_1, sks::IPv6) },
	{ "Addresses cast correctly (unix)", { "3", "smoke" }, std::bind(AddressesMatchCEquivalents, std::placeholders::_1, sks::unix) },
};

//Execute the test (return 0 for pass)
int doTest(const testInfo& test) {
	testing::tag result = testing::pass;
	std::ostream* out = &std::cout; //Where to print the final evaluation of test (pass, fail, etc)
	
	std::ostringstream fullLogStream;
	
	fullLogStream << "Starting test [ " << test.name << " ]" << std::endl;
	try {
		test.function(fullLogStream);
	} catch (testing::tag& t) {
		result = t;
		out = &std::cerr;
		*out << fullLogStream.str();
	} catch (std::exception& ex) {
		result = testing::exception;
		out = &std::cerr;
		*out << fullLogStream.str();
		
		*out << ex.what() << std::endl;
	}
	*out << result << "\t" << test.name << std::endl;
	
	return result == testing::pass ? 0 : 1;
}

int main (int argc, char** argv) {
	std::set<size_t> testsToRun; //Elements are the index of the test in the lookup table `tests`
	
	//Build set of tests to run
	if (argc < 2) {
		//Run all tests
		for (size_t i = 0; i < tests.size(); i++) {
			testsToRun.insert(i);
		}
	} else {
		//Run only certain tests/groups (do regex searching through all tests for each argument)
		for (size_t argIndex = 1; argIndex < argc; argIndex++) {
			std::string searchPattern(argv[argIndex]);
			std::regex searchRegex(searchPattern);
			
			//For each test
			for (size_t i = 0; i < tests.size(); i++) {
				const testInfo& test = tests[i];
				//For each tag of the test
				for (const std::string& identifier : test.identifiers) {
					//If the tag matches
					if (std::regex_match(identifier, searchRegex)) {
						//add the test to list to execute
						testsToRun.insert(i);
						break; //Test is already added, we don't need to keep checking it
					}
				}
			}
		}
	}
	
	//Any setup required
	//(ha! none today)
	
	//Execute tests
	int failedTests = 0;
	for (const size_t& testIndex : testsToRun) {
		failedTests += doTest(tests[testIndex]);
	}
	
	//Any post test actions
	//(also none today)
	
	return failedTests;
}
