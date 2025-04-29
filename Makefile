OBJS=dnsmessage.pb-c.o

all: $(OBJS)

clean:
	rm -f $(OBJS) dnsmessage.pb-c.c dnsmessage.pb-c.h

dnsmessage.pb-c.c: dnsmessage.proto
	protoc --c_out=. dnsmessage.proto

dnsmessage.pb-c.h: dnsmessage.proto
	protoc --c_out=. dnsmessage.proto

dnsmessage.pb-c.o: dnsmessage.pb-c.c dnsmessage.pb-c.h
