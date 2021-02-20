#include "../socks.hpp"
#include <map>
#include <string>
#include <thread>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <fstream>
extern "C" {
	#ifndef _WIN32	
		#include <unistd.h> //ioctl
	#else
		#define UNLINK(x) DeleteFile(x)
	#endif
}

static std::map<std::string, bool> report;
//Styles [0 - 27]
//Colors [30-37] (+0 for text, +10 for background)
enum fmt {
	reset = 0,
	bold = 1,
	underline = 4,
	inversecolor = 7,
	nobold = 21,
	nounderline = 24,
	noinverse = 27,
	black = 30,
	red = 31,
	green = 32,
	yellow = 33,
	blue = 34,
	magenta = 35,
	cyan = 36,
	white = 37
};
std::string format(fmt f) {
	std::string s = "\033[";
	s += std::to_string((int)f);
	s += "m";
	return s;
}
static std::string tagfail("[\033[31mFAIL\033[0m]");
static std::string tagnote("[\033[36mNOTE\033[0m]");
static std::string tagok("[\033[32m OK \033[0m]");
static std::stringstream verifystream;
//static std::ofstream verifystream("./output.txt");
//#define verifystream std::cerr
static void resetverifystream() {
	verifystream = std::stringstream(std::string());
}
static std::string verifystreamstring() {
	//return "";
	return verifystream.str();
}
static std::mutex outmutex;
template<typename T>
static std::string vecstring(std::vector<T> v, size_t endsize = 2) { //a b ... y z
	std::string s = "{ ";
	
	size_t i = 0;
	for (; i < v.size() && i < endsize; i++) {
		s += std::to_string(v[i]);
		s += " ";
	}
	
	size_t j = - endsize + v.size();
	if (j - 1 > i) {
		s += "... ";
	}
	if (j > i) {
		for (; j < v.size(); j++) {
			s += std::to_string(v[j]);
			s += " ";
		}
	}
	
	s += "}.size() = ";
	s += std::to_string(v.size());
	
	return s;
}
template<typename T>
static int verify(std::string name, T actual, T expected) {
	bool match = actual == expected;
	
	report[name] = match;
	outmutex.lock();
	if (match) {
		verifystream << tagok << "\t" << name << std::endl;
	} else {
		verifystream << tagfail << "\t" << name << std::endl;
		verifystream << "\tExpected " << format(yellow) << expected << format(reset) << std::endl;
		verifystream << "\tGot " << format(red) << actual << format(reset) << std::endl;
	}
	outmutex.unlock();
	
	return match ? 0 : 1;
}
static int verify(std::string name, sks::serror actual, sks::serror expected) {
	bool match = (actual.type == expected.type && actual.erno == expected.erno) || (actual.erno == 0 && expected.erno == 0);
	
	report[name] = match;
	outmutex.lock();
	if (match) {
		verifystream << tagok << "\t" << name << std::endl;
	} else {
		verifystream << tagfail << "\t" << name << std::endl;
		verifystream << "\tExpected " << format(yellow) << sks::errorstr(expected) << format(reset) << std::endl;
		verifystream << "\tGot " << format(red) << sks::errorstr(actual) << format(reset) << std::endl;
	}
	outmutex.unlock();
	
	return match ? 0 : 1;
}
static int verify(std::string name, std::vector<uint8_t> actual, std::vector<uint8_t> expected) {
	bool match = actual == expected;
	
	report[name] = match;
	outmutex.lock();
	if (match) {
		verifystream << tagok << "\t" << name << std::endl;
	} else {
		verifystream << tagfail << "\t" << name << std::endl;
		verifystream << "\tExpected " << format(yellow) << vecstring(expected) << format(reset) << std::endl;
		verifystream << "\tGot " << format(red) << vecstring(actual) << format(reset) << std::endl;
	}
	outmutex.unlock();
	
	return match ? 0 : 1;
}
static int verify(std::string name, int actual, int expected, bool areerrnos = true) {
	bool match = actual == expected;
	
	report[name] = match;
	outmutex.lock();
	if (match) {
		verifystream << tagok << "\t" << name << std::endl;
	} else {
		verifystream << tagfail << "\t" << name << std::endl;
		if (areerrnos) {
			verifystream << "\tExpected " << format(yellow) << sks::errorstr(expected) << format(reset) << std::endl;
			verifystream << "\tGot " << format(red) << sks::errorstr(actual) << format(reset) << std::endl;
		} else {
			verifystream << "\tExpected " << format(yellow) << expected << format(reset) << std::endl;
			verifystream << "\tGot " << format(red) << actual << format(reset) << std::endl;
		}
	}
	outmutex.unlock();
	
	return match ? 0 : 1;
}
static int verify(std::string name, const std::runtime_error& actual, const std::runtime_error& expected) {
	bool match = actual.what() == expected.what();
	
	report[name] = match;
	outmutex.lock();
	if (match) {
		verifystream << tagok << "\t" << name << std::endl;
	} else {
		verifystream << tagfail << "\t" << name << std::endl;
		verifystream << "\tExpected " << format(yellow) << expected.what() << format(reset) << std::endl;
		verifystream << "\tGot " << format(red) << actual.what() << format(reset) << std::endl;
	}
	outmutex.unlock();
	
	return match ? 0 : 1;
}
static const sks::serror snoerr = { sks::BSD, 0 };
static const std::runtime_error norte("NONE");

