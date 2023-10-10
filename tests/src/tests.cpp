#include <ostream>
#include "socks.hpp"
#include "steps.hpp"
#include "utility.hpp"
#include <mutex>
#include <memory>
#include <thread>
#include <future>

void socketsSendAndReceive(std::ostream& log, sks::domain d, sks::type t) {
	assertSystemSupports(log, d, t);

	std::string serversMessage = "You have reached the server.";
	std::string clientsMessage = "Hey server, just saying hello!";
	std::vector<uint8_t> serversMessageBytes(serversMessage.begin(), serversMessage.end());
	std::vector<uint8_t> clientsMessageBytes(clientsMessage.begin(), clientsMessage.end());

	//Two paths - One for connected types, and one for unconnected types
	if (t == sks::stream || t == sks::seq) {
		//Connected type

		//Server-side setup
		sks::socket listener(d, t);
		log << "Binding listener" << std::endl;
		listener.bind(bindableAddress(d));
		log << "Bound to " << listener.localAddress().name() << std::endl;
		listener.listen();
		log << "Listener is listening" << std::endl;
		auto acceptedFuture = std::async(std::launch::async, &sks::socket::accept, &listener); //Accept the next connection (wait in worker thread)

		//Switch to client-side
		sks::socket client(d, t);
		log << "Connecting to listener" << std::endl;
		client.connect(listener.localAddress());
		log << "Client connected, sending client's message" << std::endl;
		client.send(clientsMessageBytes);

		//Switch back to server-side
		sks::socket accepted = acceptedFuture.get();
		log << "Sending server's message" << std::endl;
		accepted.send(serversMessageBytes);
		std::vector<uint8_t> clientsMessageBytesReceived = accepted.receive();
		std::string clientsMessageReceived(clientsMessageBytesReceived.begin(), clientsMessageBytesReceived.end());
		assertEqual(clientsMessageReceived, clientsMessage, "Message recevied from client got mangled");

		//Switch back to client-side
		std::vector<uint8_t> serversMessageBytesReceived = client.receive();
		std::string serversMessageReceived(serversMessageBytesReceived.begin(), serversMessageBytesReceived.end());
		assertEqual(serversMessageReceived, serversMessage, "Message recevied from server got mangled");
	} else {
		//Non-connected type

		//Server-side setup
		sks::socket server(d, t);
		log << "Binding server socket" << std::endl;
		server.bind(bindableAddress(d, 0));
		sks::address serverAddr = server.localAddress();
		log << "Server bound to " << serverAddr.name() << std::endl;

		//Switch to client-side
		sks::socket client(d, t);
		log << "Binding client socket" << std::endl;
		client.bind(bindableAddress(d, 1));
		sks::address clientAddr = client.localAddress();
		log << "Client bound to " << clientAddr.name() << std::endl;
		client.send(clientsMessageBytes, serverAddr);
		log << "Client sent message to server" << std::endl;

		//Switch back to server-side
		sks::address clientAddrReceived("127.0.0.1");
		std::vector<uint8_t> clientsMessageBytesReceived = server.receive(clientAddrReceived);
		std::string clientsMessageReceived(clientsMessageBytesReceived.begin(), clientsMessageBytesReceived.end());
		assertEqual(clientAddrReceived, clientAddr, "Message received from client came from a different address");
		assertEqual(clientsMessageReceived, clientsMessage, "Message recevied from client got mangled");
		server.send(serversMessageBytes, clientAddr);
		log << "Server sent message to client" << std::endl;

		//Switch back to client-side
		sks::address serverAddrReceived("127.0.0.1");
		std::vector<uint8_t> serversMessageBytesReceived = client.receive(serverAddrReceived);
		std::string serversMessageReceived(serversMessageBytesReceived.begin(), serversMessageBytesReceived.end());
		assertEqual(serverAddrReceived, serverAddr, "Message received from server came from a different address");
		assertEqual(serversMessageReceived, serversMessage, "Message recevied from server got mangled");
	}
}

void addressesMatchCEquivalents(std::ostream& log, sks::domain d) {
	///Prerequisites to this test (throw results unable to meet requirement)///
	//Not any yet, since address don't need support on the hardware, just the right headers (which if we got here, we have)

	///Test variables///
	std::pair<sks::address*, sks::address*> addr = { nullptr, nullptr };
	switch (d) {
		case sks::IPv4:
			addr.first = new sks::address(sks::IPv4Address("10.0.255.85:255"));
			break;
		case sks::IPv6:
			addr.first = new sks::address(sks::IPv6Address("[a:ff00:aaaa::ff:8d5]:16000"));
			break;
		case sks::unix:
			addr.first = new sks::address(sks::unixAddress("./folder/../socket.unix.address"));
			break;
		#ifdef has_ax25
		case sks::ax25:
			addr.first = new sks::address(sks::ax25Address("KK7IPE VIA KI7VLD-7"));
			break;
		#endif
		default:
			throw std::runtime_error("Unknown domain.");
	}

	///The test///
	//Cast to C address (and length)
	sockaddr_storage caddr = *addr.first;
	socklen_t caddrlen = addr.first->size();

	//Cast it back
	addr.second = new sks::address(caddr, caddrlen);
	assertEqual(*addr.second, *addr.first, "Addresses do not match after casting to and from sockaddr struct");
}

