# Arduino Battleships

## Description
A classic game of Battleships designed to be played by two people using two Arduino RP2040 devices and a central server. This project implements the traditional Battleships game in a modern, electronic format using Arduino RP2040 microcontrollers. Players can enjoy the classic naval combat strategy game with a digital twist, making it more interactive and engaging.

## Table of Contents
- [Features](#features)
- [Prerequisites](#prerequisites)
- [Hardware Setup](#hardware-setup)
- [Installation](#installation)
- [Server Component](#server-component)
- [Usage](#usage)
- [License](#license)

## Features
- Two-player gameplay using Arduino RP2040 devices
- Central server communication for game coordination
- Classic Battleships gameplay mechanics
- Written in C++ for Arduino platform
- Real-time gameplay with WebSocket communication
- Secure and reliable game state management

## Prerequisites
- 2x Arduino RP2040 boards
- Development environment with Arduino IDE
- Python 3.x for server component
- Network connectivity between devices
- Required Arduino libraries (list to be added)
- Central server setup capability

## Hardware Setup
(Hardware setup instructions and wiring diagrams to be added)

## Installation

### Arduino Setup
1. Clone the repository:
   ```bash
   git clone https://github.com/imgmaterial/Arduino_BattleShips.git
2. Open the Arduino project:
   - Launch Arduino IDE
   - Open the project folder
   - Install required dependencies through Arduino Library Manager

3. Configure Network Settings:
   - Update WiFi credentials
   - Set server IP address
   - Configure any additional network parameters

4. Upload to Devices:
   - Connect Arduino RP2040 boards
   - Select appropriate board and port
   - Upload the code to both devices

### Server Setup
1. Install Python requirements
2. Configure network settings
3. Launch server application

## Server Component

### Server Requirements
- Python 3.x
- websockets library (`pip install websockets`)
- Network connectivity between devices

### Server Setup
1. Install the required Python package:
   ```bash
   pip install websockets
2. Configure the server IP:
   ```python
   # In server.py, modify the IP address to match your network setup
   async with serve(handler, "YOUR_IP_ADDRESS", YOUR_PORT) as server:
3. Run the server:
    ```bash
    python server.py

### Server Communication Protocol
The server implements the following message protocols:

#### Player Registration and Setup
- **REGISTER**: `REGISTER:player_name`
  - Registers a new player with the server
- **GRID**: `GRID:grid_data:player_id`
  - Sends the player's ship placement grid
- **BOATS**: `BOATS:data1:data2:data3:player_id`
  - Communicates boat placement information

#### Gameplay Commands
- **ATTACK**: `ATTACK:player_id:x:y`
  - Sends attack coordinates
- **Server Responses**:
  - `GAMESTART`: Game initiation
  - `TURN:state`: Turn indicator
  - `ATTACKRESULT:x:y:result`: Attack outcome
  - `DEFENCE:x:y:result`: Incoming attack notification
  - `GAMEEND:player_id`: Victory announcement

### Game States
1. Player Registration
2. Ship Placement Phase
3. Active Gameplay
4. Game End

## Usage

### Game Setup
1. Power on both Arduino RP2040 devices
2. Start the Python server
3. Wait for both devices to connect
4. Follow on-screen instructions for ship placement

### Gameplay
1. Players take turns selecting attack coordinates
2. System provides feedback on hits and misses
3. Game continues until all ships of one player are sunk
4. Victory is announced to both players

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
