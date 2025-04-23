# donut-cli

## Compile

```bash
gcc donut.c -s -O3 -o donut -lm
```

## Usage
```bash
Usage: ./donut [color] [speed]
Press 'q' or ESC to quit.

Arguments:
  color          Color name (optional, default: green).
                 Available: green, red, blue, cyan, magenta, yellow, white
  speed          Positive speed factor (optional, default: 1.0).
                 > 1.0: faster, < 1.0: slower.
```

