package main

import (
	"net"
	"strings"

	"github.com/rs/zerolog/log"
)

type Req struct {
	Method string
	Host   string
	Port   string
}

func newReq(line []string) *Req {
	req := new(Req)
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
		} else {
			req.Port = "80"
		}
	}
	return req
}

func serveHTTP(stream *Stream) {
	headers := make([]string, 0, 5)
	for {
		line, err := stream.Line()
		if err != nil {
			log.Error().Err(err).Msg("read line error")
			return
		}

		if line == "\r\n" || len(line) == 0 {
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

	if req.Method == "CONNECT" {
		_, _ = stream.Write([]byte("HTTP/1.1 200 Connection Established\r\n\r\n"))
	} else {
		var buf string
		for _, line := range headers {
			if strings.HasPrefix(line, "Proxy-") {
				continue
			}
			buf = buf + line + "\r\n"
		}
		buf += "\r\n"
		_, _ = remote.Write([]byte(buf))
	}
	relay(stream, remote)
}
