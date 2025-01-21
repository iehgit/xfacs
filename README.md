# xfacs

X11 clipboard utility for single-paste file content serving

## Usage

```bash
xfacs <filename>
```

## Build

```bash
make
```

## Dependencies

- libX11

## Description

Reads a file's content into X11 clipboard and serves it for a single paste operation. Forks into background and automatically terminates after successful paste or 600-second timeout.
If the input file ends with a newline character, it is automatically trimmed. This behaviour is useful for pasting file content into command lines.
