package main

import (
	"context"
	"fmt"
	"net"
	"os"
	"strings"
	"time"

	"github.com/rs/zerolog"
	"github.com/rs/zerolog/log"
	"github.com/urfave/cli/v3"
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
	lc.Control = Control(option)
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
		log.Trace().
			Uint8("type", peek[0]).
			Str("from", conn.LocalAddr().String()).
			Msg("incomeing request")
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
		Name:  "proxy",
		Usage: "socks5 && http proxy util",
		Flags: []cli.Flag{
			&cli.StringFlag{
				Name:     "addr",
				Required: true,
				Aliases:  []string{"a"},
				Usage:    "listen address",
			},

			&cli.BoolFlag{
				Name:    "fastopen",
				Value:   true,
				Aliases: []string{"f"},
				Usage:   "enable fast open",
			},

			&cli.BoolFlag{
				Name:    "nodelay",
				Value:   true,
				Aliases: []string{"n"},
				Usage:   "enable tcp nodely",
			},

			&cli.BoolFlag{
				Name:    "reuseaddr",
				Value:   true,
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
		log.Fatal().Err(err).Send()
	}
}
