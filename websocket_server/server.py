import asyncio
from websockets.asyncio.server import serve
from player import Player
import random

players = []
grid = []
async def handler(websocket):
    async for message in websocket:
        print(message)
        await parse_message(websocket, message)
        await websocket.send(message)
    


async def main():
    async with serve(handler, "192.168.1.150", 5000) as server:
        await server.serve_forever()


async def send_turn_info():
    for i in range(len(players)):
        msg = f"TURN:{players[i].turn}"
        await players[i].websocket.send(msg)


def grid_to_string(grid):
    merged_list = sum(grid, [])
    print(merged_list)
    output = ','.join(str(x) for x in merged_list)
    return output

def set_player_states(state):
    for player in players:
        player.game_state = state

async def send_player_state(player):
    message = f"STATE:{int(player.turn)}:{player.game_state}:{grid_to_string(player.grid)}"
    await player.websocket.send(message)


async def register_player(websocket_id, player_name):
    new_player = Player(websocket_id, player_name)
    print(new_player)
    _players = players
    for player in _players:
        if player.id == player_name:
            players[players.index(player)].websocket = new_player.websocket
            await send_player_state(player)
            return
    if len(players) > 1:
        return
    players.append(new_player)
    print(players)
    if (len(players) > 1):
        await broadcast_to_registered("GAMESTART")
        set_player_states(1)

async def check_victory_conditions():
    for player in players:
        if not any(1 in sublist for sublist in player.grid):
            await broadcast_to_registered(f"GAMEEND:{player.id}")
            set_player_states(3)

async def broadcast_to_registered(message):
    for player in players:
        await player.websocket.send(message)

async def change_turns():
    for player in players:
        if player.turn == True:
            player.turn = False
        else:
            player.turn = True
    await send_turn_info()


def attack_point(x,y, attacked_player_index):
    attacked_player = players[attacked_player_index]
    if attacked_player.grid[y][x] == 1 or attacked_player.grid[y][x] == 2:
        attacked_player.grid[y][x] = 2
        print(attacked_player.grid)
        return 2
    elif attacked_player.grid[y][x] == 0 or attacked_player.grid[y][x] == 3:
        attacked_player.grid[y][x] = 3
        print(attacked_player.grid)
        return 3
    

def other_player_index(index):
    if index == 0:
        return 1
    elif index == 1:
        return 0

def check_player_grid_recived():
    if (len(players) < 2):
        return False
    for player in players:
        if not player.grid_recived:
            return False
    return True

def initialise_player_turns():
    random_turn = random.randint(0,1)
    players[random_turn].turn = True

async def parse_message(websocket,message):
    split = message.split(":")
    match split[0]:
        case "REGISTER":
            await register_player(websocket, split[1])
            await websocket.send(str(websocket.id) + " registered as " + split[1])
        case "GRID":
            for player in players:
                if split[2] == player.id:
                    player.grid = [list(map(int, x.split(","))) for x in split[1].split(";")]
                    player.grid_recived = True
                    if (check_player_grid_recived()):
                        initialise_player_turns()
                        await send_turn_info()
                    await websocket.send("Recieved grid")
                    set_player_states(2)
        case "ATTACK":
            for player in players:
                if split[1] == player.id:
                    result = attack_point(int(split[2]),int(split[3]), other_player_index(players.index(player)))
                    await websocket.send(f"ATTACKRESULT:{split[2]}:{split[3]}:{result}")
                    await players[other_player_index(players.index(player))].websocket.send(f"DEFENCE:{split[2]}:{split[3]}:{result}")
                    await check_victory_conditions()
                    if result == 3:
                        await change_turns()
        case _:
            await websocket.send("Unknown command")



if __name__ == "__main__":
    asyncio.run(main())
