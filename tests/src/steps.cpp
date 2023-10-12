#include "steps.hpp"
#include <ostream>
#include "socks.hpp"
#include <btf/testing.hpp>
#include "utility.hpp"
#include <mutex>
#include <thread>
#include <future>

void assertSystemSupports(std::ostream& log, sks::domain d, sks::type t) {
	try {
		sks::socket(d, t);
	} catch (std::exception& e) {
		assert(btf::ignore, "System does not support " + str(d) + " " + str(t));
	}
}

std::pair<sks::socket, sks::socket> getRelatedSockets(std::ostream& log, sks::domain d, sks::type t) {
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
		log << "Client connected to listener" << std::endl;

		//Switch back to server-side
		sks::socket accepted = acceptedFuture.get();
		return { std::move(client), std::move(accepted) };
	} else {
		sks::socket sockA(d, t);
		sockA.bind(bindableAddress(d, 0));
		log << "Binding socket A" << std::endl;
		sks::socket sockB(d, t);
		sockB.bind(bindableAddress(d, 1));
		log << "Binding socket B" << std::endl;
		return { std::move(sockA), std::move(sockB) };
	}
}

void socketCanSendDataToSocket(std::ostream& log, sks::socket& sender, sks::socket& receiver, const std::string& message, sks::type t) {
	std::vector<uint8_t> messageBytes(message.begin(), message.end());
	std::vector<uint8_t> messageBytesReceived;

	if (t == sks::stream || t == sks::seq) {
		sender.send(messageBytes);
		log << "Sent message" << std::endl;
		messageBytesReceived = receiver.receive();
		log << "Received message" << std::endl;
	} else {
		sender.send(messageBytes, receiver.localAddress());
		log << "Sent message" << std::endl;
		sks::address receivedFromAddr;
		messageBytesReceived = receiver.receive(receivedFromAddr);
		log << "Received message" << std::endl;
		assertEqual(sender.localAddress(), receivedFromAddr, "Socket received data from wrong address");
	}

	std::string messageReceived(messageBytesReceived.begin(), messageBytesReceived.end());
	assertEqual(messageReceived, message, "Received message differs from sent message");
}
