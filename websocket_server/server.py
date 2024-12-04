import asyncio
from websockets.asyncio.server import serve

connected = set()

async def handler(websocket):
    # Register.
    connected.add(websocket)
    async for message in websocket:
        print(connected)
        await websocket.send(message)
    try:
        for socket in connected:
            await socket.send("Hello")
        await asyncio.sleep(10)
        print(connected)
    finally:
        connected.remove(websocket)
        print(connected)
    


async def main():
    async with serve(handler, "10.81.0.23", 8080) as server:
        await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(main())
