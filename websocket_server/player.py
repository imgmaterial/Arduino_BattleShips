

from typing import Any


class Player():

    def __init__(self, websocket, id):
        self.webscoket = websocket
        self.id = id

    def __repr__(self) -> str:
        return f"{self.id} on socket {self.webscoket.id}"

    def __str__(self):
        return f"{self.id} on socket {self.webscoket.id}"