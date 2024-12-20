package main

import (
	"encoding/binary"
	"net"
	"strconv"

	"github.com/rs/zerolog/log"
)

var (
	SUC    = []byte{0x05, 0x00}
	SusRep = []byte{0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	errRep = []byte{0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
)

func serveSocks(stream *Stream) {
	header := make([]byte, 2)
	_ = readBe(stream, &header)

	nmeth := header[1]

	methods := make([]byte, int(nmeth))
	_ = readBe(stream, &methods)

	_ = writeBe(stream, SUC)

	// read request

	request := make([]byte, 4)
	_ = readBe(stream, &request)

	adtp := request[3]

	var host string

	switch adtp {
	case 1:
		hostBuf := make([]byte, net.IPv4len)
		_ = readBe(stream, &hostBuf)
		host = net.IP(hostBuf).String()
	case 3:
		hostBuf := make([]byte, net.IPv6len)
		_ = readBe(stream, &hostBuf)
		host = net.IP(hostBuf).String()
	default:
		var length uint8
		_ = readBe(stream, &length)
		buf := make([]byte, length)
		_ = readBe(stream, &buf)
		host = string(buf)
	}

	portBuf := make([]byte, 2)
	_ = readBe(stream, &portBuf)
	port := binary.BigEndian.Uint16(portBuf)

	targetAddr := net.JoinHostPort(host, strconv.FormatInt(int64(port), 10))

	remote, err := net.Dial("tcp", targetAddr)
	if err != nil {
		log.Error().Err(err).Str("remote", targetAddr).Msg("connect error")
		_, _ = stream.Write(errRep)
		return
	}
	_, _ = stream.Write(SusRep)
	relay("socks", stream, remote)
}
