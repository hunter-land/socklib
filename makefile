objects = socks.o
CXXSTD = c++20
CSTD = c17


main : $(objects) main.o
	$(CXX) -Werror -Wextra -Wall -std=$(CXXSTD) $(CXXFLAGS) -o main main.o $(objects)

%.o : %.cpp
	$(CXX) -Werror -Wextra -Wall -std=$(CXXSTD) $(CXXFLAGS) -c $^ -o $@
%.o : %.c
	$(CC) -Werror -Wextra -Wall -std=$(CSTD) $(CFLAGS) -c $^ -o $@

test : $(objects) tests/unit.o
	$(CXX) -Werror -Wextra -Wall -std=$(CXXSTD) $(CXXFLAGS) -o tests/unit tests/unit.o $(objects)
	tests/unit

.PHONY : clean
clean :
	rm $(objects)
	rm main.o
	rm main
	
	rm tests/unit.o
	rm tests/unit
