package main

import (
	"bufio"
	"encoding/binary"
	"errors"
	"io"
	"net"

	"github.com/rs/zerolog/log"
)

type Stream struct {
	r *bufio.Reader
	net.Conn
}

func newStream(c net.Conn) *Stream {
	return &Stream{bufio.NewReader(c), c}
}

func (b *Stream) Peek(n int) ([]byte, error) {
	return b.r.Peek(n)
}

func (b *Stream) Read(p []byte) (int, error) {
	return b.r.Read(p)
}

func (b *Stream) Line() (string, error) {
	line, _, err := b.r.ReadLine()
	return string(line), err
}

func readBe[T any](r io.Reader, data T) (err error) {
	if err = binary.Read(r, binary.BigEndian, data); err != nil {
		log.Error().Err(err).Msg("readBe error")
	}
	return
}

func writeBe[T any](r io.Writer, data T) (err error) {
	if err = binary.Write(r, binary.BigEndian, data); err != nil {
		log.Error().Err(err).Msg("writeBe error")
	}
	return
}

type Resp struct {
	len  int64
	err  error
	from net.Addr
	to   net.Addr
}

func relay(tag string, from, to net.Conn) {
	defer func(from, to net.Conn) {
		_ = from.Close()
		_ = to.Close()
	}(from, to)

	channel := make(chan Resp, 2)

	go func(dst, src net.Conn) {
		size, err := io.Copy(dst, src)
		channel <- Resp{len: size, err: err, from: src.RemoteAddr(), to: dst.RemoteAddr()}
	}(from, to)

	go func(dst, src net.Conn) {
		size, err := io.Copy(dst, src)
		channel <- Resp{len: size, err: err, from: src.RemoteAddr(), to: dst.RemoteAddr()}
	}(to, from)

	for resp := range channel {
		fromAddr := resp.from.String()
		toAddr := resp.to.String()

		if resp.err != nil && errors.Is(resp.err, io.EOF) {
			log.Error().Err(resp.err).Str("tag", tag).Str("from", fromAddr).Str("to", toAddr).Msg("relay error")
		} else {
			log.Info().Str("tag", tag).Str("from", fromAddr).
				Str("to", toAddr).Int64("transfer", resp.len).Msg("relay success")
		}
	}
}
