

from typing import Any


class Player():

    def __init__(self, websocket, id):
        self.websocket = websocket
        self.id = id

    def __repr__(self) -> str:
        return f"{self.id} on socket {self.websocket.id}"

    def __str__(self):
        return f"{self.id} on socket {self.websocket.id}"