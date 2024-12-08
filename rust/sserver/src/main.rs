use clap::Parser;

#[derive(Parser)] // requires `derive` feature
struct Opt {
    /// server listen address
    #[arg(short, long, default_value = "0.0.0.0:10010")]
    addr: String,

    /// enable fast open
    #[arg(short, long, default_value_t = true)]
    fastopen: bool,

    /// enable tcp nodelay
    #[arg(short, long, default_value_t = true)]
    nodelay: bool,

    /// enable reuse address
    #[arg(short, long, default_value_t = true)]
    reuseaddr: bool,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::fmt()
        .with_env_filter(tracing_subscriber::EnvFilter::from_default_env())
        .with_thread_ids(true)
        .init();

    let opt = Opt::parse();
    proxy::listen(&opt.addr).await
}
