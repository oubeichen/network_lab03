This is the git repository for lab02 of computer network.
lab02 is a experiment for a weather application using socket.

client.c is the source of the client
server.c is the source of the server

make all: Compile server.c to server and client.c to client
make test: Generate and run client(with local server 127.0.0.1, should work with "make srv")
make cli: Generate and run client(with official server 114.212.191.33)
make srv: Generate and run server
