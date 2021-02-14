#include <iostream>
#include "socks.hpp"
#include <string>
#include <thread>
#include <chrono>

const unsigned short port = 5555; //Listener test port

void printinfo() {
	std::string std = std::to_string(__cplusplus);
	switch (__cplusplus) {
		case 1:
			std = "Pre C++98";
			break;
		case 199711L:
			std = "C++98";
			break;
		case 201103L:
			std = "C++11";
			break;
		case 201402L:
			std = "C++14";
			break;
		case 201703L:
			std = "C++17";
			break;
		case 202002L:
			std = "C++20";
			break;
	}
	
	std::cout << "Compiled " << __DATE__ << " at " << __TIME__ << std::endl;
	std::cout << "Standard version: " << std << std::endl;
}
void testTCPHost();
void testTCPHost2();
void testTCPClient();
void testUDPRX();
void testUDPTX();
void testInvalidConstruction();

int reversePacket(sks::packet& pkt);

int main() {
	//std::cout << "No program here yet..." << std::endl;
	std::cout << "What program do you want to run:" << std::endl;
	std::cout << "\t1: testTCPHost2();" << std::endl;
	std::cout << "\t2: testTCPClient();" << std::endl;
	std::cout << "\t3: testUDPRX();" << std::endl;
	std::cout << "\t4: testUDPTX();" << std::endl;
	std::cout << "\t5: testTCPHost();" << std::endl;
	std::cout << "\t6: testInvalidConstruction();" << std::endl;
	short inputshort;
	std::cin >> inputshort;
	switch (inputshort) {
		case 0:
			printinfo();
			break;
		case 1:
			testTCPHost2();
			break;
		case 2:
			testTCPClient();
			break;
		case 3:
			testUDPRX();
			break;
		case 4:
			testUDPTX();
			break;
		case 5:
			testTCPHost();
			break;
		case 6:
			testInvalidConstruction();
			break;
		default:
			std::cout << "Unknown option " << inputshort << std::endl;
			std::cout << "Exiting..." << std::endl;
			break;
	}
	
	

	return 0;
}

void testTCPHost() {
	std::cout << "testTCPHost()" << std::endl;
	
	//Create host socket
	sks::socket_base h(sks::domain::ipv4, sks::protocol::tcp);
	//Replaced by excption in constructor:
	/*
	if (h.errorNo() != 0) {
		std::cerr << "Failed to construct `h`: " << h.error() << " (" << h.errorNo() << ")" << std::endl;
	}
	*/
	
	//Bind to port `port`
	int r = h.bind(port);
	if (r != 0) {
		std::cerr << "Failed to bind to port " << port << ": " << sks::errorstr(r) << " (" << r << ")" << std::endl;
	}
	
	//Enter listen mode
	r = h.listen();
	if (r != 0) {
		std::cerr << "Failed to start listening: " << sks::errorstr(r) << " (" << r << ")" << std::endl;
	}
	
	//Wait for connection
	std::cout << "Waiting for connection on TCP@" << h.locaddr().addrstring << ":" << h.locaddr().port << std::endl;
	sks::socket_base c = h.accept();
	std::cout << "Client has connected from " << c.remaddr().addrstring << ":" << c.remaddr().port << std::endl;
	
	//Read incoming data
	while (c.valid()) {
		//if (c.availdata() > 0) {
			//Print data as ASCII characters
			std::vector<uint8_t> data;
			sks::serror e = c.recvfrom(data);
			if (e.erno == 0) {
				data.push_back(0);
				std::cout << (char*)data.data() << std::endl;
			} else {
				std::cerr << "Error: " << sks::errorstr(e) << std::endl;
			}
		///}
	}
	
	//std::cout << "Connection closed: " << c.error() << " (" << c.errorNo() << ")" << std::endl;
}

