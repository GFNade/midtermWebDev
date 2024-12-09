SERVER_TARGET = server
CLIENT_TARGET = client

SERVER_SRC = server.c
CLIENT_SRC = client.c

CC = gcc

CFLAGS = -lpthread

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) -o $@ $^ $(CFLAGS)

$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) -o $@ $^

clean:
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)
