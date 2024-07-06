#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::fmt()
        .with_env_filter(tracing_subscriber::EnvFilter::from_default_env())
        .with_thread_ids(true)
        .init();

    proxy::instance(format!("{}:{}", "0.0.0.0", 10086).as_str()).await?;

    Ok(())
}
