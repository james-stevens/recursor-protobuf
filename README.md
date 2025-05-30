# recursor-protobuf
Decode PowerDNS Recursor's protobuf to JSON and send to Vector

# Special Thanks
Special thanks to [Oleg Efimov](https://github.com/Sannis/protobuf2json-c) for the following files
- base64.h
- bitmap.h
- protobuf2json.c
- protobuf2json.h

These provide a standard way to convert a protobuf record into a JSON [Jansson](https://jansson.readthedocs.io/en/latest/index.html) record.

Oleg Efimov's code is provided unaltered as-is with the original MIT License.

All other code was written by me, apart from `dnsmessage.proto` provided by PowerDNS.


# Introduction

If you run [PowerDNS Recursor](https://docs.powerdns.com/recursor/index.html) as a recursive resolver, it can output all the
query and response records, but it does this with a proprietary [streaming protobuf](https://docs.powerdns.com/recursor/lua-config/protobuf.html) format.

This becomes especially necessary if you are using it to do [RPZ filtering](https://docs.powerdns.com/recursor/lua-config/rpz.html),
as the only way to capture information about queries it blocks is using this interface.

What this program does is listen on it's input for connections from PowerDNS Recursor, decodes the protobuf data into JSON, then connects
to it's output, to a listening destination, to send it the `<LF>` separated JSON.

I use [Vector](https://vector.dev) as the destination listener and
I have included an [example vector.yaml](/configs/vector.yaml) config file to show how this can be done.
But you could use any destination that can listen on a plain (non-SSL) socket and accept `<LF>` separated JSON.

Vector gives a wide range of options for enriching, filtering and exporting the data stream, so is a great tool for this use case.


# How It Works
The architecture is really simple, but is efficient (on Linux) and gives you reasonable control.

When PowerDNS Recursor creates a new thread, that thread will make a connection to this program's control process, which
will immediately `fork()` a child process to collect & process the incoming protobuf stream.

It's a bit old school, but doesn't actually use much extra memory, means each connection
is completely isolated from all the others and scales pretty well in a multi-core environment.

You can then control how many times this program forks by how many threads you run in PDNS Recursor.


## Improvement

One obvious improvement would be to buffer (batch) the writes on output. This would improve performance under heavy load.
That said, under heavy load you should have lots of PDNS Recursor threads, so lots of instances of this will `fork()` and your
O/S should take care of things, but reducing the number of `write()` calls would def reduce load.


# Extra Work It Does
The data coming from PowerDNS Recursior sends all the IP Addresses in wire format, so this program decodes them to presentation format.
Also, for `<reasons>`, in the `dnsmessage.proto` file, the `serverIdentity` is defined as binary, not string,
this causes `protobuf2json.c` to base64 encode it, so this is also decoded from base64 back to a string.

I prefer to do this than change Oleg Efimov's code.


# Usage
```
Usage:
-i <socket>       - Listen here for PDNS Recursor to connect, supports IPv4, IPv6 or Unix socket (named or anonymous/unnamed)
-o <socket>       - Connect to vector here, supports IPv4, IPv6 or Unix socket (named or anonymous/unnamed)
-l <log-level>    - see 'log_message.h' for values, preceed with 'x' to specify a hex value, 'x200001' = Normal log level to stderr
-p <path>         - Path name to save Prometheus style metrics to, default = `/var/run/recursor-protobuf.prom`, '-' for None
-t <secs>         - Period in seconds to write Prometheus stats, default = 300
-s <service>      - Add the tag 'service=<service>' in the Prometheus metrics file
-x <max>          - The maximum number of child processes to record metrics for, default=200
-D                - Debug mode, prevent forking
```

## Input & Output
the `input` and `output` sockets are specified in the same format as each other and support
- IPv4, with optional `:port` - default port `7011`
- IPv6, with optional `.port` - default port `9000`
- Unix named socket, this must be the full path name including a leading `/`
- Unix anonymous socket, identified with a leading `@`

Default input is `127.0.0.1:7011`, default output is `127.0.0.1:9000`. These are also the
sockets specified in the example config files [configs/recursor.lua](configs/recursor.lua) and [configs/vector.yaml](configs/vector.yaml).

## Logging
The default log level includes DEBUG, so I recommend you specify something different for production, e.g. `x200001` or `x200003`

See `log_message.h` for all the different log level values.

## Prometheus Stats
This utility supports periodically writing a [Prometheus](https://prometheus.io/) style file of metrics to
a location of your choice at an interval of your choice.

You can then export this file to Prometheus using [Prometheus node_exporter](https://github.com/prometheus/node_exporter)
or a simple HTTPD server like `busybox httpd`.

You can have a tag `service` added to each Prometheus metric by adding the option `-s <name>` for the service name, or you can directly 
specify a comma separated list of tags. NOTE: values must be in double quotes, e.g. `-s 'service="recursor",client="some name"'

if you use `-s <name>` but try and include `=` in the service name you will make a mess of the metrics file!


### Metrics
You will get the following metrics. All metrics are the total from all child process, i.e. all PDNS Recursor threads that make a connection.
- `recursor_protobuf_packets_in`
- `recursor_protobuf_packets_out`
- `recursor_protobuf_bytes_in`
- `recursor_protobuf_bytes_out`
- `recursor_protobuf_lost_bytes`
- `recursor_protobuf_lost_packets`

All metrics are `counter` type.

Lost packets are packets received from PDNS Recursor that we were unable to write to the output receiver
e.g. due to it being down / connection failure or the buffer was full (congestion).
Lost bytes are measured in bytes of JSON, that fail to get written.

Lost packets & bytes does **NOT** include frames correctly read from the input that did not correctly
decode as a `dnsmessage` protobuf.


## This is where the `proto` file came from

Protobuf to parse
- https://github.com/PowerDNS/pdns/blob/master/pdns/dnsmessage.proto

Downloads `dnsmessage.proto` file itself from here, using `curl`
- https://raw.githubusercontent.com/PowerDNS/pdns/refs/heads/master/pdns/dnsmessage.proto

I've included PowerDNS's `dnsmessage.proto` file in this repo as the code would not compile without it
but if you remove it, the `Makefile` should just re-curl it from that second URL.


## Required Packages To Compile

If you want to compile this, you will need to install these packages (names may vary by Linux Dist)
- `protobuf-c`
- `protobuf-c-compiler`
- `protobuf-c-dev`
- `jansson`
- `jansson-dev`

I compiled with `gcc version 12.2.1 20220924` on Alpine Linux v3.18, best of luck with anything else - shouldn't be too hard!

### To Build

To build the binary, just run `make`. Also supported
- `make clean` - remove all files created by the `Makefile`, except `dnsmessage.proto`
- `make all` - same as `make`
- `make strip` - make the binary then strip it.

I use quite a few extra compiler checks and get no compiler errors or warnings, so I expect you to get the same.


## Example Configs in `configs`
These config files are all you need to test this code out. For production use you'll want all sorts of other parameters.

### PDNS Recursor
- [configs/recursor.conf](configs/recursor.conf)
- [configs/recursor.lua](configs/recursor.lua)

with this config the default input of `127.0.0.1:7011` should work.

### [Vector](https://vector.dev)
- [configs/vector.yaml](configs/vector.yaml)

with this config the default output of `127.0.0.1:9000` should work.

This config also enables Vector's control API socket, so you can use the shell command `vector top` to monitor the packet flow.