void socketPairsCanBeCreated(std::ostream& log, sks::type t) {
	///Prerequisites to this test (throw results unable to meet requirement)///
	givenSystemSupports(log, sks::unix, t);

	///Test setup/variables///
	std::pair<std::string, std::string> firstMessage = { "Message from first socket to second", "" };
	std::pair<std::string, std::string> secondMessage = { "Message from second socket to first", "" };

	///The test///
	std::pair<sks::socket, sks::socket> sockets = sks::createUnixPair(t);
	sks::socket& a = sockets.first;
	sks::socket& b = sockets.second;

	//a --> b
	std::vector<uint8_t> messageBytes(firstMessage.first.begin(), firstMessage.first.end());
	a.send(messageBytes);
	messageBytes = b.receive();
	firstMessage.second = std::string(messageBytes.begin(), messageBytes.end());

	//b --> a
	messageBytes = std::vector<uint8_t>(secondMessage.first.begin(), secondMessage.first.end());
	b.send(messageBytes);
	messageBytes = a.receive();
	secondMessage.second = std::string(messageBytes.begin(), messageBytes.end());

	///Final asserts and cleanup///
	assertEqual(firstMessage.second, firstMessage.first, "Message from a to b does not match");
	assertEqual(firstMessage.second, firstMessage.first, "Message from b to a does not match");
}

void readReadyTimesOutCorrectly(std::ostream& log, sks::domain d, sks::type t) {
	///Prerequisites to this test (throw results unable to meet requirement)///
	givenSystemSupports(log, d, t);

	///Test variables///
	std::chrono::milliseconds timeoutGrace(10); //Allow 10ms extra for timeout
	std::vector<std::chrono::milliseconds> timeouts = { std::chrono::milliseconds(0), std::chrono::milliseconds(100) };

	//For timeouts of { 0ms, 100ms }
	//Two connected sockets of d and t
	//Both should have readReady == false when given T to time out
	//Have one send a message, check readReady again with timeout T

	//Regardless of a result, if the function takes too long (T + 10ms) fail the test

	auto sockets = getRelatedSockets(log, d, t);
	sks::socket& sockA = sockets.first;
	sks::socket& sockB = sockets.second;
	std::chrono::time_point<std::chrono::system_clock> preTimeoutTime, postTimeoutTime;
	std::chrono::milliseconds timeDiff;
	bool tooQuick = false, tooSlow = false;

	for (std::chrono::milliseconds timeoutTime : timeouts) {
		std::chrono::milliseconds timeMax = timeoutGrace + timeoutTime;
		log << "Checking with a timeout of " << timeoutTime.count() << "ms" << std::endl;
		log << "Checking should not take more than " << timeMax.count() << "ms" << std::endl;
		if (t == sks::stream || t == sks::seq) {
			//Connected types

			//First check without any data to read
			{
				preTimeoutTime = std::chrono::system_clock::now();
				assertFalse(sockA.readReady(timeoutTime), "socket A returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				assertFalse(tooQuick, "readReady() returned prematurely");
				assertFalse(tooSlow, "readReady() took longer than timeout to return");

				preTimeoutTime = std::chrono::system_clock::now();
				assertFalse(sockB.readReady(timeoutTime), "socket B returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				assertFalse(tooQuick, "readReady() returned prematurely");
				assertFalse(tooSlow, "readReady() took longer than timeout to return");
			}

			//Now send data to just one socket and check both again
			//Note: If there is data ready, returning early is allowed, but it should wait until the timeout finishes if there is not data
			sockB.send( {'A'} ); //Send a byte
			//A tiny sleep to allow the byte to flush and get received in the buffer (to help with flakiness)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			//Check again, results should be different for sockA
			{
				preTimeoutTime = std::chrono::system_clock::now();
				assertTrue(sockA.readReady(timeoutTime), "socket A returned false for readable when there was data");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				assertTrue(timeDiff < timeMax, "readReady() took longer than timeout to return");

				preTimeoutTime = std::chrono::system_clock::now();
				assertFalse(sockB.readReady(timeoutTime), "socket B returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				assertFalse(tooQuick, "readReady() returned prematurely");
				assertFalse(tooSlow, "readReady() took longer than timeout to return");
			}

			//Read the byte so the next timeout check starts without data
			sockA.receive();
		} else {
			//non-connected types should always return true when bound

			//First check without any data to read
			{
				preTimeoutTime = std::chrono::system_clock::now();
				assertFalse(sockA.readReady(timeoutTime), "socket A returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				assertFalse(tooQuick, "readReady() returned prematurely");
				assertFalse(tooSlow, "readReady() took longer than timeout to return");

				preTimeoutTime = std::chrono::system_clock::now();
				assertFalse(sockB.readReady(timeoutTime), "socket B returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				assertFalse(tooQuick, "readReady() returned prematurely");
				assertFalse(tooSlow, "readReady() took longer than timeout to return");
			}

			//Now send data to just one socket and check both again
			//Note: If there is data ready, returning early is allowed, but it should wait until the timeout finishes if there is not data
			sockB.send( {'A'}, sockA.localAddress() ); //Send a byte
			//A tiny sleep to allow the byte to flush and get received in the buffer (to help with flakiness)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));

			//Check again, results should be different for sockA
			{
				preTimeoutTime = std::chrono::system_clock::now();
				assertTrue(sockA.readReady(timeoutTime), "socket A returned false for readable when there was data");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				assertTrue( timeDiff < timeMax, "readReady() took longer than timeout to return");

				preTimeoutTime = std::chrono::system_clock::now();
				assertFalse(sockB.readReady(timeoutTime), "socket B returned true for readable when no data was sent");
				postTimeoutTime = std::chrono::system_clock::now();
				timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(postTimeoutTime - preTimeoutTime);
				//Timeout (with no data to read) should be just a little above the timeoutTime, but not below
				tooQuick = timeDiff < timeoutTime;
				tooSlow = timeDiff > timeMax;
				assertFalse(tooQuick, "readReady() returned prematurely");
				assertFalse(tooSlow, "readReady() took longer than timeout to return");
			}

			//Read the byte so the next timeout check starts without data
			sockA.receive();
		}
	}
}
