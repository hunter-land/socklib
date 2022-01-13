#include "include/errors.hpp"
#include <system_error>

namespace sks {
	std::system_error sysErr(int errnoInt) {
		std::error_code ec = std::make_error_code(static_cast<std::errc>(errnoInt));
		return std::system_error(ec);
	}
};
