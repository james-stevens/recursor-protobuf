# recursor-protobuf
Decode PowerDNS Recursor's protobuf to JSON and send to Vector

#####################################################################

I'm still working on this, but it actually fully works right now
I'd recommend using `-l x200003` to surpress debugging messages

Still need to add
- collecting metrics & writing to a Prometheus file
- checking on connection to vector
- tidy up debugging code

#####################################################################


## This is wher the `proto` file came from

Protobuf to parse
- https://github.com/PowerDNS/pdns/blob/master/pdns/dnsmessage.proto

Downloads `dnsmessage.proto` file itself from here, using `curl`
- https://raw.githubusercontent.com/PowerDNS/pdns/refs/heads/master/pdns/dnsmessage.proto

I've included the `proto` file in this code as it would not compile without it
but if you remove it, the `Makefile` should just re-curl it from that second URL.


## Required Packages To Compaile

If you want to compile this, you will need to install these
- protobuf-c
- protobuf-c-compiler
- protobuf-c-dev
- jansson
- jansson-dev

I compiled with `gcc version 12.2.1 20220924` on Alpine Linux v3.18, best of luck with anything else - shouldn't be to hard!


## Example config
These config files are all you need to test this code out. For production use you'll want all sorts of other paramters.

### PDNS Recursor
- recursor.conf
- recursor.lua

### Vector (https://vector.dev)
- vector.yaml

with this `vector.yaml` you'd need to also add `-o 127.0.0.1:9000` to make this code connect to `vector`
