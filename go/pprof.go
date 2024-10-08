//go:build pprof

package main

import (
	"net/http"
	_ "net/http/pprof"
)

func init() {
	go func() {
		_ = http.ListenAndServe("127.0.0.1:2323", nil)
	}()
}
