This is the git repository for lab03 of computer network.
lab03 is a experiment for a instant messenger using socket.

client.c is the source of the client
server.c is the source of the server

make all: Compile server.c to server and client.c to client
make test: Generate and run client(with local server 127.0.0.1:6666, should work with "make srv")
make cli: Generate and run client(with default server 127.0.0.1:6666, should work with "make srv")
make srv: Generate and run server

