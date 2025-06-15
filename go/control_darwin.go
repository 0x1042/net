//go:build darwin && !js

package main

import (
	"syscall"

	"github.com/rs/zerolog/log"
	"golang.org/x/sys/unix"
)

func Control(option Option) func(network, address string, c syscall.RawConn) error {
	return func(network, address string, c syscall.RawConn) error {
		var err error
		_ = c.Control(func(fd uintptr) {
			if option.nodelay {
				err = unix.SetsockoptInt(int(fd), unix.IPPROTO_TCP, unix.TCP_NODELAY, 1)
				log.Trace().Bool("status", err == nil).Msg("enable tcp no delay")
			}
			if option.reuseAddr {
				err = unix.SetsockoptInt(int(fd), unix.SOL_SOCKET, unix.SO_REUSEADDR, 1)
				err = unix.SetsockoptInt(int(fd), unix.SOL_SOCKET, unix.SO_REUSEPORT, 1)
				log.Trace().Bool("status", err == nil).Msg("enable resuse address")
			}
		})
		if err != nil {
			log.Error().
				Str("network", network).
				Str("address", address).
				Err(err).
				Msg("set control error")
		} else {
			log.Info().Str("network", network).Str("address", address).Msg("set control success")
		}
		return nil
	}
}
