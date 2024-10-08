//go:build !linux && !darwin

package main

import (
	"runtime"
	"syscall"

	"github.com/rs/zerolog/log"
)

func Control(option Option) func(network, address string, c syscall.RawConn) error {
	log.Warn().Str("os", runtime.GOOS).Msg("os does not support.")
	return nil
}