static std::vector<uint8_t> allBytes;
static std::vector<uint8_t> allBytesReversed;
static const std::vector<sks::domain> allDomains = { sks::unix, sks::ipv4, sks::ipv6 };
static sks::sockaddress host;
static int hostvalid = 0; //Invalid = 0, valid = 1, will never be valid (skip) = 2 (For connection-based)
static int sockready = 0; //How many sockets are ready (For connectionless)

template <class BidirectionalIterator>
  void reverse (BidirectionalIterator first, BidirectionalIterator last)
{
  while ((first!=last)&&(first!=--last)) {
    std::iter_swap (first,last);
    ++first;
  }
}

size_t allUnitTests();
/*
 *	            #1                                #2            
 *	           Host                             Client          
 *	/------------------------\        /------------------------\
 *	|      Construct h       |        |      Construct c       |
 *	|                        |        |                        |
 *	|         Listen         |        |                        |
 *	|                        |        |                        |
 *	|       hc = Accept      |~~~~~~~~|        Connect         |
 *	|                        |        |                        |
 *	|        d = Recv        |<-------|          Send          |
 *	|                        |        |   Bytes of [0, 256)    |
 *	|       reverse d        |        |                        |
 *	|                        |        |                        |
 *	|          Send          |------->|        r = Recv        |
 *	|       reversed d       |        |                        |
 *	|                        |        |         Verify         |
 *	|                        |        |                        |
 *	|     Recv (timeout)     |~~~~~~~~|         Close          |
 *	|    (Error expected)    |        |                        |
 *	|                        |        |                        |
 *	|      Verify error      |        |                        |
 *	|                        |        |                        |
 *	\----------Join----------/        \----------Join----------/
 */
int tcp_proper();
void tcp_proper_t1(int&, sks::domain);
void tcp_proper_t2(int&, sks::domain);
/*
 *	            #1                                #2            
 *	          "Echo"                            Sender          
 *	/------------------------\        /------------------------\
 *	|                        |        |                        |
 *	|      Construct b       |        |      Construct a       |
 *	|                        |        |                        |
 *	|       Show ready       |        |       Show ready       |
 *	|                        |        |                        |
 *	|        d = Recv        |<-------|          Send          |
 *	|                        |        |                        |
 *	|       reverse d        |        |                        |
 *	|                        |        |                        |
 *	|          Send          |------->|          Recv          |
 *	|                        |        |                        |
 *	|    Verify and close    |        |    Verify and close    |
 *	|                        |        |                        |
 *	\----------Join----------/        \----------Join----------/
 */
