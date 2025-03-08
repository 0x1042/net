import socket
import threading
import proxy
from typing import Tuple

class Dispatch:
    def __init__(self, local: socket.socket, addr: Tuple[str, int]):
        self.local = local
        self.addr = addr

    def handle(self):
        flag: bytes = self.local.recv(1, socket.MSG_PEEK)

        if flag[0] == 0x05:
            proxy = proxy.SocksProxy(self.local, self.addr)
            client_thread = threading.Thread(target=proxy.handle)
            client_thread.start()
        else:
            proxy = proxy.HttpProxy(self.local, self.addr)
            client_thread = threading.Thread(target=proxy.handle)
            client_thread.start()
