#pragma once
#include <cstdlib> //size_t

namespace sks {
	#ifdef _WIN32 //Classic
		#ifdef IS_SKS_SOURCE //Define this if ONLY when building the library
			__declspec(dllexport)
		#else
			__declspec(dllimport)
		#endif
	#endif
	extern bool autoInitialize; //Initialize and finalize the socket library (when applicable) automatically

	void initialize();
	void deinitialize();

	//Primarily for internal use by sockets. Library user most likely doesn't need to call these
	void incremenetUserCounter();
	void decremenetUserCounter();
};