int udp_proper();
void udp_proper_t1(int&, sks::domain);
void udp_proper_t2(int&, sks::domain);
void init();
void cleanup();

int main() {

	#ifdef _WIN32
		WSADATA wsaData;
		int iResult;

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed: %d\n", iResult);
			return 1;
		}
	#endif

	size_t failureTotal = allUnitTests();
	
	//Check report for failure rate
	//Return number of failures
	//int failureTotal = 0;
	size_t passedTotal = 0;
	for (const std::pair<std::string, bool>& rpt : report) {
		if (rpt.second == true) {
			passedTotal++;
		}
	}
	size_t reportsTotal = passedTotal + failureTotal;
	double percentPassing = 100 - (100. * failureTotal / reportsTotal);
	
	std::cout << std::endl;
	if (failureTotal == 0) {
		std::cout << tagok << "\tAll tests passed." << std::endl;
	} else {
		std::cerr << tagfail << "\tTotal of " << failureTotal << "/" << reportsTotal << " check(s) failed. (" << std::setprecision(2) << percentPassing << "% passing)." << std::endl;
	}
	std::cout << std::endl;
	
	//Set readme badge:
	//"https://img.shields.io/badge/Unit%20Tests-100%25-success"
	//"https://img.shields.io/badge/Unit%20Tests-99%25-critical"
	
	return failureTotal;
}

size_t allUnitTests() {
	init();
	
	size_t failures = 0;
	
	failures += tcp_proper();
	failures += udp_proper();
	
	cleanup();
	
	return failures;
}

