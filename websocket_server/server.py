import asyncio
from websockets.asyncio.server import serve
from player import Player

players = set()
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


def register_player(websocket_id, player_name):
    new_player = Player(websocket_id, player_name)
    for player in players:
        if player.id == player_name:
            players.remove(player)
    if len(players) > 1:
        return
    players.add(new_player)
    print(players)


async def broadcast_to_registered(message):
    for player in players:
        player.websocket.send(message)

async def change_turns():
    global turns
    if turns == [True, False]:
        turns = [False, True]
    elif turns == [False, True]:
        turns = [True, False]
    for i in range(len(players)):
        msg = f"TURN:{turns[i]}"
        await list(players)[i].websocket.send(msg)

def attack_point(x,y):
    if grid[y][x] == 1 or grid[y][x] == 2:
        grid[y][x] = 2
        print(grid)
        return 2
    elif grid[y][x] == 0 or grid[y][x] == 3:
        grid[y][x] = 3
        print(grid)
        return 3

async def parse_message(websocket,message):
    split = message.split(":")
    match split[0]:
        case "REGISTER":
            register_player(websocket, split[1])
            await websocket.send(str(websocket.id) + " registered as " + split[1])
        case "GRID":
            global grid
            grid = [list(map(int, x.split(","))) for x in split[1].split(";")]
            await websocket.send("Recieved grid")
        case "ATTACK":
            print(split)
            result = attack_point(int(split[2]),int(split[3]))
            await websocket.send(f"ATTACKRESULT:{split[2]}:{split[3]}:{result}")
            if result == 3:
                await change_turns()
        case _:
            await websocket.send("Unknown command")




if __name__ == "__main__":
    asyncio.run(main())
