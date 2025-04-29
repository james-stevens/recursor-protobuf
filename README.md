# recursor-protobuf
Decode PowerDNS Recursor's protobuf to JSON and send to Vector


#################################################################

WORK IN PROGRESS - Don't even attempt to expect this to work, yet

#################################################################


Protobuf to parse
- https://github.com/PowerDNS/pdns/blob/master/pdns/dnsmessage.proto

Downloads actual `.proto` file from here
- https://raw.githubusercontent.com/PowerDNS/pdns/refs/heads/master/pdns/dnsmessage.proto


Before you start, install these
- protobuf-c
- protobuf-c-compiler
- protobuf-c-dev
- jansson
- jansson-dev
