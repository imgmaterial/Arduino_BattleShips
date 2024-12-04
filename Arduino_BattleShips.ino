#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

#include "defines.h"

#include <WebSockets2_Generic.h>

using namespace websockets2_generic;

WebsocketsClient client;

bool connected = false;

//LED button controlls
int button_pin = 3;

//Joystick variables
int joy_x = A1;
int joy_y = A0;

int grid_x = 0;
int grid_y = 0;

bool your_turn = true;
bool button_pressed =false;
typedef enum{
  Up = 0,
  Down = 1,
  Right = 2,
  Left = 3,
  None = 4,
}JoyPos;

int enemy_grid[5][5];

int battleship_grid[5][5];

unsigned long previouse_millis;
// display connected to the HW I2C (this is the best option)
U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup(void) {
  pinMode(button_pin, INPUT);
  u8g2.begin();
  intializeShips();
  client_setup();
  previouse_millis = millis();
}

void loop(void) {
  reconnect_to_server();
  if (client.available()) 
  {
    client.poll();
  }
  u8g2.clearBuffer();
  readJoy();
  read_button();
  draw_grid();
  draw_cursor(grid_x,grid_y);
  render_current_grid();
  u8g2.sendBuffer();
}


void read_button(){
  PinStatus button = digitalRead(button_pin);
  if (!your_turn){return;}
  if (button == HIGH){
    button_pressed = false;
  }
  else{
    if (!button_pressed){
      attack_grid_point(enemy_grid);
      send_grid();
    }
    button_pressed = true;
  }
}

void attack_grid_point(int (*grid)[5]){
  if (grid[grid_x][grid_y] == 1 || grid[grid_x][grid_y] == 2){
    grid[grid_x][grid_y] = 2; 
  }
  else{
    grid[grid_x][grid_y] = 3;
    your_turn = false;
  }
}

void render_current_grid(){
  if (your_turn){
    draw_grid_details(enemy_grid);
  }
  else{
    draw_grid_details(battleship_grid);
  }
}

void intializeShips(){
  battleship_grid[0][0] = 1;
  battleship_grid[0][1] = 1;
  battleship_grid[2][0] = 1;
  battleship_grid[3][0] = 1;
  battleship_grid[4][1] = 1;
  battleship_grid[4][2] = 1;
  battleship_grid[4][3] = 1;

  enemy_grid[0][0] = 1;
  enemy_grid[0][1] = 1;
  enemy_grid[2][0] = 1;
  enemy_grid[3][0] = 1;
  enemy_grid[4][1] = 1;
  enemy_grid[4][2] = 1;
  enemy_grid[4][3] = 1;
}

void draw_grid(){
  for (int i = 12; i <= 112; i = i + 20){
    u8g2.drawHLine(12, i, 100);
    u8g2.drawVLine(i, 12, 100);
  }
}

void drawCross(u8g2_uint_t start_x, u8g2_uint_t start_y, u8g2_uint_t width, u8g2_uint_t height){
  u8g2.drawLine(start_x, start_y, start_x + width, start_y + height);
  u8g2.drawLine(start_x, start_y + height, start_x + width, start_y);
}

void drawShade(u8g2_uint_t start_x, u8g2_uint_t start_y, u8g2_uint_t width, u8g2_uint_t height){
  u8g2.drawLine(start_x, start_y, start_x + width, start_y + height);
  u8g2.drawLine(start_x + width/2, start_y, start_x + width, start_y + height/2);
  u8g2.drawLine(start_x, start_y + height/2, start_x + width/2, start_y + height);
}

void draw_grid_details(int grid[5][5]){
  int start_x = 12;
  int start_y = 12;
  int radius = 10;
  int offset = 20;
  for (int i  = 0; i < 5;i++){
    for (int j = 0;j < 5;j++){
      switch (grid[i][j]){
        case (1):
          if (!your_turn){
            drawShade(start_x+i*offset, start_y+j*offset, 20, 20);
          }
          break;
        case (2):
          drawCross(start_x+i*offset, start_y+j*offset, 20, 20);
          break;
        case (3):
          u8g2.drawCircle(start_x + radius + i*offset, start_y + radius + j*offset, 8);
          break;
      }
    }
  }
}

