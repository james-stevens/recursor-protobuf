#
# (c) Copyright 2025 JRCS Ltd - All Right Reserved
#
CC=gcc
CFLAGS=-Wall -O2
LDFLAGS=-lprotobuf-c -ljansson
#
OBJS=dnsmessage.pb-c.o protobuf2json.o socket_server.o log_message.o liball.o recursor-protobuf.o
PDNS_SRC=https://raw.githubusercontent.com/PowerDNS/pdns/refs/heads/master/pdns/dnsmessage.proto

all: recursor-protobuf

clean:
	rm -f $(OBJS) dnsmessage.pb-c.c dnsmessage.pb-c.h dnsmessage.proto

dnsmessage.pb-c.c: dnsmessage.proto
	protoc --c_out=. dnsmessage.proto

dnsmessage.pb-c.h: dnsmessage.proto
	protoc --c_out=. dnsmessage.proto

dnsmessage.pb-c.o: dnsmessage.pb-c.c dnsmessage.pb-c.h

protobuf2json.o: protobuf2json.c protobuf2json.h

dnsmessage.proto:
	curl $(PDNS_SRC) > dnsmessage.proto

socket_server.o: socket_server.c socket_server.h liball.h ipall.h log_message.h

log_message.o: log_message.c log_message.h

liball.o: liball.c liball.h

recursor-protobuf.o: recursor-protobuf.c log_message.h

recursor-protobuf: $(OBJS) 
	$(CC) -o recursor-protobuf $(CFLAGS) $(OBJS) $(LDFLAGS)
