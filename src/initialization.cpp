#include "initialization.hpp"
#include "errors.hpp"
#include "macros.hpp"
extern "C" {
	#ifdef __SKS_AS_WINDOWS__
		#include <winsock2.h>
	#endif
}

namespace sks {
	bool autoInitialize = true;
	static size_t libraryUsers = 0;

	void initialize() {
		if (libraryUsers == 0) {
			#ifdef __SKS_AS_WINDOWS__
				// Initialize Winsock
				WSADATA wsaData;
				int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData); //Initialize version 2.2
				if (iResult != 0) {
					throw sysErr(iResult);
				}
			#endif
		}
		libraryUsers++;
	}
	void deinitialize() {
		libraryUsers--;
		if (libraryUsers == 0) {
			#ifdef __SKS_AS_WINDOWS__
				//Clean up Winsock
				int e = WSACleanup();
				if (e != 0) {
					throw sysErr(WSAGetLastError());
				}
			#endif
		}
	}
};
