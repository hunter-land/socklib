#include "steps.hpp"
#include "testing.hpp"
#include <socks/socks.hpp>
#include <socks/addrs.hpp>
#include "utility.hpp"
#include <ostream>
#include <thread>
#include <mutex>



sks::address bindableAddress(sks::domain d, uint8_t index) {
	switch (d) {
		case sks::IPv4:
			return sks::address("127.0.0.1:0", d);
		case sks::IPv6:
			return sks::address("[::1]:0", d);
		case sks::unix:
			{
				std::string unixAddressPath = "testing." + std::to_string(index) + ".unix";
				return sks::address("testing.unix", d);
			}
	}
	throw std::runtime_error("No bindable address for domain.");
}

void GivenTheSystemSupports(std::ostream& log, sks::domain d) {
	//We know it is supported if we can create a socket in that domain
	log << "Checking system for *any* " << d << " support" << std::endl;
	sks::type type;
	switch (d) {
		case sks::IPv4:
		case sks::IPv6:
		case sks::unix:
			type = sks::stream;
			break;
		//case sks::ax25:
		//	type = sks::seq;
		//	break;
		default:
			throw std::runtime_error("Unknown domain");
	}
	
	return GivenTheSystemSupports(log, d, type);
}
void GivenTheSystemSupports(std::ostream& log, sks::domain d, sks::type t) {
	log << "Checking system for " << t << " support in the " << d << " domain" << std::endl;
	try {
		sks::socket sock(d, t);
		log << "System supports " << d << " " << t << "s" << std::endl;
	} catch (std::system_error& se) {
		if (se.code() == std::make_error_code(std::errc::address_family_not_supported) ||
		    se.code() == std::make_error_code(std::errc::invalid_argument) ||
		    se.code() == std::make_error_code((std::errc)ESOCKTNOSUPPORT)) {
			
			log << "Test requires " << t << " over " << d << " support." << std::endl;
			throw testing::ignore;
		} else {
			log << "Could not check for socket support" << std::endl;
			throw se;
		}
	} catch (std::exception& e) {
		log << "Could not check for socket support (Non-system error" << std::endl;
		throw e;
	}
}

std::pair<socketAndAddress, socketAndAddress> GetRelatedSockets(std::ostream& log, sks::domain d, sks::type t) {
	//Generate and connect (if applicable) two sockets
	std::mutex logMutex;
	bool accepterReady = false;
	std::pair<socketAndAddress, socketAndAddress> pair;
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
				pair.first.addr = listener.localAddress();

				//Accept a connection
				sks::socket client = listener.accept();
				//clientAddress.second = new sks::address(client.connectedAddress());
				logMutex.lock();
				log << "Acting Client connected from " << client.connectedAddress().name() << std::endl;
				logMutex.unlock();

				//Connected, set structure variables
				std::swap(pair.first.socket, client); //Move client into the structure we will be returning
				pair.first.addr = client.localAddress();
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
				server.connect(pair.first.addr); //Establish connection
				logMutex.lock();
				log << "Acting Client " << server.localAddress().name() << " connected to Acting Server " << server.connectedAddress().name() << std::endl;
				logMutex.unlock();



				//Connected, set structure variables
				std::swap(pair.second.socket, server); //Move client into the structure we will be returning
				pair.second.addr = server.localAddress();
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
		std::swap(pair.first.socket, socketA);
		pair.first.addr = socketA.localAddress();

		sks::socket socketB(d, t);
		socketB.bind(bindableAddress(d, 1));
		std::swap(pair.second.socket, socketB);
		pair.second.addr = socketB.localAddress();
	}

	if (except) {
		//Re-throw up to test manager
		std::rethrow_exception(except);
	}

	return pair;
}
