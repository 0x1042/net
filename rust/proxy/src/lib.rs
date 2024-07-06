use tracing::{error, info};

pub mod http;
pub mod socks;

pub async fn new_instance(port: u16) {
    let endpoint = format!("0.0.0.0:{}", port);

    tokio::spawn(async move {
        if let Err(err) = instance(&endpoint).await {
            error!("start instance error. {err}");
        }
    });
}

pub async fn instance(addr: &str) -> anyhow::Result<()> {
    let ln = tokio::net::TcpListener::bind(addr).await?;

    info!("server start. auto://{}", addr);

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

pub async fn simple(port: u16) {
    let addr = format!("0.0.0.0:{}", port);
    let ln = tokio::net::TcpListener::bind(&addr).await.unwrap();

    info!("server start. auto://{}", addr);

    loop {
        let (incoming, peer) = ln.accept().await.unwrap();
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

    if ver[0] == 0x05 {
        return socks::handle(incoming).await;
    }
    return http::handle(incoming).await;
}
