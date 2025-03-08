import logging
import typer
import socket
import relay

def run(host: str = "127.0.0.1", port: int = 10086, backlog: int = 1024) -> None:
    server_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(backlog)
    logging.info(f"server listen at auto://{host}:{port}")
    try:
        while True:
            recoming, raddr = server_socket.accept()
            logging.info(f"incoming request. {raddr}")
            dispatch: relay.Dispatch = relay.Dispatch(recoming, raddr)
            dispatch.handle()
    except Exception as ex:
        logging.error(f"server run error. {ex}")
    finally:
        server_socket.close()
        logging.info("server stopped.")

if __name__ == "__main__":

    logging.basicConfig(
        format="%(asctime)s - %(thread)d - %(levelname)s - %(message)s",
        level=logging.DEBUG,
    )

    typer.run(run)
