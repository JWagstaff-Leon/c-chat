bin/server : bin/main.o bin/server.o bin/connections.o bin/messages.o
	gcc bin/main.o bin/server.o bin/connections.o bin/messages.o -o bin/server

bin/main.o : src/main.c inc/connections.h inc/messages.h ../pub/event.h
	gcc -Iinc -I../pub -c src/main.c -o bin/main.o

bin/server.o : inc/server.h src/server.c connections.h
	gcc -Iinc src/server.c -o bin/server.c

bin/connections.o : inc/connections.h src/connections.c inc/user.h ../pub/event.h
	gcc -Iinc -I../pub -c src/connections.c -o bin/connections.o

bin/messages.o : inc/messages.h src/messages.c
	gcc -Iinc -c src/messages.c -o bin/messages.o

clean :
	rm -r bin/*