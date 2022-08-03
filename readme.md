# SockLib / SocKetS

![Build Status](https://github.com/hunter-land/socklib/workflows/Build/badge.svg) ![Test Status](https://github.com/hunter-land/socklib/workflows/Test/badge.svg)

## About
This library brings a modern C++ interface to sockets, simplifies their use, and doesn't remove any functionality.

### Features
- Addresses can be constructed from formatted strings alone, no explicit `AF_*` argument required (in most cases[⁽¹⁾](#notes)).
- Errors are thrown, never returned.
- Sockets are closed gracefully when deconstructed.
- Send functions block until all data is sent.
- Operating System agnostic[⁽²⁾](#notes)! (Windows and Linux explicitly maintained)
- Supports limited casting/constructing to/from C structures

## Setup
This is an example of setup and setup options showing how to download, build, and install this library on your system of choice.

### Prerequisites
This library is built using `cmake` and at least one of [their supported generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#cmake-generators). CMake can be downloaded via a package manager (i.e. `# apt install cmake` or `# pacman -S cmake`), or directly from [the CMake download page](https://cmake.org/download/).

### Installation

1. Download the source code
    ```bash
    git clone https://github.com/hunter-land/socklib.git
    ```

2. Generate project files with CMake
    ```bash
    cmake -S . -B ./build
    ```
Additional optional arguments include:
- `CMAKE_BUILD_TYPE` which can be set to `Release` (default) or `Debug` with the syntax `-DCMAKE_BUILD_TYPE=value`.
- `BUILD_SHARED_LIBS` can be set to `ON` (default) to shared, or `OFF` for static using the same syntax.

3. Build the generated project (This step varies based on your system and person configuration, below are only examples)
	#### Linux
	Using `make` in a terminal
	```bash
	cd ./build    #cd into whatever directory you had cmake put the files
	make install    #Build and install the library on the system
	```
	#### Windows
	Navigate to the build folder and open the Visual Studio solution (you may need to open it as an administrator to install the library).
	In Visual Studio, right click the `install` target and select `build` to build and install the library.

4. You may need to update your linker cache so it sees the newly installed library (`ldconfig` on Linux).

## Example Usage
Specific usage details are given in the documentation

### Basic, single-connection server
This example program will wait for a client to connect, send a message to the client, receive a message from the client, and close the connection.
```cpp
#include <socks/socks.hpp> //Include this socket library
#include <iostream>
#include <vector>
#include <cstdint>
#include <string>

int main() {
	//Set up a socket for listening
	sks::socket listener(sks::IPv4, sks::stream); //Create a stream socket in the IPv4 domain (TCP)
	listener.bind(sks::address("127.0.0.1:4570")); //Bind to localhost on port 4570
	listener.listen(); //Begin listening for client
	std::cout << "Server is listening on " << listener.localAddress().name() << std::endl;

	//Wait for a client to connect
	sks::socket client = listener.accept(); //Blocks until a client connects
	std::cout << "Client connected from " << client.connectedAddress().name() << std::endl;

	//Send a message to the client
	std::string message = "You have reached the server."; //Message to be sent is this ASCII string
	std::vector<uint8_t> messageData(message.begin(), message.end()); //Convert string into vector of bytes
	client.send(messageData); //Send the message to the client

	//Receive a message from the client
	std::vector<uint8_t> clientMessageData = client.receive(); //Get the message as a vector of bytes
	std::string clientMessage(clientMessageData.begin(), clientMessageData.end()); //We know the message is a string so we create a string out of it
	std::cout << "Client said: " << clientMessage << std::endl; //Display the string

	//Socket is deconstructed automatically, which gracefully closes the connection for us
	return 0;
}
```

### Basic client
This example client program will connect to a server, receive a message from the server, send a message to the server, and close the connection.
```cpp
#include <socks/socks.hpp> //Include this socket library
#include <iostream>
#include <vector>
#include <cstdint>
#include <string>

int main() {
	//Set up socket
	sks::socket server(sks::IPv4, sks::stream); //Create a stream socket in the IPv4 domain (TCP)
	sks::address serverAddress("127.0.0.1:4570"); //Address of the server we will connect to
	server.connect(serverAddress); //Establish connection
	std::cout << "Connected to host " << server.connectedAddress().name() << std::endl;

	//Receive a message from the server
	std::vector<uint8_t> serverMessageData = server.receive(); //Get the message as a vector of bytes
	std::string serverMessage(serverMessageData.begin(), serverMessageData.end()); //We know the message is a string so we create a string out of it
	std::cout << "Server said: " << serverMessage << std::endl; //Display the string

	//Send a message to the server
	std::string message = "Hey server, just saying hello!"; //Message to be sent is this ASCII string
	std::vector<uint8_t> messageData(message.begin(), message.end()); //Convert string into vector of bytes
	server.send(messageData); //Send the message to the server

	//Socket is deconstructed automatically, which gracefully closes the connection for us
	return 0;
}
```
*More examples can be found in the examples directory.*

## Notes
1. Address families/domains which do not have any strict formatting (i.e. `unix`) *do* require a family/domain hint to be passed to an address constructor.
2. Once the [AX.25 Linux rework](https://www.ampr.org/grants/grant-fixing-the-linux-kernel-ax-25/) is completed, the AX25 address will be limited to systems which can support that protocol. Socket interface will remain agnostic.

## Contact
Project Creator
- Hunter Land - [hunterland4.5.7@gmail.com](mailto:hunterland4.5.7@gmail.com) - [Block#2716](https://discordapp.com/users/201452615890894848)

Project Link: https://github.com/hunter-land/socklib
