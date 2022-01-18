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
	
	{ "Addresses cast correctly (IPv4)",       { "3", "smoke" }, std::bind(AddressesMatchCEquivalents, std::placeholders::_1, sks::IPv4) },
	{ "Addresses cast correctly (IPv6)",       { "3", "smoke" }, std::bind(AddressesMatchCEquivalents, std::placeholders::_1, sks::IPv6) },
	{ "Addresses cast correctly (unix)",       { "3", "smoke" }, std::bind(AddressesMatchCEquivalents, std::placeholders::_1, sks::unix) },
	
	{ "Socket pairs can be created (stream)",  { "4" },          std::bind(SocketPairsCanBeCreated, std::placeholders::_1, sks::stream) },
	{ "Socket pairs can be created (dgram)",   { "4" },          std::bind(SocketPairsCanBeCreated, std::placeholders::_1, sks::dgram) },
	{ "Socket pairs can be created (seq)",     { "4" },          std::bind(SocketPairsCanBeCreated, std::placeholders::_1, sks::seq) },
};

//Execute the test (return 0 for pass)
testing::tag doTest(const testInfo& test) {
	testing::tag result = testing::pass;
	std::ostream* out = &std::cout; //Where to print the final evaluation of test (pass, fail, etc)
	
	std::stringstream fullLogStream;
	
	fullLogStream << "Starting test [ " << test.name << " ]" << std::endl;
	try {
		test.function(fullLogStream);
	} catch (const testing::tag& t) {
		switch (t) {
			case testing::tag::pass:
				break; //Convention is the function runs without throwing, but just in case
			case testing::tag::ignore:
				//Print to normal output the last line of fullLogStream
				{
					std::string line, lastLine;
					while (std::getline(fullLogStream, line)) {
						lastLine = line;
					}
					*out << lastLine << std::endl;
				}
				break;
			default:
				out = &std::cerr;
				*out << fullLogStream.str();
				break;
		}
		result = t;
	} catch (const std::exception& ex) {
		result = testing::exception;
		out = &std::cerr;
		*out << fullLogStream.str();
		
		*out << "Test threw an exception: " << ex.what() << std::endl;
	} catch (...) {
		//Literally a catch-all
		//We don't know the type thrown, nor anything about it
		//Shouldn't usually happen, but this is for testing so who knows what will happen, thats OUR job to check!
		result = testing::exception;
		out = &std::cerr;
		*out << fullLogStream.str();
		*out << "Test threw a type which is not an std::exception." << std::endl;
	}
	*out << result << "\t" << test.name << std::endl;
	
	//return result == testing::pass ? 0 : 1;
	return result;
}

int main(int argc, char** argv) {
	std::map<size_t, testing::tag> testsToRun; //Keys are the index of the test in the lookup table `tests`, values are their execution result
	
	//Build set of tests to run
	if (argc < 2) {
		//Run all tests
		for (size_t i = 0; i < tests.size(); i++) {
			testsToRun[i] = testing::tag::skip;
		}
	} else {
		//Run only certain tests/groups (do regex searching through all tests for each argument)
		for (size_t argIndex = 1; argIndex < (size_t)argc; argIndex++) {
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
						testsToRun[i] = testing::tag::skip;
						break; //Test is already added, we don't need to keep checking it
					}
				}
			}
		}
	}
	
	//Execute all selected tests
	std::map<testing::tag, size_t> tagTotals;
	for (std::pair<const size_t, testing::tag>& testResultPair : testsToRun) {
		size_t testIndex = testResultPair.first;
		testing::tag& executionResult = testResultPair.second;
		
		executionResult = doTest(tests[testIndex]);
		tagTotals[executionResult]++;
	}
	
	//Return number of tests which didn't pass or get ignored
	return testsToRun.size() - (tagTotals[testing::tag::pass] + tagTotals[testing::tag::ignore]);
}
