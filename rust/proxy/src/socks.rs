use std::net::{Ipv4Addr, Ipv6Addr, SocketAddr, SocketAddrV4, SocketAddrV6};
use std::slice;
use tokio::io::{AsyncRead, AsyncReadExt, AsyncWriteExt};
use tokio::time::Instant;
use tracing::{debug, info};

#[derive(Clone, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub enum Addr {
    /// Socket address (IP Address)
    SocketAddress(SocketAddr),
    /// Domain name address
    DomainNameAddress(String, u16),
}

impl Addr {
    async fn from_reader<R>(adtype: u8, reader: &mut R) -> anyhow::Result<Addr>
    where
        R: AsyncRead + Unpin,
    {
        if adtype == 0x01 {
            let mut buf = [0u8; 6];
            let _ = reader.read_exact(&mut buf).await?;

            let v4addr = Ipv4Addr::new(buf[0], buf[1], buf[2], buf[3]);
            let port = u16::from_be_bytes([buf[4], buf[5]]);
            return Ok(Addr::SocketAddress(SocketAddr::V4(SocketAddrV4::new(
                v4addr, port,
            ))));
        }

        if adtype == 0x04 {
            let mut buf = [0u16; 9];

            let bytes_buf = unsafe { slice::from_raw_parts_mut(buf.as_mut_ptr() as *mut _, 18) };
            let _ = reader.read_exact(bytes_buf).await?;

            let v6addr = Ipv6Addr::new(
                u16::from_be(buf[0]),
                u16::from_be(buf[1]),
                u16::from_be(buf[2]),
                u16::from_be(buf[3]),
                u16::from_be(buf[4]),
                u16::from_be(buf[5]),
                u16::from_be(buf[6]),
                u16::from_be(buf[7]),
            );
            let port = u16::from_be(buf[8]);

            return Ok(Addr::SocketAddress(SocketAddr::V6(SocketAddrV6::new(
                v6addr, port, 0, 0,
            ))));
        }
        let mut length_buf = [0u8; 1];
        let _ = reader.read_exact(&mut length_buf).await?;
        let length = length_buf[0] as usize;

        // Len(Domain) + Len(Port)
        let buf_length = length + 2;

        let mut raw_addr = vec![0u8; buf_length];
        let _ = reader.read_exact(&mut raw_addr).await?;

        let raw_port = &raw_addr[length..];
        let port = u16::from_be_bytes([raw_port[0], raw_port[1]]);

        raw_addr.truncate(length);

        let addr = String::from_utf8(raw_addr).unwrap();

        Ok(Addr::DomainNameAddress(addr, port))
    }
}

// https://datatracker.ietf.org/doc/html/rfc1928
// https://datatracker.ietf.org/doc/html/rfc1929
pub async fn handle(mut incoming: tokio::net::TcpStream) -> anyhow::Result<()> {
    let start = Instant::now();

    // step1 handshake

    //    +----+----------+----------+
    //    |VER | NMETHODS | METHODS  |
    //    +----+----------+----------+
    //    | 1  |    1     | 1 to 255 |
    //    +----+----------+----------+

    {
        let mut buf = vec![0; 2];
        let _ = incoming.read_exact(&mut buf).await?;
        let ver = buf[0];
        let nmet = buf[1];
        if ver != 0x05 {
            return Err(anyhow::anyhow!("unsupport version"));
        }

        let mut methods = vec![0u8; nmet as usize];
        let _ = incoming.read_exact(&mut methods).await?;

        for method in methods.iter() {
            debug!("method is {:?}", *method);

            // if *method == 0x00 {
            //     let mut buffer = vec![0x05, 0x00];
            //     incoming.write(&mut buffer).await?;
            // }

            if *method == 0x02 {
                let buffer = vec![0x05, 0x02];
                let _ = incoming.write(&buffer).await?;
                let ver = incoming.read_u8().await?;

                debug!("ver is {}", ver);

                // assert ver = 1

                let ulen = incoming.read_u8().await?;
                let mut namebuf = vec![0u8; ulen as usize];
                let _ = incoming.read_exact(&mut namebuf).await?;

                let plen = incoming.read_u8().await?;
                let mut pbuf = vec![0u8; plen as usize];
                let _ = incoming.read_exact(&mut pbuf).await?;

                // todo check

                info!(
                    "user is {} passwd is {}",
                    String::from_utf8(namebuf).unwrap(),
                    String::from_utf8(pbuf).unwrap()
                );
                let authreply = vec![0x01, 0x00];
                let _ = incoming.write(&authreply).await?;
            }
        }
    }

    // step2 read request

    // +----+-----+-------+------+----------+----------+
    // |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
    // +----+-----+-------+------+----------+----------+
    // | 1  |  1  | X'00' |  1   | Variable |    2     |
    // +----+-----+-------+------+----------+----------+
    //  o  IP V4 address: X'01'
    //  o  DOMAINNAME: X'03'
    //  o  IP V6 address: X'04'
    let mut buf: Vec<u8> = vec![0; 4];
    incoming.read_exact(&mut buf).await?;

    // todo check ver and cmd
    let adtype = buf[3];
    let addr = Addr::from_reader(adtype, &mut incoming).await?;

    let mut remote = match addr {
        Addr::SocketAddress(sock) => tokio::net::TcpStream::connect(sock).await?,
        Addr::DomainNameAddress(domain, port) => {
            tokio::net::TcpStream::connect((domain, port)).await?
        }
    };

    let reply = vec![0x05, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    let _ = incoming.write(&reply).await?;

    let from = incoming.peer_addr()?;
    // let tunnel = incoming.local_addr()?;
    let target = remote.peer_addr()?;

    let (rl, wl) = tokio::io::copy_bidirectional(&mut incoming, &mut remote).await?;

    info!(
        "TCP tunnel {} <-> {} write {} read {} cost {:?}",
        from,
        target,
        rl,
        wl,
        start.elapsed()
    );
    Ok(())
}
