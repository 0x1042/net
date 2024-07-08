package main

import (
	"bytes"
	"net"
	"strings"

	"github.com/rs/zerolog/log"
)

var SUCCESS = []byte("HTTP/1.1 200 Connection Established\r\n\r\n")

const (
	CONNECT  = "CONNECT"
	LF       = "\r\n"
	PORT     = "80"
	HEADLEN  = 5
	HEADSIZE = 512
)

type Req struct {
	Method string
	Host   string
	Port   string
}

func newReq(line []string) *Req {
	req := new(Req)
	req.Port = PORT
	// first
	{
		// CONNECT www.google.com:443 HTTP/1.1\r\n
		// GET http://www.google.com/ HTTP/1.1\r\n
		tmps := strings.Split(line[0], " ")
		req.Method = tmps[0]
	}

	// second
	{

		// Host: www.google.com:443
		// Host: www.google.com
		tmps := strings.Split(line[1], ":")
		req.Host = strings.TrimSpace(tmps[1])
		if len(tmps) == 3 {
			req.Port = strings.TrimSpace(tmps[2])
		}
	}
	return req
}

func serveHTTP(stream *Stream) {
	headers := make([]string, 0, HEADLEN)
	for {
		line, err := stream.Line()
		if err != nil {
			log.Error().Err(err).Msg("read line error")
			return
		}

		if line == LF || len(line) == 0 {
			break
		}
		headers = append(headers, line)
	}

	req := newReq(headers)
	remoteAddr := net.JoinHostPort(req.Host, req.Port)

	remote, err := net.Dial("tcp", remoteAddr)
	if err != nil {
		log.Error().Err(err).Str("to", remoteAddr).Msg("connect error")
		return
	}

	if req.Method == CONNECT {
		_, _ = stream.Write(SUCCESS)
	} else {
		buf := bytes.Buffer{}
		buf.Grow(HEADSIZE)
		for _, line := range headers {
			if strings.HasPrefix(line, "Proxy") {
				continue
			}
			buf.WriteString(line)
			buf.WriteString(LF)
		}
		buf.WriteString(LF)
		_, _ = remote.Write(buf.Bytes())
	}
	relay(stream, remote)
}
