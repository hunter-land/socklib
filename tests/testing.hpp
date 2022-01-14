#pragma once
#include <string>
#include <ostream>

namespace testing {
	enum tag {
		pass,
		fail,
		ignore,
		
		warn,
		note,
		
		exception,
	};
	
	class assertError : public std::runtime_error {
	protected:
		tag m_tag;
	public:
		assertError(tag t, const char* what);
		tag result() const;
	};
	
	void assertTrue(bool expression, std::string message, tag failResult = fail);
	
	std::ostream& operator<<(std::ostream& os, const tag t);
};
