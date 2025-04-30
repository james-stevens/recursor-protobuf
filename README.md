# recursor-protobuf
Decode PowerDNS Recursor's protobuf to JSON and send to Vector


#################################################################

WORK IN PROGRESS - Don't even attempt to expect this to work, yet

#################################################################


Protobuf to parse
- https://github.com/PowerDNS/pdns/blob/master/pdns/dnsmessage.proto

Downloads `dnsmessage.proto` file itself from here, using `curl`
- https://raw.githubusercontent.com/PowerDNS/pdns/refs/heads/master/pdns/dnsmessage.proto


If you want to compile this, you will need to install these
- protobuf-c
- protobuf-c-compiler
- protobuf-c-dev
- jansson
- jansson-dev

I compiled with `gcc version 12.2.1 20220924` on Alpine Linux v3.18, best of luck with anything else - shouldn't be to hard!


## Frame Stream Format
### Frame Streams Control Frame Format - Data frame length equals 00 00 00 00

```
|------------------------------------|----------------------|
| Data frame length                  | 4 bytes              |  
|------------------------------------|----------------------|
| Control frame length               | 4 bytes              |
|------------------------------------|----------------------|
| Control frame type                 | 4 bytes              |
|------------------------------------|----------------------|
| Control frame content type         | 4 bytes (optional)   |
|------------------------------------|----------------------|
| Control frame content type length  | 4 bytes (optional)   |
|------------------------------------|----------------------|
| Content type payload               | xx bytes             |     
|------------------------------------|----------------------|
```

### Frame Streams Data Frame Format

```
|------------------------------------|----------------------|
| Data frame length                  | 4 bytes              |
|------------------------------------|----------------------|
| Payload - Protobuf                 | xx bytes             |
|------------------------------------|----------------------|
```
