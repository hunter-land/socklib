# Example: Telnet Echo Server
This is an example of hosting connections using the Socklib/SKS library.
It supports multiple connections through telnet, and any message sent over telnet will be echoed to every connected telnet client.

Note: This example *does* require Socklib/SKS already be installed.

# Building
## **Linux and WSL**
Running `cmake -S . -B build` will generate a directory (`build/`) which contains a Makefile.

Running `make` inside that (`build/`) directory will yield the example executable.

Telnet can then be used to connect to the server. Example: `telnet localhost 8888`

## **Windows**
*TODO*

## **macOS**
*TODO*