objects = socks.o
CXXSTD = c++11
CSTD = c17

build : $(objects)

%.o : %.cpp
	$(CXX) -Werror -Wextra -Wall -std=$(CXXSTD) $(CXXFLAGS) -c $^ -o $@
%.o : %.c
	$(CC) -Werror -Wextra -Wall -std=$(CSTD) $(CFLAGS) -c $^ -o $@

test : $(objects) tests/unit.o
	$(CXX) -Werror -Wextra -Wall -std=$(CXXSTD) $(CXXFLAGS) -pthread -o tests/unit tests/unit.o $(objects)
	tests/unit

.PHONY : clean
clean :
	-@rm $(objects)
	-@rm main.o
	-@rm main
	
	-@rm tests/unit.o
	-@rm tests/unit
