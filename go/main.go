package main

import (
	"context"
	"fmt"
	"net"
	"os"
	"strings"
	"syscall"
	"time"

	"github.com/rs/zerolog"
	"github.com/rs/zerolog/log"
	"github.com/urfave/cli/v3"
	"golang.org/x/sys/unix"
)

type Option struct {
	addr      string
	fastOpen  bool
	reuseAddr bool
	nodelay   bool
}

func listen(option Option) error {
	var lc net.ListenConfig
	lc.SetMultipathTCP(true)
	lc.Control = func(_, _ string, c syscall.RawConn) error {
		var err error
		_ = c.Control(func(fd uintptr) {
			if option.nodelay {
				err = unix.SetsockoptInt(int(fd), unix.IPPROTO_TCP, unix.TCP_NODELAY, 1)
			}
			if option.fastOpen {
				err = unix.SetsockoptInt(int(fd), unix.IPPROTO_TCP, unix.TCP_FASTOPEN, 1)
			}
			if option.reuseAddr {
				err = unix.SetsockoptInt(int(fd), unix.SOL_SOCKET, unix.SO_REUSEADDR, 1)
				err = unix.SetsockoptInt(int(fd), unix.SOL_SOCKET, unix.SO_REUSEPORT, 1)
			}
		})
		return err
	}
	ln, err := lc.Listen(context.Background(), "tcp", option.addr)
	if err != nil {
		return err
	}

	log.Info().Str("addr", option.addr).Msg("server start")

	for {
		conn, err := ln.Accept()
		if err != nil {
			log.Error().Err(err).Msg("accept error")
			break
		}
		stream := newStream(conn)

		peek, _ := stream.Peek(1)
		log.Trace().Uint8("type", peek[0]).Str("from", conn.LocalAddr().String()).Msg("incomeing request")
		if peek[0] == 0x05 {
			go serveSocks(stream)
		} else {
			go serveHTTP(stream)
		}
	}
	return nil
}

func main() {
	log.Logger = log.Output(zerolog.ConsoleWriter{
		Out:        os.Stderr,
		NoColor:    false,
		TimeFormat: time.TimeOnly,
		FormatLevel: func(level any) string {
			return strings.ToUpper(fmt.Sprintf("|%-6s|", level))
		},
	})

	cmd := &cli.Command{
		Name: "proxy",

		Flags: []cli.Flag{
			&cli.StringFlag{
				Name:    "addr",
				Value:   ":10085",
				Aliases: []string{"a"},
				Usage:   "listen address",
			},

			&cli.BoolFlag{
				Name:    "fastopen",
				Value:   false,
				Aliases: []string{"f"},
				Usage:   "enable fast open",
			},

			&cli.BoolFlag{
				Name:    "nodelay",
				Value:   false,
				Aliases: []string{"n"},
				Usage:   "enable tcp nodely",
			},

			&cli.BoolFlag{
				Name:    "reuseaddr",
				Value:   false,
				Aliases: []string{"r"},
				Usage:   "enable reuse addr",
			},
		},

		Action: func(_ context.Context, cmd *cli.Command) error {
			var option Option
			option.addr = cmd.String("addr")
			option.fastOpen = cmd.Bool("fastopen")
			option.reuseAddr = cmd.Bool("reuseaddr")
			option.nodelay = cmd.Bool("nodelay")
			return listen(option)
		},
	}
	if err := cmd.Run(context.TODO(), os.Args); err != nil {
		log.Fatal().Err(err)
	}
}