void draw_cursor(int x, int y){
  u8g2.drawBox(12+20*x, 12+20*y, 20, 20);
}

void readJoy(){
  int pos_x = analogRead(joy_x);
  int pos_y = analogRead(joy_y);
  if (pos_x <300){
    processJoyInput(Left);
  }
  else if(pos_x > 700){
    processJoyInput(Right);
  }
  if (pos_y > 700 && pos_y <900){
    processJoyInput(Down);
  }
  else if(pos_y <300){
    processJoyInput(Up);
  }
  else if(pos_y > 900){
    if (!your_turn){
      your_turn = true;
    }
  }
}

void processJoyInput(JoyPos input){
  unsigned long current_millis = millis();
  if (current_millis - previouse_millis > 300){
    processJoyPos(input);
    previouse_millis = current_millis;
  }
}

void processJoyPos(JoyPos input){
  switch (input){
    case Up:
      grid_y--;
      if (grid_y < 0){
        grid_y = 0;
      }
      break;
    case Down:
      grid_y++;
      if (grid_y > 4){
        grid_y = 4;
      }
      break;
    case Right:
      grid_x++;
      if (grid_x > 4){
        grid_x = 4;
      }
      break;
    case Left:
      grid_x--;
      if (grid_x < 0){
        grid_x = 0;
      }
      break;
  }
}


void onEventsCallback(WebsocketsEvent event, String data) 
{
  (void) data;
  
  if (event == WebsocketsEvent::ConnectionOpened) 
  {
    Serial.println("Connnection Opened");
  } 
  else if (event == WebsocketsEvent::ConnectionClosed) 
  {
    Serial.println("Connnection Closed");
    connected = false;
  } 
  else if (event == WebsocketsEvent::GotPing) 
  {
    Serial.println("Got a Ping!");
  } 
  else if (event == WebsocketsEvent::GotPong) 
  {
    Serial.println("Got a Pong!");
  }
}

void client_setup() 
{
  Serial.begin(115200);
  while (!Serial && millis() < 5000);

  Serial.println("\nStarting RP2040-Client with WiFiNINA on " + String(BOARD_NAME));
  Serial.println(WEBSOCKETS2_GENERIC_VERSION);
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) 
  {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    return;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) 
  {
    Serial.println("Please upgrade the firmware");
  }

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  // Connect to wifi
  WiFi.begin(ssid, password);

  // Wait some time to connect to wifi
  for (int i = 0; i < 15 && WiFi.status() != WL_CONNECTED; i++)
  {
    Serial.print(".");
    delay(1000);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Connected to Wifi, IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Connecting to WebSockets Server @");
    Serial.println(websockets_server_host);
  }
  else
  {
    Serial.println("\nNo WiFi");
    return;
  }

  // run callback when messages are received
  client.onMessage([&](WebsocketsMessage message) 
  {
    Serial.print("Got Message: ");
    Serial.println(message.data());
  });

  // run callback when events are occuring
  client.onEvent(onEventsCallback);
  
  // try to connect to Websockets server
  reconnect_to_server();
}

void reconnect_to_server(){
  if (!connected){
    connected = client.connect(websockets_server_host, websockets_server_port, "/");
    if (connected) 
    {
      Serial.println("Connected!");
    
      send_client_registration();
      send_grid();
    } 
    else 
    {
      Serial.println("Not Connected!");
    }
  }
}

void send_client_registration(){
  String msg = "REGISTER:" + String(device_identifier);
  client.send(msg);
}

void send_grid(){
  String msg = "GRID:" + grid_to_string(enemy_grid);
  client.send(msg);
}

void send_attack(int x, int y){
  String msg = "ATTACK:" + String(x) + ":" + String(y);
  client.send(msg);
}

String grid_to_string(int grid[5][5]){
  String grid_str = String("");
  for (int i = 0; i<5;i++){
    for (int j = 0; j<5;j++){
      grid_str = grid_str + String(grid[j][i]);
      if (j != 4){
        grid_str = grid_str + String(",");
      }
    }
    if (i != 4){
        grid_str = grid_str + ";";
    }
  }
  Serial.println(grid_str);
  return grid_str;
}