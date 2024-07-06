- [cpp](#cpp)
  - [build](#build)
  - [run](#run)
- [rust](#rust)
  - [build](#build-1)
  - [run](#run-1)
- [go](#go)
  - [build](#build-2)
  - [run](#run-2)

# cpp

## build 

```
cd cpp
make build
```

## run

```
./target/server --help
Usage: ./target/server <option> [value]
Options:
  -p, --port          listen port
  -f, --fastopen      enable fastopen
  -r, --reuseaddr     enable reuse address
  -n, --nodelay       enable tcp nodelay
  -w, --worker        worker number
  -h, --help          print help


./target/server --port 10085 --fastopen --reuseaddr --nodelay --worker 2
```

# rust 

## build 

```
cd rust
make build
```

## run

```

```

# go 

## build 

```
cd go
make build
```

## run

```
./bin/server --help

NAME:
   proxy - proxy

USAGE:
   proxy [global options] [command [command options]] [arguments...]

COMMANDS:
   help, h  Shows a list of commands or help for one command

GLOBAL OPTIONS:
   --addr value, -a value  listen address (default: ":10085")
   --fastopen, -f          enable fast open (default: false)
   --nodelay, -n           enable tcp nodely (default: false)
   --reuseaddr, -r         enable reuse addr (default: false)
   --help, -h              show help (default: false)

./bin/server --addr :10010 --fastopen --nodelay --reuseaddr
```