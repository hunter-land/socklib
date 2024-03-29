#pragma once
#include "macros.hpp"
#include <cstdlib> //size_t

namespace sks {
	#ifdef _WIN32 //Classic
		#ifndef __MINGW32__ //MinGW is nicer
			#ifdef IS_SKS_SOURCE //Define this if ONLY when building the library
				__declspec(dllexport)
			#else
				__declspec(dllimport)
			#endif
		#endif
	#endif
	extern bool autoInitialize; //Initialize and finalize the socket library (when applicable) automatically

	void initialize();
	void deinitialize();
};