void testTCPHost2() {
	std::cout << "testTCPHost2()" << std::endl;
	
	//Create host socket
	sks::socket_base h(sks::domain::ipv4, sks::protocol::tcp);
	//Replaced by excption in constructor:
	/*
	if (h.errorNo() != 0) {
		std::cerr << "Failed to construct `h`: " << h.error() << " (" << h.errorNo() << ")" << std::endl;
	}
	*/
	
	//Bind to port `port`
	int r = h.bind(port);
	if (r != 0) {
		std::cerr << "Failed to bind to port " << port << ": " << sks::errorstr(r) << " (" << r << ")" << std::endl;
	}
	
	//Enter listen mode
	r = h.listen();
	if (r != 0) {
		std::cerr << "Failed to start listening: " << sks::errorstr(r) << " (" << r << ")" << std::endl;
	}
	
	//Apply pre and post functions
	h.setpre(&reversePacket);
	h.setpost(reversePacket);
	std::cout << "Using reverse byte-order pre and post functions." << std::endl;
	
	//Wait for connection
	std::cout << "Waiting for connection on TCP@" << h.locaddr().addrstring << ":" << h.locaddr().port << std::endl;
	sks::socket_base c = h.accept();
	std::cout << "Client has connected from " << c.remaddr().addrstring << ":" << c.remaddr().port << std::endl;
		
	//Read incoming data
	while (c.valid()) {
		std::string ascii = "I am a TCP server. State your business.";
		std::vector<uint8_t> data(ascii.begin(), ascii.end());
		sks::serror e_send = c.sendto(data);
		if (e_send.erno == 0) {
			data.push_back(0);
			std::cout << "Sent " << data.size() << " bytes to " << c.remaddr().addrstring << ":" << c.remaddr().port <<  ": " << (char*)data.data() << std::endl;
		} else {
			std::cerr << "Send error: " << sks::errorstr(e_send) << std::endl;
		}
		
		//Print data as ASCII characters
		sks::packet pkt;
		sks::serror e_recv = c.recvfrom(pkt);
		if (e_recv.erno == 0) {
			pkt.data.push_back(0);
			std::cout << pkt.data.size() << " bytes from " << pkt.rem.addrstring << ":" << pkt.rem.port << ": " << (char*)pkt.data.data() << std::endl;
		} else {
			std::cerr << "Recv error: " << sks::errorstr(e_recv) << std::endl;
		}
	}

	//std::cout << "Connection closed: " << c.errorstr() << " (" << c.errorNo() << ")" << std::endl;
}

void testTCPClient() {
	std::cout << "testTCPClient()" << std::endl;
	
	//Create host socket
	sks::socket_base c(sks::domain::ipv4, sks::protocol::tcp);
	//Replaced by excption in constructor:
	/*
	if (c.errorNo() != 0) {
		std::cerr << "Failed to construct `c`: " << c.error() << " (" << c.errorNo() << ")" << std::endl;
	}
	*/
	
	//Construct address struct
	sks::sockaddress rem;
	rem.d = sks::ipv4;
	rem.addrstring = "10.0.0.150";
	std::cout << "IP = " << rem.addrstring << "; Port = ";
	std::cin >> rem.port;
	
	//Attempt to connect
	int r = c.connect(rem);
	if (r != 0) {
		std::cerr << "Failed to connect to TCP@" << rem.addrstring << ":" << rem.port << ": " << sks::errorstr(r) << std::endl;
	} else {
		std::cout << "Connected to " << c.remaddr().addrstring << ":" << c.remaddr().port << std::endl;
	}
	
	//Set a 5s timeout for rx and tx
	c.setrxtimeout(5000000);
	c.settxtimeout(5000000);
	
	//Read incoming data
	while (c.valid()) {
		sks::packet pkt;
		sks::serror e_recv = c.recvfrom(pkt);
		if (e_recv.erno == 0) {
			pkt.data.push_back(0);
			std::cout << pkt.data.size() << " bytes from " << pkt.rem.addrstring << ":" << pkt.rem.port << ": " << (char*)pkt.data.data() << std::endl;
		} else {
			std::cerr << "Error: " << sks::errorstr(e_recv) << std::endl;
		}
		
		std::string ascii = "I am a TCP client. Business?";
		std::vector<uint8_t> data(ascii.begin(), ascii.end());
		sks::serror e_send = c.sendto(data);
		if (e_send.erno == 0) {
			data.push_back(0);
			std::cout << "Sent " << data.size() << " bytes to " << c.remaddr().addrstring << ":" << c.remaddr().port <<  ": " << (char*)data.data() << std::endl;
		} else {
			std::cerr << "Error: " << sks::errorstr(e_send) << std::endl;
		}
		
		std::this_thread::sleep_for( std::chrono::seconds(1) );
	}
	
	//std::cout << "Connection closed: " << c.error() << " (" << c.errorNo() << ")" << std::endl;
}

