import asyncio
from websockets.asyncio.server import serve

connected = set()

async def handler(websocket):
    async for message in websocket:
        print(message)
        await websocket.send(message)
    


async def main():
    async with serve(handler, "192.168.1.150", 5000) as server:
        await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(main())
