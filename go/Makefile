LDFLAGS := -s -w
TARGET := target/server
TAGS=urfave_cli_no_docs,netgo
BUILD=go build -tags $(TAGS) -trimpath

vet:
	go vet ./...

fmt:
	gofumpt -l -w .

tidy:
	go mod tidy

lint:
	golangci-lint run -v

build: tidy fmt vet
	$(BUILD) -ldflags "$(LDFLAGS)" -o "$(TARGET)"
