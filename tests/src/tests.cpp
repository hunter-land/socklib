#include <ostream>
#include "socks.hpp"
#include "steps.hpp"
#include "utility.hpp"
#include <mutex>
#include <memory>
#include <thread>
#include <btf/testing.hpp>

static const std::chrono::milliseconds timeoutGrace(5); //Allow 1ms extra for timeouts (Code isn't instant, and the OS will get to our call when it gets to it)
static const std::chrono::milliseconds timeoutError(5); //Allowed +/- to timeouts (Add above for upper-bound)

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

	auto sockets = getRelatedSockets(log, d, t);
	sks::socket& sockA = sockets.first;
	sks::socket& sockB = sockets.second;
	std::chrono::time_point<std::chrono::system_clock> preTimeoutTime, postTimeoutTime;
	std::chrono::milliseconds timeMin = timeout - timeoutError;
	std::chrono::milliseconds timeMax = timeout + timeoutError + timeoutGrace;

	bool rr; //readReady(...) return
	std::chrono::system_clock::duration timeDiff; //Time it took for readReady(...) to return

	//Check when there is no data waiting
	log << "Timing readReady(...) without any data to read" << std::endl;
	preTimeoutTime = std::chrono::system_clock::now();
	rr = sockA.readReady(timeout);
	postTimeoutTime = std::chrono::system_clock::now();
	timeDiff = postTimeoutTime - preTimeoutTime;
	assertFalse(rr, "readReady returned true when there was no data");
	assertGreaterThan(timeDiff, timeMin, "readReady() returned prematurely");
	assertLessThan(timeDiff, timeMax, "readReady() took longer than timeout to return");

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
	assertLessThan(timeDiff, timeoutGrace, "readReady() took longer than grace period to return");
}

void bytesReadyCorrectCount(std::ostream& log, const sks::domain& d, const sks::type& t) {
	assertSystemSupports(log, d, t);

	std::pair<sks::socket, sks::socket> sockPair = getRelatedSockets(log, d, t);
	std::vector<uint8_t> dataToSend = {'C', 'h', 'e', 'c', 'k'};
	sks::socket& sockA = sockPair.first;
	sks::socket& sockB = sockPair.second;
	size_t br;

	br = sockA.bytesReady();
	assertEqual(br, 0, "bytesReady returned non-zero before any data was sent");

	log << "Sending data (" << dataToSend.size() << " bytes) to socket" << std::endl;
	if (t == sks::stream || t == sks::seq) {
		sockB.send(dataToSend);
	} else {
		sockB.send(dataToSend, sockA.localAddress());
	}

	br = sockA.bytesReady();
	assertEqual(br, dataToSend.size(), "bytesReady returned an incorrect byte count");

	log << "Receiving (reading) data from buffer" << std::endl;
	if (t == sks::stream || t == sks::seq) {
		sockA.receive();
	} else {
		sks::address addr;
		sockA.receive(addr);
	}

	br = sockA.bytesReady();
	assertEqual(br, 0, "bytesReady returned non-zero after reading all data");
}

