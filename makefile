obj = server_options.o web_server.o
cc = gcc
CFLAGS +=

web_server:$(obj)
	$(cc) $(CFLAGS) -o web_server $(obj) -lpthread

server_options.o : server_options.h server_options.c
	$(cc) $(CFLAGS) -c server_options.c
web_server.o : server_options.h web_server.h web_server.c
	$(cc) $(CFLAGS) -c web_server.c

.PHONY: clean
clean:
	rm web_server $(obj)
