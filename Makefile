SRCS = $(wildcard *.cpp)
APPS = $(patsubst %.cpp,build/%,$(SRCS))

all: $(APPS)

build/%: %.cpp
	mkdir -p build
	g++ -Wall -g -std=c++17 $< -o $@

clean:
	rm -rf build