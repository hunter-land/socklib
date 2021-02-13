objects = main.o socks.o
CXXSTD = c++20
CSTD = c17


main : $(objects)
	$(CXX) -Werror -Wextra -Wall -std=$(CXXSTD) $(CXXFLAGS) -o main  $(objects)

%.o : %.cpp
	$(CXX) -Werror -Wextra -Wall -std=$(CXXSTD) $(CXXFLAGS) -c $^ -o $@
%.o : %.c
	$(CC) -Werror -Wextra -Wall -std=$(CSTD) $(CFLAGS) -c $^ -o $@


.PHONY : clean
clean :
	rm $(objects) main
