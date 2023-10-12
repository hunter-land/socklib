#include <ostream>
#include "socks.hpp"
#include "steps.hpp"
#include "utility.hpp"
#include <mutex>
#include <memory>
#include <thread>
#include <future>

void socketsSendAndReceive(std::ostream& log, const sks::domain& d, const sks::type& t) {
	assertSystemSupports(log, d, t);

	std::string serversMessage = "You have reached the server.";
	std::string clientsMessage = "Hey server, just saying hello!";

	std::pair<sks::socket, sks::socket> socks = getRelatedSockets(log, d, t);
	socketCanSendDataToSocket(log, socks.first, socks.second, serversMessage, t);
	socketCanSendDataToSocket(log, socks.second, socks.first, clientsMessage, t);
}

void addressesMatchCEquivalents(std::ostream& log, const sks::domain& d) {
	///Prerequisites to this test (throw results unable to meet requirement)///
	//Not any yet, since address don't need support on the hardware, just the right headers (which if we got here, we have)

	///Test variables///
	sks::address addrOrig;
	switch (d) {
		case sks::IPv4:
			addrOrig = sks::address(sks::IPv4Address("10.0.255.85:255"));
			break;
		case sks::IPv6:
			addrOrig = sks::address(sks::IPv6Address("[a:ff00:aaaa::ff:8d5]:16000"));
			break;
		case sks::unix:
			addrOrig = sks::address(sks::unixAddress("./folder/../socket.unix.address"));
			break;
		#ifdef has_ax25
		case sks::ax25:
			addrOrig = sks::address(sks::ax25Address("KK7IPE VIA KI7VLD-7"));
			break;
		#endif
		default:
			throw std::runtime_error("Unknown domain");
	}

	///The test///
	//Cast to C address (and length)
	log << "Casing sks::address to sockaddr_storage" << std::endl;
	sockaddr_storage caddr = addrOrig;
	socklen_t caddrlen = addrOrig.size();

	//Cast it back
	log << "Constructing sks::address from sockaddr_storage and length" << std::endl;
	sks::address addrFromC(caddr, caddrlen);
	assertEqual(addrFromC, addrOrig, "Addresses do not match after casting to and from sockaddr struct");
}

void socketPairsCanBeCreated(std::ostream& log, const sks::type& t) {
	///Prerequisites to this test (throw results unable to meet requirement)///
	assertSystemSupports(log, sks::unix, t);

	///The test///
	std::pair<sks::socket, sks::socket> sockets = sks::createUnixPair(t);
	sks::socket& a = sockets.first;
	sks::socket& b = sockets.second;

	 //NOTE: sks::stream is used below (instead of the actual type) due to how socketpair (underlying function for createUnixPair) works
	//       	It associates the sockets and 'connects' them even though this type can't be connected, so we can't include an address when sending

	//a --> b
	socketCanSendDataToSocket(log, a, b, "Message from first socket to second", sks::stream);

	//b --> a
	socketCanSendDataToSocket(log, b, a, "Message from first socket to second", sks::stream);
}

void readReadyTimesOutCorrectly(std::ostream& log, const sks::domain& d, const sks::type& t, const std::chrono::milliseconds& timeout) {
	assertSystemSupports(log, d, t);

	std::chrono::milliseconds timeoutGrace(10); //Allow 10ms extra for timeout (Code isn't instant, and the OS will get to our call when it gets to it)
	//Regardless of boolean returned, if readReady(...) takes too long (T + 10ms), fail the test

	auto sockets = getRelatedSockets(log, d, t);
	sks::socket& sockA = sockets.first;
	sks::socket& sockB = sockets.second;
	std::chrono::time_point<std::chrono::system_clock> preTimeoutTime, postTimeoutTime;
	std::chrono::milliseconds timeMax = timeout + timeoutGrace;

	bool rr; //readReady(...) return
	std::chrono::system_clock::duration timeDiff; //Time it took for readReady(...) to return

	//Check when there is no data waiting
	log << "Timing readReady(...) without any data to read" << std::endl;
	preTimeoutTime = std::chrono::system_clock::now();
	rr = sockA.readReady(timeout);
	postTimeoutTime = std::chrono::system_clock::now();
	timeDiff = postTimeoutTime - preTimeoutTime;
	assertFalse(rr, "readReady returned true when there was no data");
	assertTrue(timeout <= timeDiff, "readReady() returned prematurely");
	assertTrue(timeDiff < timeMax, "readReady() took longer than timeout to return");

	 //Send data so the check below should return true
	log << "Sending data to socket" << std::endl;
	if (t == sks::stream || t == sks::seq) {
		sockB.send({'C', 'h', 'e', 'c', 'k'});
	} else {
		sockB.send({'C', 'h', 'e', 'c', 'k'}, sockA.localAddress());
	}

	//Check when we have data waiting
	log << "Timing readReady(...) with data to read" << std::endl;
	preTimeoutTime = std::chrono::system_clock::now();
	rr = sockA.readReady(timeout);
	postTimeoutTime = std::chrono::system_clock::now();
	timeDiff = postTimeoutTime - preTimeoutTime;
	assertTrue(rr, "readReady returned false when there was data");
	assertTrue(timeDiff < timeoutGrace, "readReady() took longer than grace period to return");
}
