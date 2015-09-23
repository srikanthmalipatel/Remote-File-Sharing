#
# Makefile
# SrikanthMalipatel, 2015-09-22 23:33
#


all: napster

napster: driver.o server.o client.o
	g++ -g driver.o server.o client.o -o napster

driver.o: driver.cpp
	g++ -g -c driver.cpp

server.o: server.cpp
	g++ -g -c server.cpp

client.o: client.cpp
	g++ -g -c client.cpp

clean:
	rm *.o hello

# vim:ft=make
#
