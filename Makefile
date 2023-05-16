all: st_reactor react_server client

client: Client.c
	gcc Client.c -o client

react_server: Server.c 
	gcc Server.c -o react_server -L. -lst_reactor -lpthread

st_reactor: st_reactor.c st_reactor.h
	gcc -c -fPIC st_reactor.c -o st_reactor.o
	gcc -shared -o libst_reactor.so st_reactor.o

clean:
	rm *.o *.so react_server client