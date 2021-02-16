#include "../socks.hpp"
#include <map>
#include <string>
#include <thread>
#include <iostream>
#include <vector>
#include <cstdint>
#include <mutex>
#include <sstream>
extern "C" {
	#include <unistd.h> //ioctl
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
static std::string tagok("[\033[32m OK \033[0m]");
static std::stringstream verifystream;
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
static int verify(std::string name, int actual, int expected) {
	bool match = actual == expected;
	
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
static int hostvalid = 0; //Invalid = 0, valid = 1, will never be valid (skip) = 2

void allUnitTests();
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
void init();
void cleanup();



int main() {
	allUnitTests();
	
	//Check report for failure rate
	//Return number of failures
	int failureTotal = 0;
	for (const std::pair<std::string, bool>& rpt : report) {
		if (rpt.second == false) {
			failureTotal++;
		}
	}
	
	std::cout << std::endl;
	if (failureTotal == 0) {
		std::cout << tagok << "\tAll tests passed." << std::endl;
	} else {
		std::cerr << tagfail << "\tTotal of " << failureTotal << " check(s) failed." << std::endl;
	}
	std::cout << std::endl;
	
	//Set badge value and color for passing:
	//TODO
	
	return failureTotal;
}



void allUnitTests() {
	init();
	
	tcp_proper();
	
	cleanup();
}

int tcp_proper() {
	for (sks::domain d : allDomains) {		
		hostvalid = 0;
		verifystream = std::stringstream(std::string());
		
		int t1f = 0;
		int t2f = 0;
		
		std::thread t1(tcp_proper_t1, std::ref(t1f), d);
		std::thread t2(tcp_proper_t2, std::ref(t2f), d);
		
		t1.join();
		t2.join();
		
		//Report
		if (t1f > 0 || t2f > 0) { //At least one error
			std::cerr << tagfail << "\ttcp_proper(" << sks::to_string(d) << ") failed with " << (t1f + t2f) << " errors:" << std::endl;
			std::cerr << verifystream.str() << std::endl;
		} else {
			std::cout << tagok << "\ttcp_proper(" << sks::to_string(d) << ") passed without errors." << std::endl;
		}
	}
	std::cout << std::endl;
	
	return 0;
}
void tcp_proper_t1(int& failures, sks::domain d) {
	int e;
	sks::serror se;
	std::string prefix = "tcp_proper_t1(";
	prefix += sks::to_string(d);
	prefix += ") ";
	
	sks::socket_base h(d, sks::tcp); //Construct h
	
	if (d == sks::unix) {
		unlink("./.socklib_unit_test_unix_socket"); //Make sure file is non-existant so we can bind successfully
		//Bind to this
		e = h.bind("./.socklib_unit_test_unix_socket");
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
	try {
		hc = new sks::socket_base(std::move(h.accept()));
		failures += verify(prefix + "h.accept()", norte, norte);
	} catch (std::runtime_error& rte) {
		failures += verify(prefix + "h.accept()", rte, norte);
		
		//Accept failed, we have no socket at `hc`, we cannot continue the test
		//Assume remaining verifications all fail
		failures += 4;
		if (d != sks::unix) {
			failures += 3;
		}
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
	
	//Unix sockets are not bi-directional
	if (d != sks::unix) {
		std::reverse(data.begin(), data.end());
		
		se = hc->sendto(data);
		failures += verify(prefix + "hc.sendto(...) error", se, snoerr); //"No such file or directory" is because rem_addr string is ""
			
		se = hc->recvfrom(data);
		sks::serror classclosed = { sks::CLASS, sks::CLOSED };
		failures += verify(prefix + "hc.recvfrom(...)#2 error", se, classclosed);
		//data's value is undefined, at least for now
		failures += verify(prefix + "hc.valid()", hc->valid(), false);
	}
	
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
	
	e = c->connect(host);
	failures += verify(prefix + "c.connect(...)", e, 0);
	
	e = c->setrxtimeout(5000000); //microseconds
	failures += verify(prefix + "c.setrxtimeout(5s)", e, 0);
	e = c->settxtimeout(5000000); //microseconds
	failures += verify(prefix + "c.settxtimeout(5s)", e, 0);
	
	se = c->sendto(allBytes);
	failures += verify(prefix + "c.sendto(...) error", se, snoerr);
	
	if (d != sks::unix) {
		std::vector<uint8_t> data;
		se = c->recvfrom(data);
		failures += verify(prefix + "c.recvfrom(...) error", se, snoerr);
		failures += verify(prefix + "c.recvfrom(...) data", data, allBytesReversed);
	}
	
	//Close
	delete c;
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