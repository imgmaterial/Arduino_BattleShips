import asyncio
from websockets.asyncio.server import serve
from player import Player

players = []
turns = [True, False]
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
        msg = f"TURN:{turns[i]}"
        await players[i].websocket.send(msg)

async def register_player(websocket_id, player_name):
    new_player = Player(websocket_id, player_name)
    print(new_player)
    _players = players
    for player in _players:
        if player.id == player_name:
            print("went here")
            players[players.index(player)] = new_player
    if len(players) > 1:
        return
    players.append(new_player)
    print(players)
    if (len(players) > 1):
        await broadcast_to_registered("GAMESTART")
        await send_turn_info()


async def broadcast_to_registered(message):
    for player in players:
        await player.websocket.send(message)

async def change_turns():
    global turns
    if turns == [True, False]:
        turns = [False, True]
    elif turns == [False, True]:
        turns = [True, False]
    await send_turn_info()


def attack_point(x,y, attacked_player_index):
    attacked_player = players[attacked_player_index]
    if attacked_player.grid[y][x] == 1 or attacked_player.grid[y][x] == 2:
        attacked_player.grid[y][x] = 2
        print(grid)
        return 2
    elif attacked_player.grid[y][x] == 0 or attacked_player.grid[y][x] == 3:
        attacked_player.grid[y][x] = 3
        print(grid)
        return 3

def other_player_index(index):
    if index == 0:
        return 1
    elif index == 1:
        return 0

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
                    await websocket.send("Recieved grid")
        case "ATTACK":
            for player in players:
                if split[1] == player.id:
                    result = attack_point(int(split[2]),int(split[3]), other_player_index(players.index(player)))
                    await websocket.send(f"ATTACKRESULT:{split[2]}:{split[3]}:{result}")
                    await players[other_player_index(players.index(player))].websocket.send(f"DEFENCE:{split[2]}:{split[3]}:{result}")
                    if result == 3:
                        await change_turns()
        case _:
            await websocket.send("Unknown command")



if __name__ == "__main__":
    asyncio.run(main())
