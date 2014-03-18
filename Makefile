all: client server
	gcc -o client client.o
	gcc -o server server.o
test:client.o
	gcc -o client client.o
	clear
	@./client 127.0.0.1
cli: client.o
	gcc -o client client.o
	clear
	@./client
srv: server.o
	gcc -o server server.o
	clear
	@./server
client: client.c
	gcc -Wall -pedantic -std=c99 -g -c client.c -o client.o
server: server.c
	gcc -Wall -pedantic -std=c99 -g -c server.c -o server.o
clean:
	rm -rf *.o
	rm -rf *.d
	rm -rf *.out
	rm -rf client
	rm -rf server

