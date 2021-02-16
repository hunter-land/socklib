# SockLib / SocKetS

![Build Status](https://github.com/hunter-land/socklib/workflows/Build/badge.svg)

SocKetS is a C++ wrapping of the standard C sockets, bringing modern interfacing to sockets.

  - System Agnostic
  - Better error handling and returning
  - Easily replaces C socket calls and code
  - Supports C++11 standard onwards

### Usage

All data is contained within the namespace `sks::`
  - `sks::socket_base` is the socket class, which is assigned a
    - Domain of either UNIX, IPv4, or IPv6.
    - Type of either TCP, UDP, RDM, SEQ, or raw.
    - Protocol ID. (Optional)

Function names and purpose are kept similar to other languages and implementations. (`listen()`, `accept()`, `bind()`, `connect()`, `sendto()`, and `recvfrom()`)
