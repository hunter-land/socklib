#pragma once
//Macros to be included in source files (No affect on anyone using the library)

#if __has_include(<unistd.h>) & __has_include(<sys/socket.h>) //SHOULD be true if POSIX, false otherwise
	#define __AS_POSIX__ //POSIX system should have BSD sockets
#elif defined _WIN32
	#define __AS_WINDOWS__ //Use WinSocks
#else
	#error System socket implementation unknown
#endif

#if __has_include (<netax25/axlib.h>)
	#define __HAS_AX25__
#endif