void receiveTimesOutCorrectly(std::ostream& log, const sks::domain& d, const sks::type& t, const std::chrono::milliseconds& timeout) {
	assertSystemSupports(log, d, t);
	if (timeout.count() == 0) {
		//timeout of 0 is "do not time out". This will cause the test below to hang when we expect it to time out
		assert(btf::ignore, "Timeout of 0ms invalid for test");
	}

	auto sockets = getRelatedSockets(log, d, t);
	sks::socket& sockA = sockets.first;
	sks::socket& sockB = sockets.second;
	std::chrono::time_point<std::chrono::system_clock> preTimeoutTime, postTimeoutTime;
	std::chrono::milliseconds timeMin = timeout - timeoutError;
	std::chrono::milliseconds timeMax = timeout + timeoutError + timeoutGrace;

	std::vector<uint8_t> receivedBytes;
	std::chrono::system_clock::duration timeDiff; //Time it took for receive(...) to return

	//Set timeout
	sockA.receiveTimeout(timeout);
	auto timeoutReadBack = std::chrono::duration_cast<std::chrono::milliseconds>(sockA.receiveTimeout());
	//OS can cause rounding in timeout time. (eg 100ms can become 103.3ms)
	assertLessThanEqual(timeoutReadBack, timeout + timeoutError, "receive timeout was set higher than error allows");
	assertGreaterThanEqual(timeoutReadBack, timeout - timeoutError, "receive timeout was set lower than error allows");

	//Call receive without data waiting - Timeout expected
	bool received = false;
	preTimeoutTime = std::chrono::system_clock::now();
	try {
		if (t == sks::stream || t == sks::seq) {
			receivedBytes = sockA.receive();
		} else {
			sks::address addr;
			receivedBytes = sockA.receive(addr);
		}
		received = true;
	} catch (const std::system_error& e) {
		const std::vector<std::errc> allowed = { std::errc::resource_unavailable_try_again, std::errc::timed_out };
		assertOneOf((std::errc)e.code().value(), allowed, "Timeout did not throw expected error code");
	}
	assertFalse(received, "receive() did not time out with no data waiting");
	postTimeoutTime = std::chrono::system_clock::now();
	timeDiff = postTimeoutTime - preTimeoutTime;
	assertGreaterThan(timeDiff, timeMin, "receive() returned prematurely");
	assertLessThan(timeDiff, timeMax, "receive() took longer than timeout to return");

	//Send data so the check below should return true
	log << "Sending data to socket" << std::endl;
	if (t == sks::stream || t == sks::seq) {
		sockB.send({'C', 'h', 'e', 'c', 'k'});
	} else {
		sockB.send({'C', 'h', 'e', 'c', 'k'}, sockA.localAddress());
	}

	//Call receive without data waiting - Should return instantly
	preTimeoutTime = std::chrono::system_clock::now();
	if (t == sks::stream || t == sks::seq) {
		receivedBytes = sockA.receive();
	} else {
		sks::address addr;
		receivedBytes = sockA.receive(addr);
	}
	postTimeoutTime = std::chrono::system_clock::now();
	timeDiff = postTimeoutTime - preTimeoutTime;
	assertLessThan(timeDiff, timeoutGrace, "receive() took longer than grace period to return");
}

void closedSocketCanBeDetected(std::ostream& log, const sks::domain& d, const sks::type& t) {
	assertSystemSupports(log, d, t);
	if (t != sks::stream && t != sks::seq) {
		assert(btf::ignore, "Test is only for connected types");
	}

	auto sockets = getRelatedSockets(log, d, t);
	sks::socket& sockA = sockets.first;
	//sks::socket& sockB = sockets.second;
	std::unique_ptr<sks::socket> sockBPtr = std::unique_ptr<sks::socket>(new sks::socket(std::move(sockets.second)));
	sockBPtr = nullptr; //Causes sockB to be deconstructed

	//Can we, without receiving, detect that sockB is closed?
	assertTrue(sockA.readReady(), "readReady returned false after peer closed connection");
	assertEqual(sockA.bytesReady(), 0, "bytesReady returned non-zero for a closed socket that didn't send anything");
}

void addressComparisonsAreCorrect(std::ostream& log, const sks::domain& d) {
	//Compare address of domain to 1) others of domain 2) other domains 3) blank
	//Compare with less-than and equal operators

	sks::address baseAddr = bindableAddress(d, 0);
	log << "Comparing addresses against " << baseAddr.name() << std::endl;

	for (const sks::address& addr : addresses) {
		//Should not be equal to this addr (check with `==` and `a < b && b < a`
		std::string addrStr = addr.size() > 0 ? addr.name() : "Empty Address";

		log << " == " << addrStr << std::endl;
		assertFalse(baseAddr == addr, "Addresses compared equal (==) when they are different");

		log << " < " << addrStr << std::endl;
		assertTrue(baseAddr < addr || addr < baseAddr, "Addresses compared equal (<) when they are different");
	}
}

void defaultConstructedAddressHasSizeOfZero(std::ostream& log) {
	assertEqual(sks::address().size(), 0, "Default-constructed address has non-zero size");
}