void testUDPRX() {
	std::cout << "testUDPRX()" << std::endl;
	
	//Create socket
	sks::socket_base s(sks::domain::ipv4, sks::protocol::udp);
	//Replaced by excption in constructor:
	/*
	if (s.errorNo() != 0) {
		std::cerr << "Failed to construct `s`: " << s.error() << " (" << s.errorNo() << ")" << std::endl;
	}
	*/
	
	//Bind to port `port`
	int r = s.bind(port);
	if (r != 0) {
		std::cerr << "Failed to bind to port " << port << ": " << sks::errorstr(r) << " (" << r << ")" << std::endl;
	}
	
	//Wait for data
	std::cout << "Waiting for data on UDP@" << s.locaddr().addrstring << ":" << s.locaddr().port << std::endl;
	
	//Set a 5s timeout for rx
	s.setrxtimeout(5000000);
	
	//Enable receiving broadcasts
	s.setbroadcastoption(true);
	
	//Read incoming data
	while (s.valid()) {
		sks::packet pkt;
		sks::serror e = s.recvfrom(pkt);
		if (e.erno == 0) {
			pkt.data.push_back(0);
			std::cout << pkt.data.size() << " bytes from " << pkt.rem.addrstring << ":" << pkt.rem.port << ": " << (char*)pkt.data.data() << std::endl;
		} else {
			if (e.type != sks::errortype::BSD && e.erno != EAGAIN) {
				std::cerr << "Error: " << sks::errorstr(e) << std::endl;
			}
		}
	}
}

void testUDPTX() {
	std::cout << "testUDPRX()" << std::endl;
	
	//Create socket
	sks::socket_base s(sks::domain::ipv4, sks::protocol::udp);
	//Replaced by excption in constructor:
	/*
	if (s.errorNo() != 0) {
		std::cerr << "Failed to construct `s`: " << s.error() << " (" << s.errorNo() << ")" << std::endl;
	}
	*/
	
	//Bind to any port
	int r = s.bind();
	if (r != 0) {
		std::cerr << "Failed to bind: " << sks::errorstr(r) << " (" << r << ")" << std::endl;
	}
	
	//Create datagram to send
	sks::packet dg;
	dg.rem.addrstring = "localhost";
	dg.rem.d = sks::ipv4;
	std::cout << "IP = " << dg.rem.addrstring << "; Port = ";
	std::cin >> dg.rem.port;
	uint8_t i = 0;
	std::string ascii = "This line is being sent once every second. i";
	dg.data = std::vector<uint8_t>(ascii.begin(), ascii.end());
	
	//Send data every 1 second
	while (s.valid()) {
		//Set i
		dg.data[dg.data.size() - 1] = i + '0';
		sks::serror e = s.sendto(dg);
		if (e.erno == 0) {
			dg.data.push_back(0);
			std::cout << "Sent " << dg.data.size() << " bytes to " << dg.rem.addrstring << ":" << dg.rem.port <<  ": " << (char*)dg.data.data() << std::endl;
			dg.data.resize(ascii.size());
		} else {
			std::cerr << "Failed to send data: " << sks::errorstr(e) << std::endl;
		}
		
		std::this_thread::sleep_for( std::chrono::seconds(1) );
		i = (i + 1) % 10;
	}
}

void testInvalidConstruction() {
	std::cout << "testInvalidConstruction()" << std::endl;
	
	//Ew, a try-catch in C++
	try {
		//Attempt to create socket with bad arguments
		sks::socket_base h(sks::domain::ipv4, (sks::protocol)(sks::protocol::tcp + 3));
		
		std::cerr << "Failed: No error caught!" << std::endl;
	} catch (std::runtime_error& e) {
		std::cout << "Runtime error '" << e.what() << "' caught." << std::endl;
	} catch (std::exception& e) {
		std::cout << "Non-runtime error '" << e.what() << "' caught. Expected a runtime error." << std::endl;
	}
	
	
}

int reversePacket(sks::packet& pkt) {
	std::reverse(pkt.data.begin(), pkt.data.end());
	return 0;
}

//