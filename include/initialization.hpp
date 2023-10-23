#pragma once
#include <cstdlib> //size_t

namespace sks {
	//Initialize and finalize the socket library (when applicable) automatically
	static bool autoInitialize = true;

	namespace {
		static size_t libraryUsers = 0;
	};

	void initialize();
	void deinitialize();
};
