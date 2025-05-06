# Example Configs in `configs`
These config files are all you need to test this code out. For production use you'll want all sorts of other parameters.

## PDNS Recursor
- [configs/recursor.conf](configs/recursor.conf)
- [configs/recursor.lua](configs/recursor.lua)

with this config the default input of `127.0.0.1:7011` should work.

## [Vector](https://vector.dev)
- [configs/vector.yaml](configs/vector.yaml)

with this config the default output of `127.0.0.1:9000` should work.

This config also enables Vector's control API socket, so you can use the shell command `vector top` to monitor the packet flow.
