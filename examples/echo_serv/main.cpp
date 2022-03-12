#include <iostream>
#include <string>
#include <algorithm>

#include <socks/socks.hpp>

int main()
{
    std::string motd = "Echo Serv Telnet Example\r\n";
    std::vector<uint8_t> motd_message = std::vector<uint8_t>(motd.begin(), motd.end());
    // Create the master socket, handles all incoming connections
    sks::socket master_socket = sks::socket(sks::IPv4, sks::stream);
    // Generate the address parameters for the master socket
    sks::address host = sks::address("localhost:8888");

    // Attempt to bind the address parameter to the master socket, catch if it fails
    try {
        master_socket.bind(host);
    }
    catch (std::exception& e) {
        std::cerr << "Bind Error:\n" << e.what() << std::endl;
    }


    std::cout << "Listener on address " << master_socket.localAddress().name() << std::endl;

    // Try to specify maximum of 3 pending connections for the master socket
    try {
        master_socket.listen(3);
    }
    catch (std::exception& e) {
        std::cerr << "Listen failure:\n" << e.what() << std::endl;
    }

    std::cout << "Waiting for connections..." << std::endl;

    std::vector<sks::socket> clientList;

    while (true)
    {
        // Try to check for new socket connection, and add it to the clientList vector
        try {
            // Is master socket ready to receive a new client socket?
            if (master_socket.readReady())
            {
                // Push new client socket to the back of the clientList vector
                clientList.push_back(master_socket.accept());

                // Get a reference to the last client in the clientList (which will be the last one pushed back)
                sks::socket& lastConnectedClient = clientList.back();
                // Convert to sks::IPv4Address so we can easily pull the IP and Port
                sks::IPv4Address info = (sks::IPv4Address)lastConnectedClient.connectedAddress();

                // Yes, it's ugly.
                std::cout << "New client connect. IP is "
                          << info.addr()[0] << "." << info.addr()[1] << "." << info.addr()[2] << "." << info.addr()[3]
                          << ", Port is " << info.port()
                          << std::endl;

                // Send the motd as a vector of bytes (uint8_t)
                lastConnectedClient.send(motd_message);
            }
        }
        catch (std::exception& e) {
            std::cerr << "Reading data on master_socket FAILED:\n" << e.what() << std::endl;
        }

        for (auto& l : clientList)
        {
            try {
                if (l.readReady())
                {
                    std::string cIP = l.connectedAddress().name();
                    std::vector<uint8_t> bytes = l.receive();

                    if (bytes.empty())
                    {
                        std::cout << cIP << " has disconnected" << std::endl;
                        clientList.erase(std::remove(clientList.begin(), clientList.end(), l), clientList.end());
                        continue;
                    }

                    std::cout << cIP << "> " << std::string(bytes.begin(), bytes.end()) << std::endl;
                    for (auto& cl : clientList)
                    {
                        if (l != cl)
                        {
                            cl.send(bytes);
                        }
                    }
                }
            }
            catch (std::exception& e)
            {
                std::cerr << "Echo error:\n" << e.what() << std::endl;
            }
        }
    }
    return 0;
}