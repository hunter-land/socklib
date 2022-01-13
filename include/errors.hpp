#pragma once
#include <system_error>

namespace sks {
	//Turn an errno error value into a C++ throwable exception type
	std::system_error sysErr(int errnoInt);
};
