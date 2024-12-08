use tokio::io::BufReader;
use tokio::io::{AsyncBufReadExt, AsyncWriteExt};
use tokio::time::Instant;
use tracing::{debug, info};

const CONNECT: &str = "CONNECT";
const LF: &str = "\r\n";
const SUC: &str = "HTTP/1.1 200 Connection Established\r\n\r\n";

pub async fn handle(incoming: tokio::net::TcpStream) -> anyhow::Result<()> {
    let start = Instant::now();
    let from = incoming.peer_addr()?;

    let mut stream = BufReader::new(incoming);
    let mut lines = Vec::with_capacity(4);
    loop {
        let mut buf = String::new();
        stream.read_line(&mut buf).await?;
        if buf.eq(LF) || buf.is_empty() {
            break;
        }
        lines.push(buf);
    }

    let reqlines = lines.join("");
    debug!("reqlines {:?}", &reqlines);
    let mut headers = [httparse::EMPTY_HEADER; 4];
    let mut req = httparse::Request::new(&mut headers);
    req.parse(reqlines.as_bytes())?;

    let mut host = String::new();
    for ele in req.headers.iter() {
        if ele.name == "Host" {
            host = String::from_utf8(ele.value.to_vec()).unwrap();
            break;
        }
    }

    let mut endpoint = host;

    if !endpoint.contains(":") {
        endpoint = format!("{}:80", endpoint);
    }

    let mut remote = tokio::net::TcpStream::connect(&endpoint).await?;

    info!("connect to {} succsss. ", &endpoint);

    if req.method.unwrap() == CONNECT {
        let _ = stream.write(SUC.as_bytes()).await?;
    } else {
        let mut reqlines = String::new();
        reqlines.reserve(256);
        for line in lines.iter() {
            if line.starts_with("Proxy-Connection") {
                continue;
            }
            reqlines.push_str(line);
        }
        reqlines.push_str(LF);
        let _ = remote.write(reqlines.as_bytes()).await?;
    }

    let (rl, wl) = tokio::io::copy_bidirectional(&mut stream, &mut remote).await?;
    info!(
        "http tunnel {} <-> {} write {} read {} cost {:?}",
        from,
        &endpoint,
        rl,
        wl,
        start.elapsed()
    );
    Ok(())
}
