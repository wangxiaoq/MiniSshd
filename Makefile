all: server client

server: mini-sshd.c util.h
	gcc $< -o $@ -lpthread

client: ssh-client.c util.h
	gcc $< -o $@

.PHONY: clean
clean:
	rm server client
