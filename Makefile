build-server: bin/ server.c
	gcc server.c -o ./bin/server

build-client: bin/ client.c
	gcc client.c -o ./bin/client

build: build-server build-client

run-server: ./bin/server
	./bin/server

run-client: ./bin/server
	./bin/client
