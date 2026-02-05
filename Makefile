build-server: bin/ src/transport.c
	gcc ./src/transport.c -o ./bin/transport

build-client: bin/ src/client.c
	gcc ./src/client.c -o ./bin/client

build: build-server build-client

run-server:
	./bin/transport

run-client:
	./bin/client
