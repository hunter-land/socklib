#pragma once

namespace sks {

	#if __has_include(<unistd.h>) & __has_include(<sys/socket.h>) //SHOULD be true if POSIX, false otherwise
		#define __SKS_AS_POSIX__ //POSIX system should have BSD sockets
		//const bool posixBsdSockets = true;
	#elif defined _WIN32
		#define __SKS_AS_WINDOWS__ //Use WinSocks
		//const bool posixBsdSockets = false;
	#else
		#error System socket implementation unknown
	#endif

	#if __has_include (<netax25/axlib.h>)
		//NOTE: This will be uncommented, and support added, once the AX25 kernel rework is completed
		//#define __SKS_HAS_AX25__
		//const bool supportsAx25 = true;
	#else
		//const bool supportsAx25 = false;
	#endif

}
