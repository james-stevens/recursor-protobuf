OBJS=dnsmessage.pb-c.o
PDNS_SRC=https://raw.githubusercontent.com/PowerDNS/pdns/refs/heads/master/pdns/dnsmessage.proto

all: $(OBJS)

clean:
	rm -f $(OBJS) dnsmessage.pb-c.c dnsmessage.pb-c.h dnsmessage.proto

dnsmessage.pb-c.c: dnsmessage.proto
	protoc --c_out=. dnsmessage.proto

dnsmessage.pb-c.h: dnsmessage.proto
	protoc --c_out=. dnsmessage.proto

dnsmessage.pb-c.o: dnsmessage.pb-c.c dnsmessage.pb-c.h

dnsmessage.proto:
	wget -nv $(PDNS_SRC)
