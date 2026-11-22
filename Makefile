CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -pthread

.PHONY: all run-server run-client run-tests clean

all : build-server build-client build-test

run-server : build-server
	@echo "[Info]: Starting Server"
	@cd server/ && ./server $(args)

run-client : build-client
	@cd client && ./client $(args)

run-test: build-test 
	@echo "[Info]: Starting Tests"
	@./tests/test

build-server : server/server.cpp libraries/tcp.hpp libraries/http.hpp libraries/utils.hpp
	@echo "[Info]: Compiling Server"
	@$(CXX) $(CXXFLAGS) server/server.cpp -o server/server

build-client : client/client.cpp libraries/tcp.hpp libraries/http.hpp libraries/utils.hpp
	
	@$(CXX) $(CXXFLAGS) client/client.cpp -o client/client

build-test : tests/test.cpp
	@echo "[Info]: Compiling Test"
	@$(CXX) $(CXXFLAGS) tests/test.cpp -o tests/test

clean : 
	@echo "[Info]: Removing EXECUTABLE FILES"
	@rm -f client/client server/server tests/test