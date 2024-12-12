

from typing import Any


class Player():

    def __init__(self, websocket, id):
        self.websocket = websocket
        self.id = id
        self.turn = False
        self.grid = []
        self.grid_recived = False
        self.game_state = 0
    
    def set_grid(self, grid):
        self.grid = grid

    def set_turn(self, turn):
        self.turn = turn

    def __repr__(self) -> str:
        return f"{self.id} on socket {self.websocket.id}"

    def __str__(self):
        return f"{self.id} on socket {self.websocket.id}"