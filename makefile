# 046267 Computer Architecture - Spring 2020 - HW #2

cacheSim: cacheSim.cpp
	g++ -std=c++11 -o cacheSim cacheSim.cpp

.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim
