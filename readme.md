# SockLib / SocKetS

![Build Status](https://github.com/hunter-land/socklib/workflows/Build/badge.svg) ![Test Status](https://github.com/hunter-land/socklib/workflows/Test/badge.svg)


## About
I have often found myself needing to use sockets in C++, but having no easy access to them.
The typical options are either to use C sockets and change the style of the code to C for that section, or to download a larger-than-it-needs-to-be library and learn new socket semantics just for their library which abstracted away things you need control over.
SocKetS solves these issues, by wrapping C sockets, making them system agnostic, and keeping it object-oriented but still lower-level, just like the C++ standard libraries.

<!--The SocKetS library is system agnostic and can easily replace C sockets in your C++ code (C++11 onwards).
  - System Agnostic (WinSocks are a thing of the past)
  - Better error handling and returning (No more global errno checking)
  - Easily replaces C socket calls
  - Supports C++11 standard onwards-->

Many function names and purpose are kept obvious and similar to most socket implementations. (e.g.: `bind()`, `sendto()`)


## Setup
This is an example of setup and options involved showing how to download, build, and install this library on your system.

### Prerequisites
This library is built using `cmake` and assumes you have it installed. If you do not, you can download it from:
- Your system's package manager (such as `apt`)
- [The official CMake website](https://cmake.org/)

### Installation
CMake is used to produce build files and can be from root directory with
1. Download the source code
	```bash
	git clone https://github.com/hunter-land/socklib.git
	```
2. Run CMake
	`CMAKE_BUILD_TYPE` can be set to `Release` (default) or `Debug` with `-DCMAKE_BUILD_TYPE=<value>`
    Shared vs Static can selected with `-DBUILD_SHARED_LIBS=<value>` with `ON` for shared and `OFF` for static
	```bash
	#This calls cmake, which will place system-specific build files in the build directory (./build)
	cmake -S . -B ./build
	```
3. Build the generated project (This step varies based on your system and configuration, but the rule of thumb for each system is given)
	#### Linux
	```bash
	cd ./build    #cd into whatever directory you had cmake put the files
	make install    #Build and install the library on the system
	```
	#### Windows
	Navigate to the build folder and open the Visual Studio solution (you may need to open it as an administrator).
	In Visual Studio, you can either build all targets or right click the `install` target and select `build`
4. Update your linker so it sees the newly installed library. 
	```bash
	ldconfig        #creates the necessary links and cache
	```


## Usage
Specific usage details are given in the documentation
### Windows considerations
Windows requires the program to initialize their sockets, so you must include intialization in your code before any calls to the library. An example code block is provided below.
```cpp
// Initialize Winsock
WSADATA wsaData;
int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); //Initialize version 2.2
if (iResult != 0) {
	std::cerr << "WSAStartup failed: " << iResult << std::endl;
	return 1;
}
```
### Basic, single-connection server
Wait for a client to connect on a given port, send a message, and wait for a single reply before closing the connection.
```cpp
#include <socks.hpp>
#include <iostream>
#include <vector>
#include <cstdint>
#include <string>

int main() {
	//Set up listener socket
	sks::socket_base listener(sks::ipv4, sks::tcp); //Create a TCP socket for ipv4 addresses to conenct to
	sks::serror err = listener.bind(457); //Listen on port 457
	err = listener.listen();

	//Wait for a client to connect
	sks::socket_base client = listener.accept();

	//Send a message to the client
	std::string message = "Hello client!"; //Message to be sent is this ascii string
	std::vector<uint8_t> messageData(message.begin(), message.end()); //Convert string into vector of bytes
	err = client.sendto(messageData); //Send it

	//Wait to hear something back, store it in the recvData vector
	std::vector<uint8_t> recvData;
	err = client.recvfrom(recvData);
	//Interpret message as a string
	std::string recvMessage(recvData.begin(), recvData.end());

	std::cout << "Client says " << recvMessage << std::endl;

	//Sockets are closed gracefully when deconstructed, so we don't need to manually close them here
	return 0;
}
```
### Basic client
Connect to an ipv4 address and port, receive a message, reply to it, and close the connection.
```cpp
#include <socks.hpp>
#include <iostream>
#include <vector>
#include <cstdint>
#include <string>

int main() {
	//Set up socket
	sks::socket_base socket(sks::ipv4, sks::tcp); //Create a TCP socket to connect to an ipv4 address
	sks::sockaddress remoteAddress("127.0.0.1", 457); //We will be connecting to the localhost on port 457
	sks::serror err = socket.connect(remoteAddress);

    //Wait for the server to start the conversation
	std::vector<uint8_t> recvData;
	err = socket.recvfrom(recvData);

	//Interpret message as a string
	std::string recvMessage(recvData.begin(), recvData.end());
	std::cout << "Server says " << recvMessage << std::endl;

	//Reply to the server
	std::string message = "Hey server!"; //Message to be sent is this ascii string
	std::vector<uint8_t> messageData(message.begin(), message.end()); //Convert string into vector of bytes
	err = socket.sendto(messageData); //Send it

	//Socket is closed gracefully when deconstructed, no code needed here
	return 0;
```
*For more examples and information, please view the full documentation.*


## Contact
Hunter Land - hunterland4.5.7@gmail.com - [Block#2716](https://discordapp.com/users/201452615890894848)
Project Link: https://github.com/hunter-land/socklib