build:
	cargo build --release --timings -Zno-embed-metadata -Zprofile-hint-mostly-unused
# 	cargo build --release --timings -Zprofile-hint-mostly-unused
# 	cargo build --release --timings

debug:
	cargo +nightly build --no-default-features -Zno-embed-metadata -Zprofile-hint-mostly-unused

clean:
	cargo clean

lint:
	cargo clippy --all-targets --all-features -- -D warnings
