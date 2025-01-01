import socket
import logging
import struct
import threading
from typing import Tuple

from abc import ABC, abstractmethod


class Proxy(ABC):
    def __init__(self, local: socket.socket, addr: Tuple[str, int], bufsize: int = 1024 * 32):
        self.local = local
        self.addr = addr
        self.bufsize = bufsize

    @abstractmethod
    def handle(self):
        pass

    def copy(self, reader: socket.socket, writer: socket.socket) -> None:
        try:
            while True:
                data = reader.recv(self.bufsize)
                if not data:
                    break
                writer.sendall(data)
        except Exception as err:
            logging.error(f"copy error {err}")

    def copy_bidirectional(self, remote: socket.socket):
        thread1 = threading.Thread(target=self.copy, args=(self.local, remote))
        thread2 = threading.Thread(target=self.copy, args=(remote, self.local))

        thread1.start()
        thread2.start()

        thread1.join()
        thread2.join()


class Socks5(Proxy):
    def __init__(self, local: socket.socket, addr: Tuple[str, int], bufsize: int = 1024 * 32):
        super().__init__(local, addr, bufsize)

    def handle(self):
        buf: bytes = self.local.recv(2)
        logging.debug(f"reading framed: header {buf}")

        self.local.recv(buf[1])

        no_auth: bytes = bytes([0x05, 0x00])
        self.local.sendall(no_auth)

        req_buf: bytes = self.local.recv(4)
        logging.debug(f"reading framed req_buf: {req_buf}")

        addr_buf: bytes = self.local.recv(6)
        host_int, port = struct.unpack(">IH", addr_buf)
        host_str = socket.inet_ntoa(struct.pack(">I", host_int))

        logging.info(f"remoting is: {host_str}:{port}")

        remote = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        remote.connect((host_str, port))

        suc_buf: bytes = bytes([0x05, 0x00, 0x00, 0x01, 0, 0, 0, 0, 0, 0])

        self.local.sendall(suc_buf)

        self.copy_bidirectional(remote)


class Http(Proxy):
    def __init__(self, local: socket.socket, addr: Tuple[str, int], bufsize: int = 1024 * 32):
        super().__init__(local, addr, bufsize)

    def handle(self):
        pass


class Dispatch:
    def __init__(self, local: socket.socket, addr: Tuple[str, int]):
        self.local = local
        self.addr = addr

    def handle(self):
        flag: bytes = self.local.recv(1, socket.MSG_PEEK)

        if flag[0] == 0x05:
            proxy = Socks5(self.local, self.addr)
            client_thread = threading.Thread(target=proxy.handle)
            client_thread.start()
