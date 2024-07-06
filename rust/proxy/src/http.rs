use tokio::io::BufReader;
use tokio::io::{AsyncBufReadExt, AsyncWriteExt};
use tokio::time::Instant;
use tracing::info;

pub async fn handle(incoming: tokio::net::TcpStream) -> anyhow::Result<()> {
    let start = Instant::now();
    let from = incoming.peer_addr()?;

    let mut stream = BufReader::new(incoming);
    let mut lines = Vec::with_capacity(4);
    loop {
        let mut buf = String::new();
        stream.read_line(&mut buf).await?;
        if buf.eq("\r\n") {
            break;
        }
        lines.push(buf);
    }

    let reqlines = lines.join("");
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

    if req.method.unwrap() == "CONNECT" {
        let resp = format!("HTTP/1.1 200 Connection Established\r\n\r\n");
        stream.write(resp.as_bytes()).await?;
    } else {
        let tmpline = format!("{} {} HTTP/1.1\r\n", req.method.unwrap(), req.path.unwrap());
        remote.write(tmpline.as_bytes()).await?;

        for ele in req.headers.iter() {
            if !ele.name.eq("Proxy-Connection") {
                let name = ele.name.to_owned();
                let val = String::from_utf8(ele.value.to_vec()).unwrap();
                let hl = format!("{}: {}\r\n", name, val);
                remote.write(hl.as_bytes()).await?;
            }
        }
        remote.write("\r\n".as_bytes()).await?;
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
