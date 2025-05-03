# Ignore this file, its not relevant

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
