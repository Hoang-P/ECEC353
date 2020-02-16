SERVER	:= chat_server.c
CLIENT := chat_client.c
CC	:= gcc
SERVER_TARGET	:= hdp38_njs76_chat_server
CLIENT_TARGET	:= chat_client
LINK	:= -std=c99 -Wall -lrt

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER)
	$(CC) -o $(SERVER_TARGET) $(SERVER) $(LINK)

$(CLIENT_TARGET): $(CLIENT)
	$(CC) -o $(CLIENT_TARGET) $(CLIENT) $(LINK)

clean:
	rm -f /dev/mqueue/hdp38_njs76_c* $(CLIENT_TARGET) $(SERVER_TARGET)