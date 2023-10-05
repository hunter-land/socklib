#include "steps.hpp"
#include <ostream>
#include "socks.hpp"
#include <btf/testing.hpp>
#include "utility.hpp"
#include <mutex>
#include <thread>

void givenSystemSupports(std::ostream& log, sks::domain d, sks::type t) {
	try {
		sks::socket(d, t);
	} catch (std::exception& e) {
		assert(btf::ignore, "System does not support " + str(d) + " " + str(t));
	}
}

std::pair<sks::socket, sks::socket> getRelatedSockets(std::ostream& log, sks::domain d, sks::type t) {
	//Generate and connect (if applicable) two sockets
	std::mutex logMutex;
	bool accepterReady = false;
	std::pair<sks::socket*, sks::socket*> pair;
	sks::address listeningAddr("0.0.0.0");
	std::exception_ptr except; //If either thread finds an exception, set this to the exception, exit threads, and throw it

	if (t == sks::stream || t == sks::seq) {
		//Connected types
		std::thread serverThread([&]{
			try {
				sks::socket listener(d, t, 0);
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
				accepterReady = true;
				listeningAddr = listener.localAddress();

				//Accept a connection
				sks::socket client = listener.accept();
				//clientAddress.second = new sks::address(client.connectedAddress());
				logMutex.lock();
				log << "Acting Client connected from " << client.connectedAddress().name() << std::endl;
				logMutex.unlock();

				sks::socket* cliHeap = new sks::socket(std::move(client));
				pair.first = cliHeap;
			} catch (...) {
				if (except == nullptr) {
					except = std::current_exception();
				}
			}
		});

		std::thread clientThread([&]{
			try {
				//Set up socket
				sks::socket server(d, t, 0);
				//Wait for host thread to be ready for us
				while (!accepterReady) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
				server.connect(listeningAddr); //Establish connection
				logMutex.lock();
				log << "Acting Client " << server.localAddress().name() << " connected to Acting Server " << server.connectedAddress().name() << std::endl;
				logMutex.unlock();

				//Connected, set structure variables
				sks::socket* serHeap = new sks::socket(std::move(server));
				pair.second = serHeap;
			} catch (...) {
				if (except == nullptr) {
					except = std::current_exception();
				}
			}
		});

		serverThread.join();
		clientThread.join();
	} else {
		//Non-connected types
		sks::socket socketA(d, t);
		socketA.bind(bindableAddress(d, 0));
		pair.first = new sks::socket(std::move(socketA));

		sks::socket socketB(d, t);
		socketB.bind(bindableAddress(d, 1));
		pair.second = new sks::socket(std::move(socketB));
	}

	if (except) {
		//Re-throw up to test manager
		std::rethrow_exception(except);
	}

	return {sks::socket(std::move(*pair.first)), sks::socket(std::move(*pair.second))};
}
