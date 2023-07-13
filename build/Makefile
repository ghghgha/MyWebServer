CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = WebServer_v1.0
OBJS = ../code/pool/*.cpp  \
		../code/buffer/*.cpp \
		../code/timer/*.cpp \
		../code/http/*.cpp \
		../code/log/*.cpp \
        ../code/server/*.cpp \
        ../code/main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)