int tcp_proper() {
	int failures = 0;
	
	for (sks::domain d : allDomains) {		
		hostvalid = 0;
		resetverifystream();
		
		int t1f = 0;
		int t2f = 0;
		
		std::thread t1(tcp_proper_t1, std::ref(t1f), d);
		std::thread t2(tcp_proper_t2, std::ref(t2f), d);
		
		t1.join();
		t2.join();
		
		//Report
		if (t1f > 0 || t2f > 0) { //At least one error
			std::cerr << tagfail << "\ttcp_proper(" << sks::to_string(d) << ") failed with " << (t1f + t2f) << " errors:" << std::endl;
			std::cerr << verifystreamstring() << std::endl;
		} else {
			std::cout << tagok << "\ttcp_proper(" << sks::to_string(d) << ") passed without errors." << std::endl;
		}
		
		failures += t1f + t2f;
	}
	std::cout << std::endl;
	
	return failures;
}
void tcp_proper_t1(int& failures, sks::domain d) {
	int e;
	sks::serror se;
	std::string prefix = "tcp_proper_t1(";
	prefix += sks::to_string(d);
	prefix += ") ";
	
	sks::socket_base h(d, sks::tcp); //Construct h
	
	if (d == sks::unix) {
		unlink("sks_utest_unix.sock"); //Make sure file is non-existant so we can bind successfully
		//Bind to this
		e = h.bind("sks_utest_unix.sock");
	} else {
		//Bind to something
		e = h.bind();
	}
	int bound = verify(prefix + "h.bind()", e, 0);
	if (bound == 0) {
		host = h.locaddr();
		hostvalid = 1;
	} else {
		hostvalid = 2;
	}
	failures += bound;
	
	
	se = h.listen();
	failures += verify(prefix + "h.listen()", se, snoerr);
	
	sks::socket_base* hc = nullptr;
	
	//We wait 5 seconds for a connection attempt, and rule the test as a failure if we get none
	e = h.canread(5000);
	failures += verify(prefix + "h.listen() wait", e, 1, false);
	if (e != 1) {
		//std::cerr << "No incoming connection requests after 5 seconds" << std::endl;
		
		failures += 8;
		return;
	}
	
	try {
		hc = new sks::socket_base( h.accept() );
		failures += verify(prefix + "h.accept()", norte, norte);
	} catch (std::runtime_error& rte) {
		failures += verify(prefix + "h.accept()", rte, norte);
		
		//Accept failed, we have no socket at `hc`, we cannot continue the test
		//Assume remaining verifications all fail
		failures += 7;

		delete hc;

		return;
	}
	
	e = hc->setrxtimeout(5000000); //microseconds
	failures += verify(prefix + "hc.setrxtimeout(5s)", e, 0);
	e = hc->settxtimeout(5000000); //microseconds
	failures += verify(prefix + "hc.settxtimeout(5s)", e, 0);
	
	std::vector<uint8_t> data;
	se = hc->recvfrom(data);
	failures += verify(prefix + "hc.recvfrom(...)#1 error", se, snoerr);
	failures += verify(prefix + "hc.recvfrom(...)#1 value", data, allBytes);
	
	::reverse(data.begin(), data.end());
	
	se = hc->sendto(data);
	failures += verify(prefix + "hc.sendto(...) error", se, snoerr);
		
	se = hc->recvfrom(data);
	sks::serror classclosed = { sks::CLASS, sks::CLOSED };
	failures += verify(prefix + "hc.recvfrom(...)#2 error", se, classclosed);
	//data's value is undefined, at least in the current version
	failures += verify(prefix + "hc.valid()", hc->valid(), false);
	
	delete hc;
}
void tcp_proper_t2(int& failures, sks::domain d) {
	int e;
	sks::serror se;
	std::string prefix = "tcp_proper_t2(";
	prefix += sks::to_string(d);
	prefix += ") ";
	
	sks::socket_base* c = new sks::socket_base(d, sks::tcp);
	
	while(hostvalid == 0) {} //Wait for host to bind and set connection data
	if (hostvalid == 2) {
		failures += 7;
		return;
	}
	
	e = c->connect(host);
	failures += verify(prefix + "c.connect(...)", e, 0);
	
	//We wait 5 seconds for the accept() call, and rule the test as a failure if it takes more than 5 seconds
	e = c->canwrite(5000);
	failures += verify(prefix + "c.canwrite(...) wait", e, 1, false);
	if (e != 1) {
		//std::cerr << "No incoming connection requests after 5 seconds" << std::endl;
		failures += 5;
		return;
	}
	
	e = c->setrxtimeout(5000000); //microseconds
	failures += verify(prefix + "c.setrxtimeout(5s)", e, 0);
	e = c->settxtimeout(5000000); //microseconds
	failures += verify(prefix + "c.settxtimeout(5s)", e, 0);
	
	se = c->sendto(allBytes);
	failures += verify(prefix + "c.sendto(...) error", se, snoerr);
	
	std::vector<uint8_t> data;
	se = c->recvfrom(data);
	failures += verify(prefix + "c.recvfrom(...) error", se, snoerr);
	failures += verify(prefix + "c.recvfrom(...) data", data, allBytesReversed);
	
	//Close
	delete c;
}

