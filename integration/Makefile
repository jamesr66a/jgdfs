CXX=clang++
DEBUG=-g -Wall -Werror -Wunused -pedantic
spawner: gdfs
	$(CXX) $(DEBUG) -std=c++11 -o ./spawner spawner.cpp -lcpprest

gdfs: main.cpp
	$(CXX) $(DEBUG) -std=c++11 -o ./main main.cpp -lfuse -lglog -lcpprest -lgflags -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=27

clean:
	rm spawner main
