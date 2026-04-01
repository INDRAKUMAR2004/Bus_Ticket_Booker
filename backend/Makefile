CC=gcc
CFLAGS=-Wall -O2

# Detect OS to include ws2_32 on Windows only, without polluting Linux
ifeq ($(OS),Windows_NT)
	LDFLAGS=-lws2_32
	EXE=.exe
else
	LDFLAGS=
	EXE=
endif

all: server

server: server.c routes.c utils.c
	$(CC) $(CFLAGS) server.c routes.c utils.c -o server$(EXE) $(LDFLAGS)

clean:
	rm -f server server.exe
