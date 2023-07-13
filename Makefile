all:
	mkdir -p bin
	cd build && make
	./bin/WebServer_v1.0
