#include "steps.hpp"
#include "testing.hpp"
#include <ostream>
#include <mutex>
#include <thread>
extern "C" {
	#include <sys/socket.h>
}

/**	
 *	This file contains all basic/critical tests for functionality of sockets.
 *	
 */

void AddressesMatchCEquivalents(std::ostream& log, sks::domain d) {
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
		default:
			throw std::runtime_error("Unknown domain.");
	}
	
	///The test///
	//Cast to C address (and length)
	sockaddr_storage caddr = *addr.first;
	socklen_t caddrlen = addr.first->size();
	
	//Cast it back
	addr.second = new sks::address(caddr, caddrlen);
	testing::assertTrue(
		*addr.first == *addr.second,
		"Addresses do not match\n"
		"Expected [ " + addr.first->name() + " ]\n"
		"Actual   [ " + addr.second->name() + " ]"
	);
}

void SocketsCanCommunicate(std::ostream& log, sks::domain d, sks::type t) {
	///Prerequisites to this test (throw results unable to meet requirement)///
	GivenTheSystemSupports(log, d);
	
	///Test setup/variables///
	std::mutex logMutex;
	std::pair<sks::address*, sks::address*> serverAddress = { nullptr, nullptr };
	std::pair<std::string, std::string> hostsMessage = { "You have reached the server.", "" };
	std::pair<std::string, std::string> clientsMessage = { "Hey server, just saying hello!", "" };
	
	///The test///
	std::thread hostThread([&]{
		sks::socket listener(sks::IPv4, sks::stream, 0);	
		//Bind to localhost (intra-system address) with random port assigned by OS
		listener.bind(bindableAddress(d));
		logMutex.lock();
		log << "Acting Server bound to " << listener.localAddress().name() << std::endl;
		logMutex.unlock();
		
		//Listen
		listener.listen();
		logMutex.lock();
		log << "Acting Server is listening" << std::endl;
		logMutex.unlock();
		serverAddress.first = new sks::address(listener.localAddress());
		
		//Accept a connection
		sks::socket client = listener.accept();
		logMutex.lock();
		log << "Acting Client connected from " << client.connectedAddress().name() << std::endl;
		logMutex.unlock();
		
		//Send a message to the client
		std::vector<uint8_t> messageData(hostsMessage.first.begin(), hostsMessage.first.end()); //Convert the string into vector of bytes
		client.send(messageData); //Send the message to the client
		logMutex.lock();
		log << "Acting Server sent \"" << hostsMessage.first << "\" to Acting Client" << std::endl;
		logMutex.unlock();

		//Receive a message from the client
		std::vector<uint8_t> clientMessageData = client.receive(); //Get the message as a vector of bytes
		clientsMessage.second = std::string(clientMessageData.begin(), clientMessageData.end()); //We know the message is a string so we create a string out of it
		logMutex.lock();
		log << "Acting Client said \"" << clientsMessage.second << "\" to Acting Server" << std::endl; //Display the string
		logMutex.unlock();
	});
	std::thread clientThread([&]{
		//Set up socket
		sks::socket server(sks::IPv4, sks::stream, 0);
		//Wait for host thread to be ready for us
		while (serverAddress.first == nullptr) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		server.connect(*serverAddress.first); //Establish connection
		serverAddress.second = new sks::address(server.connectedAddress());
		logMutex.lock();
		log << "Acting Client " << server.localAddress().name() << " connected to Acting Server " << server.connectedAddress().name() << std::endl;
		logMutex.unlock();

		//Receive a message from the server
		std::vector<uint8_t> serverMessageData = server.receive(); //Get the message as a vector of bytes
		hostsMessage.second = std::string(serverMessageData.begin(), serverMessageData.end()); //We know the message is a string so we create a string out of it
		logMutex.lock();
		log << "Acting Server said \"" << hostsMessage.second << "\" to Acting Client" << std::endl; //Display the string
		logMutex.unlock();

		//Send a message to the server
		std::vector<uint8_t> messageData(clientsMessage.first.begin(), clientsMessage.first.end()); //Convert string into vector of bytes
		server.send(messageData); //Send the message to the server
		logMutex.lock();
		log << "Acting Client sent \"" << clientsMessage.first << "\" to Acting Server" << std::endl;
		logMutex.unlock();
	});
	
	hostThread.join();
	clientThread.join();
	
	testing::assertTrue(
		*serverAddress.first == *serverAddress.second,
		"Host's local address differs from client's remote address\n"
		"Expected [ " + serverAddress.first->name() + " ]\n"
		"Actual   [ " + serverAddress.second->name() + " ]"
	);
	delete serverAddress.first;
	delete serverAddress.second;	
	testing::assertTrue(hostsMessage.first == hostsMessage.second, "Hosts's message was not correctly received by client");
	testing::assertTrue(clientsMessage.first == clientsMessage.second, "Client's message was not correctly received by host");
}
