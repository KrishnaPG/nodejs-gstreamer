CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -Iinclude `pkg-config --cflags gstreamer-1.0 json-c zmq`
LIBS = `pkg-config --libs gstreamer-1.0 json-c zmq`

SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)

TARGET = worker

all: $(TARGET)

$(TARGET): $(OBJS)
    $(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
    $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
    rm -f $(OBJS) $(TARGET)

install:
    cp $(TARGET) /usr/local/bin/

.PHONY: all clean install