use tracing::{error, info};

pub mod http;
pub mod socks;

const VER: u8 = 0x05;

pub async fn listen(addr: &str) -> anyhow::Result<()> {
    let mut endpoint = addr.to_owned();
    let info: Vec<_> = addr.split(":").collect();
    if info[0].is_empty() {
        endpoint = format!("0.0.0.0:{}", info[1]);
    }
    let ln = tokio::net::TcpListener::bind(&endpoint).await?;
    info!("server start. auto://{}", &endpoint);
    loop {
        let (incoming, peer) = ln.accept().await?;
        info!("incoming request {:?}", peer);
        tokio::spawn(async move {
            if let Err(err) = proxy(incoming).await {
                error!("proxy error. {err}");
            }
        });
    }
}

async fn proxy(incoming: tokio::net::TcpStream) -> anyhow::Result<()> {
    incoming.set_nodelay(true)?;
    let mut ver = vec![0; 1];
    let _ = incoming.peek(&mut ver).await?;

    if ver[0] == VER {
        return socks::handle(incoming).await;
    }
    http::handle(incoming).await
}