int udp_proper() {
	int failures = 0;
	
	for (sks::domain d : allDomains) {
		sockready = 0;
		resetverifystream();
		
		int t1f = 0;
		int t2f = 0;

#ifdef _WIN32
		if (d == sks::unix) {
			// UDP UNIX not supported on windows(At least yet, but lets be honest with ourselves)
			std::cout << tagnote << "\tUnix UDP sockets are not supported on this platform. Skipping." << std::endl;
			continue;
		}
#endif
		
		std::thread t1(udp_proper_t1, std::ref(t1f), d);
		std::thread t2(udp_proper_t2, std::ref(t2f), d);
		
		t1.join();
		t2.join();
		
		//Report
		if (t1f > 0 || t2f > 0) { //At least one error
			std::cerr << tagfail << "\tudp_proper(" << sks::to_string(d) << ") failed with " << (t1f + t2f) << " errors:" << std::endl;
			std::cerr << verifystreamstring() << std::endl;
		} else {
			std::cout << tagok << "\tudp_proper(" << sks::to_string(d) << ") passed without errors." << std::endl;
		}
		
		failures += t1f + t2f;
	}
	std::cout << std::endl;
	
	return failures;
}
void udp_proper_t1(int& failures, sks::domain d) {
	int e;
	sks::serror se;
	std::string prefix = "udp_proper_t1(";
	prefix += sks::to_string(d);
	prefix += ") ";
	
	sks::socket_base b(d, sks::udp);
	
	if (d == sks::unix) {
		unlink("./.socklib_unit_test_unix_socket_1"); //Make sure file is non-existant so we can bind successfully
		//Bind to this
		e = b.bind("./.socklib_unit_test_unix_socket_1");
	} else {
		//Bind to something
		e = b.bind();
	}
	failures += verify(prefix + "b.bind()", e, 0);
	
	host = b.locaddr();
	sockready++;
	
	while (sockready != 2) {}
	
	e = b.setrxtimeout(5000000); //microseconds
	failures += verify(prefix + "b.setrxtimeout(5s)", e, 0);
	e = b.settxtimeout(5000000); //microseconds
	failures += verify(prefix + "b.settxtimeout(5s)", e, 0);
	
	sks::packet pkt;
	se = b.recvfrom(pkt);
	failures += verify(prefix + "b.recvfrom(...) error", se, snoerr);
	failures += verify(prefix + "b.recvfrom(...) data", pkt.data, allBytes);
	
	::reverse(pkt.data.begin(), pkt.data.end());
	
	se = b.sendto(pkt);
	//std::cout << "b @ " << b.locaddr().addrstring << " port = " << b.locaddr().port << "; sending to a @ " << pkt.rem.addrstring << " port = " << pkt.rem.port << std::endl;
	failures += verify(prefix + "b.sendto(...) error", se, snoerr);
	
	sockready--;
	
	while (sockready != 0) {}
	
	//Close
}
void udp_proper_t2(int& failures, sks::domain d) {
	int e;
	sks::serror se;
	std::string prefix = "udp_proper_t2(";
	prefix += sks::to_string(d);
	prefix += ") ";
	
	sks::socket_base a(d, sks::udp);
	
	if (d == sks::unix) {
		unlink("./.socklib_unit_test_unix_socket_2"); //Make sure file is non-existant so we can bind successfully
		//Bind to this
		e = a.bind("./.socklib_unit_test_unix_socket_2");
	} else {
		//Bind to something
		e = a.bind();
	}
	failures += verify(prefix + "a.bind()", e, 0);
	
	sockready++;
	
	while (sockready != 2) {}
	
	e = a.setrxtimeout(5000000); //microseconds
	failures += verify(prefix + "a.setrxtimeout(5s)", e, 0);
	e = a.settxtimeout(5000000); //microseconds
	failures += verify(prefix + "a.settxtimeout(5s)", e, 0);
	
	sks::packet pkt;
	pkt.rem = host;
	pkt.data = allBytes;
	
	se = a.sendto(pkt);
	failures += verify(prefix + "a.sendto(...)", se, snoerr);
	//std::cout << "a @ " << a.locaddr().addrstring << " port = " << a.locaddr().port << "; sending to b @ " << pkt.rem.addrstring << " port = " << pkt.rem.port << std::endl;
	
	se = a.recvfrom(pkt);
	failures += verify(prefix + "a.recvfrom(...) error", se, snoerr);
	failures += verify(prefix + "a.recvfrom(...) data", pkt.data, allBytesReversed);
	
	sockready--;
	
	while (sockready != 0) {}
	
	//Close
}


void init() {
	for (size_t i = 0; i <= UINT8_MAX; i++) {
		allBytes.push_back(i);
		allBytesReversed.push_back(UINT8_MAX - i);
	}
}
void cleanup() {
	
}

//