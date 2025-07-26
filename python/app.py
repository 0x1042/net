import proxy
from absl import logging
import asyncio


async def main():
    handler = proxy.Socks5Proxy()
    server = await asyncio.start_server(handler.handle_client, "127.0.0.1", 10086)

    async with server:
        logging.info("SOCKS5 proxy server started on 127.0.0.1:10086")
        await server.serve_forever()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logging.info("Proxy server shutting down.")
