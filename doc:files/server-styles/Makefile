.PHONY: all
all:
	g++ -Wall -g -std=c++11 -o client client.cpp -lcurses
	g++ -Wall -g -std=c++11 -o server-single-threaded server-single-threaded.cpp
	g++ -Wall -g -std=c++11 -o server-multiplexed server-multiplexed.cpp
	g++ -Wall -g -std=c++11 -pthread -o server-multithreaded server-multithreaded.cpp

.PHONY: clean
clean:
	rm -f *.o client server-single-threaded server-multithreaded server-multiplexed core.*