#pragma once
#include <cstdint>

namespace sks {
	//Initialize and finalize the socket library (when applicable) automatically
	static bool autoInitialize = true;

	namespace {
		static std::size_t libraryUsers = 0;
	};

	void initialize();
	void deinitialize();
};
