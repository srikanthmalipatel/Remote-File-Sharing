#
# Makefile
# SrikanthMalipatel, 2015-09-22 23:33
#


all: proj1

proj1: smalipat_proj1.o server.o client.o base.o
	g++ -g smalipat_proj1.o server.o client.o base.o -o proj1

smalipat_proj1.o: smalipat_proj1.cpp
	g++ -g -c smalipat_proj1.cpp

server.o: server.cpp
	g++ -g -c server.cpp

client.o: client.cpp
	g++ -g -c client.cpp
	
base.o: base.cpp
	g++ -g -c base.cpp

clean:
	rm *.o proj1

# vim:ft=make
#
