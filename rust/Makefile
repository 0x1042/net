build:
	cargo build --release

debug:
	cargo +nightly build --no-default-features -Zno-embed-metadata

clean:
	cargo clean

lint:
	cargo clippy --all-targets --all-features -- -D warnings