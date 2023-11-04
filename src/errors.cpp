#include "errors.hpp"
#include <system_error>
#include "macros.hpp"
#ifdef __SKS_AS_WINDOWS__
	#include <ws2tcpip.h>
#endif

namespace sks {
	#ifdef __SKS_AS_WINDOWS__
		//Windows prefixes standard error codes, which causes issues with using std::errc enums defined in the standard
		std::system_error sysErr(int wsaErrorCode) {
			if (wsaErrorCode > 10000 && wsaErrorCode <= 10061) {
				//These values are just errno values + 10000
				std::error_code ec = std::make_error_code(static_cast<std::errc>(wsaErrorCode - 10000));
				return std::system_error(ec);
			}
			return std::system_error(wsaErrorCode, std::system_category());
		}
	#else
		std::system_error sysErr(int errnoInt) {
			std::error_code ec = std::make_error_code(static_cast<std::errc>(errnoInt));
			return std::system_error(ec);
		}
	#endif
};
