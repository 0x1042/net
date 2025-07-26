import asyncio
import struct

from absl import logging

# SOCKS5 协议常量
SOCKS_VERSION = 5
METHOD_NO_AUTH = 0x00
CMD_CONNECT = 0x01
ADDR_TYPE_IPV4 = 0x01
ADDR_TYPE_DOMAIN = 0x03
ADDR_TYPE_IPV6 = 0x04


class Socks5Proxy:
    async def handle_client(self, client_reader, client_writer):
        """处理单个客户端连接的完整流程"""
        client_addr = client_writer.get_extra_info("peername")
        logging.info(f"New connection from {client_addr}")

        try:
            # 1. 握手阶段
            await self.perform_handshake(client_reader, client_writer)

            # 2. 请求阶段
            target_host, target_port = await self.parse_request(
                client_reader, client_writer
            )
            if not target_host:
                return  # 解析失败，已在内部关闭连接

            # 3. 连接目标服务器并建立中继
            await self.establish_relay(
                client_reader, client_writer, target_host, target_port
            )

        except asyncio.CancelledError:
            logging.info(f"Connection {client_addr} cancelled.")
        except Exception as e:
            logging.error(f"Error handling client {client_addr}: {e}")
        finally:
            if not client_writer.is_closing():
                client_writer.close()
                await client_writer.wait_closed()
            logging.info(f"Connection from {client_addr} closed.")

    async def perform_handshake(self, reader, writer):
        """处理 SOCKS5 握手"""
        # 读取客户端的握手信息： VER | NMETHODS | METHODS
        header = await reader.read(2)
        version, nmethods = struct.unpack("!BB", header)

        if version != SOCKS_VERSION:
            raise ValueError("Invalid SOCKS version")

        # 读取支持的方法列表
        methods = await reader.read(nmethods)
        if METHOD_NO_AUTH not in methods:
            raise ValueError("Unsupported authentication method")

        # 回复客户端：选择“无需认证”方法
        writer.write(struct.pack("!BB", SOCKS_VERSION, METHOD_NO_AUTH))
        await writer.drain()
        logging.info("Handshake successful (No Authentication)")

    async def parse_request(self, reader, writer):
        """解析客户端的连接请求，返回 (host, port)"""
        # 读取请求头：VER | CMD | RSV | ATYP
        header = await reader.read(4)
        version, cmd, _, addr_type = struct.unpack("!BBBB", header)

        if version != SOCKS_VERSION or cmd != CMD_CONNECT:
            # 不支持的命令，发送错误回复并关闭连接
            # 构建一个通用的失败响应
            reply = (
                struct.pack("!BBBB", SOCKS_VERSION, 0x07, 0x00, ADDR_TYPE_IPV4)
                + b"\x00\x00\x00\x00\x00\x00"
            )
            writer.write(reply)
            await writer.drain()
            raise ValueError(f"Unsupported command: {cmd}")

        # 根据地址类型读取目标地址
        if addr_type == ADDR_TYPE_IPV4:
            ip_bytes = await reader.read(4)
            host = ".".join(map(str, ip_bytes))
        elif addr_type == ADDR_TYPE_DOMAIN:
            domain_len_byte = await reader.read(1)
            domain_len = domain_len_byte[0]
            domain_bytes = await reader.read(domain_len)
            host = domain_bytes.decode("utf-8")
        else:  # 不支持 IPv6
            raise ValueError(f"Unsupported address type: {addr_type}")

        # 读取端口
        port_bytes = await reader.read(2)
        port = struct.unpack("!H", port_bytes)[0]

        logging.info(f"Client requests to connect to {host}:{port}")
        return host, port

    async def establish_relay(
        self, client_reader, client_writer, target_host, target_port
    ):
        """连接目标服务器，并建立双向数据转发"""
        try:
            # 连接目标服务器
            target_reader, target_writer = await asyncio.open_connection(
                target_host, target_port
            )
            logging.info(f"Successfully connected to {target_host}:{target_port}")

            # 向客户端发送成功响应
            # 获取本地绑定的地址和端口信息 (通常是0.0.0.0)
            bind_addr = target_writer.get_extra_info("sockname")
            # 回复格式: VER | REP | RSV | ATYP | BND.ADDR | BND.PORT
            # REP=0x00 表示成功
            reply = (
                struct.pack("!BBBB", SOCKS_VERSION, 0x00, 0x00, ADDR_TYPE_IPV4)
                + b"\x00\x00\x00\x00"
                + struct.pack("!H", bind_addr[1])
            )
            client_writer.write(reply)
            await client_writer.drain()

            # 4. 中继阶段
            # 创建两个任务，一个负责从客户端读、往目标写；另一个反之。
            task1 = asyncio.create_task(
                self.relay_data(client_reader, target_writer, "Client -> Target")
            )
            task2 = asyncio.create_task(
                self.relay_data(target_reader, client_writer, "Target -> Client")
            )

            # 等待任意一个任务结束（通常是因为一方关闭了连接）
            await asyncio.gather(task1, task2, return_exceptions=True)

        except Exception as e:
            logging.error(f"Failed to connect to {target_host}:{target_port}: {e}")
            # 向客户端发送连接失败响应
            reply = (
                struct.pack("!BBBB", SOCKS_VERSION, 0x04, 0x00, ADDR_TYPE_IPV4)
                + b"\x00\x00\x00\x00\x00\x00"
            )
            client_writer.write(reply)
            await client_writer.drain()
        finally:
            # 确保目标服务器的连接被关闭
            if "target_writer" in locals() and not target_writer.is_closing():
                target_writer.close()
                await target_writer.wait_closed()

    async def relay_data(self, reader, writer, direction):
        """单向数据转发的协程"""
        try:
            while True:
                data = await reader.read(4096)  # 每次最多读取 4KB
                if not data:
                    # 连接已关闭
                    break
                writer.write(data)
                await writer.drain()  # 等待数据写入缓冲区
        except Exception as e:
            logging.warning(f"Relay {direction} error: {e}")
        finally:
            # 关闭写端，通知对方我们这边已经没有数据要发送了
            if not writer.is_closing():
                writer.close()
                await writer.wait_closed()
