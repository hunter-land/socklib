#include "testing.hpp"
#include <map>
#include <string>
#include <ostream>

namespace testing {
	assertError::assertError(tag t, const char* what) : std::runtime_error(what) {
		m_tag = t;
	}
	tag assertError::result() const {
		return m_tag;
	}
	
	void assertTrue(bool expression, std::string message, tag failResult) {
		if (!expression) {
			//Not according to plan, do the thing
			throw assertError(failResult, message.c_str());
		}
	}
	
	std::ostream& operator<<(std::ostream& os, const tag t) {
		static std::map<tag, std::string> tags = {
			{tag::pass,   "[\033[32m PASS \033[0m]"},
			{tag::fail,   "[\033[31m FAIL \033[0m]"},
			{tag::ignore, "[\033[37m IGND \033[0m]"},
			
			{tag::note,   "[\033[36m NOTE \033[0m]"},
			{tag::warn,   "[\033[35m WARN \033[0m]"},
			
			{tag::exception,   "[\033[31m\033[1m EXCP \033[0m]"},
		};
		
		os << tags[t];
		return os;
	}
};
