import asyncio
from websockets.asyncio.server import serve
from player import Player

players = set()
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
    players.add(new_player)
    print(players)


async def broadcast_to_registered(message):
    for player in players:
        player.websocket.send(message)

def attack_point(x,y):
    if grid[y][x] == 1:
        return 2
    elif grid[y][x] == 0:
        return 3
    print(grid)

async def parse_message(websocket,message):
    split = message.split(":")
    match split[0]:
        case "REGISTER":
            register_player(websocket, split[1])
            await websocket.send(str(websocket.id) + " registered as " + split[1])
        case "GRID":
            grid = [list(map(int, x.split(","))) for x in split[1].split(";")]
            await websocket.send("Recieved grid")
        case "ATTACK":
            result = attack_point(split[2],split[3])
            await websocket.send(f"ATTACKRESULT:{split[2]}:{split[3]}:{result}")
        case _:
            await websocket.send("Unknown command")




if __name__ == "__main__":
    asyncio.run(main())
