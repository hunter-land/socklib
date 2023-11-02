#include "errors.hpp"
#include <system_error>
#ifdef _WIN32
	#include <ws2tcpip.h>
#endif

namespace sks {
	#ifdef _WIN32
	//Windows prefixes standard error codes, which causes issues with using std::errc enums defined in the standard (They're smart like that)
	int makeErrnoIfCompatible(int wsaErrorCode) {
		if (wsaErrorCode > 10000 && wsaErrorCode <= 10061) {
			return wsaErrorCode - 10000; //These values are just errno values + 10000
		}
		return wsaErrorCode;
	}
	#endif

	std::system_error sysErr(int errnoInt) {
		#ifdef _WIN32
			errnoInt = makeErrnoIfCompatible(errnoInt);
		#endif
		std::error_code ec = std::make_error_code(static_cast<std::errc>(errnoInt));
		return std::system_error(ec);
	}
};
