build-server: bin/ src/server/transport.c
	gcc ./src/server/transport.c ./src/server/http-protocal.c ./src/server/main.c ./src/server/transport.h -o ./bin/transport

build-client: bin/ src/client/client.c
	gcc ./src/client/client.c -o ./bin/client

build: build-server build-client

run-server:
	./bin/transport

run-client:
	./bin/client
