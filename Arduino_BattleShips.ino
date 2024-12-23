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

typedef enum{
  Vertical = 0,
  Horizontal = 1,
}Orientation;

typedef struct{
  int size;
  Orientation orientation;
}Ship;

typedef enum{
  MenuState = 0,
  PlacementState = 1,
  ActiveState = 2,
  PostGame = 3,
}GameState;


typedef enum{
  Up = 0,
  Down = 1,
  Right = 2,
  Left = 3,
  None = 4,
}JoyPos;

bool button_pressed =false;
Ship ships[] = {{3, Vertical}, {3, Vertical}, {2, Vertical}};
//Game state variables
int grid_x = 0;
int grid_y = 0;
bool your_turn = false;
bool victory = false;
int current_ship_placement = 0;
GameState current_state = MenuState;
int enemy_grid[5][5];
int battleship_grid[5][5];
unsigned long previouse_millis;
// display connected to the HW I2C (this is the best option)
U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup(void) {
  pinMode(button_pin, INPUT);
  u8g2.begin();
  client_setup();
  previouse_millis = millis();
}

void loop(void) {
  switch (current_state){
    case MenuState:
      MenuState_update();
      break;
    case PlacementState:
      PlacementState_update();
      break;
    case ActiveState:
      ActiveState_update();
      break;
    case PostGame:
      PostGame_update();
      break;
  }
  reconnect_to_server();
  if (client.available()) 
  {
    client.poll();
  }
}

void GameReset(){
  grid_x = 0;
  grid_y = 0;
  your_turn = false;
  victory = false;
  current_ship_placement = 0;
  current_state = MenuState;
  enemy_grid[5][5];
  battleship_grid[5][5];
  previouse_millis = 0;
}

void MenuState_update(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tf);
  render_connecting();
  u8g2.sendBuffer();
}

void PlacementState_update(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tf);
  read_button_placement();
  u8g2.drawStr(12, 124, "Placement");
  draw_grid();
  read_joy_placement();
  draw_placement_cursor(ships[current_ship_placement], grid_x, grid_y);
  draw_grid_details(battleship_grid);
  u8g2.sendBuffer();
}

void ActiveState_update(){
  u8g2.clearBuffer();
  readJoy();
  read_button();
  draw_grid();
  draw_cursor(grid_x,grid_y);
  render_current_grid();
  u8g2.sendBuffer();
}

void PostGame_update(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tf);
  read_button_placement();
  u8g2.drawStr(12, 64, victory?"You Won":"You Lost");
  u8g2.sendBuffer();
}

void read_button_placement(){
  PinStatus button = digitalRead(button_pin);
  if (button == HIGH){
    button_pressed = false;
  }
  else{
    if (!button_pressed){
      if (can_place_ships(ships[current_ship_placement], grid_x, grid_y)){
        add_ship_to_grid(ships[current_ship_placement], grid_x, grid_y);
        current_ship_placement++;
        if (current_ship_placement > 2){
          //current_state = ActiveState;
          send_grid();
        }
      }
    }
    button_pressed = true;
  }
}


void render_connecting(){
  if (connected){
    u8g2.drawStr(12, 64, "Connected");
    u8g2.drawStr(12, 84, "Waiting for");
    u8g2.drawStr(12, 104, "game start");
  }
  else{
    u8g2.drawStr(12, 64, "Connecting");
  }
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
    }
    button_pressed = true;
  }
}

void attack_grid_point(int (*grid)[5]){
  send_attack(grid_x, grid_y);
}

void render_current_grid(){
  if (your_turn){
    draw_grid_details(enemy_grid);
  }
  else{
    draw_grid_details(battleship_grid);
  }
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
          drawShade(start_x+i*offset, start_y+j*offset, 20, 20);
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

void add_ship_to_grid(Ship ship, int x, int y){
  if (ship.orientation == Vertical){
    for (int i = y;i<=(y + ship.size - 1);i++){
      battleship_grid[grid_x][i] = 1;
    }
  }
  if (ship.orientation == Horizontal){
    for (int i = x;i<=(x + ship.size - 1);i++){
      battleship_grid[i][grid_y] = 1;
    }
  }
}

void draw_placement_cursor(Ship ship, int x, int y){
  if (ship.orientation == Vertical){
    for (int i = y;i<=(y + ship.size - 1);i++){
      u8g2.drawBox(12+20*x, 12+20*i, 20, 20);
    }
  }
  if (ship.orientation == Horizontal){
    for (int i = x;i<=(x + ship.size - 1);i++){
      u8g2.drawBox(12+20*i, 12+20*y, 20, 20);
    }
  }
}

void read_joy_placement(){
  int pos_x = analogRead(joy_x);
  int pos_y = analogRead(joy_y);
  if (pos_x <300){
    process_joy_input_placement(Left);
  }
  else if(pos_x > 700){
    process_joy_input_placement(Right);
  }
  if (pos_y > 700 && pos_y <900){
    process_joy_input_placement(Down);
  }
  else if(pos_y <300){
    process_joy_input_placement(Up);
  }
  else if(pos_y > 900){
    turn_ship_placement();
  }
}

void turn_ship_placement(){
  unsigned long current_millis = millis();
    if (current_millis - previouse_millis > 300){
      if (ships[current_ship_placement].orientation == Vertical){
        ships[current_ship_placement].orientation = Horizontal;
      }
      else{
        ships[current_ship_placement].orientation = Vertical;
      }
      fix_out_of_bounds_position();
      previouse_millis = current_millis;
    }
}

void fix_out_of_bounds_position(){
  Ship ship = ships[current_ship_placement];
  if (ship.orientation == Horizontal && (grid_x + ship.size - 1) > 4){
    grid_x = 4 - ship.size + 1;
  }
  if (ship.orientation == Vertical && (grid_y + ship.size - 1) > 4){
    grid_y = 4 - ship.size + 1;
  }
}

void process_joy_input_placement(JoyPos input){
  unsigned long current_millis = millis();
  if (current_millis - previouse_millis > 300){
    process_joy_pos_placement(input);
    previouse_millis = current_millis;
  }
}

void process_joy_pos_placement(JoyPos input){
  switch (input){
    case Up:
      if (check_movement_boundaries_placement(ships[current_ship_placement], grid_x, grid_y - 1)){
        grid_y--;
      }
      break;
    case Down:
      if (check_movement_boundaries_placement(ships[current_ship_placement], grid_x, grid_y + 1)){
        grid_y++;
      }
      break;
    case Right:
      if (check_movement_boundaries_placement(ships[current_ship_placement], grid_x + 1, grid_y)){
        grid_x++;
      }
      break;
    case Left:
      if (check_movement_boundaries_placement(ships[current_ship_placement], grid_x - 1, grid_y)){
        grid_x--;
      }
      break;
  }
  Serial.print("x:");
  Serial.print(grid_x);
  Serial.print(" y:");
  Serial.print(grid_y);
  Serial.print("\n");
}

bool can_place_ships(Ship ship, int x, int y){
  if (ship.orientation == Vertical){
    for (int i = y;i<=(y + ship.size - 1);i++){
      if (battleship_grid[grid_x][i] == 1){
        return false;
      }
    }
  }
  if (ship.orientation == Horizontal){
    for (int i = x;i<=(x + ship.size - 1);i++){
      if (battleship_grid[i][grid_y] == 1){
        return false;
      }
    }
  }
  return true;
}

bool check_movement_boundaries_placement(Ship ship, int x, int y){
  if (ship.orientation == Vertical){
    if ((y + ship.size -1 < 5) && y >= 0 && x >= 0 && x<5){
      return true;
    }
  }
  if (ship.orientation == Horizontal){
    if ((x + ship.size -1 < 5 && x >= 0 && y >=0 && y < 5)){
      return true;
    }
  }
  return false;
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
    Serial.println("Click");
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
    //Serial.println(message.data());
    parse_message(message.data());
  });

  // run callback when events are occuring
  client.onEvent(onEventsCallback);
  
  // try to connect to Websockets server
  reconnect_to_server();
}

void parse_message(String message){
  Serial.println(message);
  if (message.startsWith("ATTACKRESULT")){
    //12 15 17
    enemy_grid[String(message[13]).toInt()][String(message[15]).toInt()] = String(message[17]).toInt();
  }
  else if(message.startsWith("TURN")){
    if (current_state == PlacementState){
      current_state = ActiveState;
    }
    if (message.endsWith("True")){
      your_turn = true;
    }
    else if (message.endsWith("False")){
      your_turn = false;
    }
    display_message(your_turn?"Your Turn":"Enemy Turn");
  }
  else if (message.startsWith("GAMESTART")){
    if (current_state == MenuState){
      current_state = PlacementState;
    }
    display_message("Starting Game");
  }
  else if (message.startsWith("DEFENCE")){
    //DEFENCE:3:3:3
    battleship_grid[String(message[8]).toInt()][String(message[10]).toInt()] = String(message[12]).toInt();
  }
  else if (message.startsWith("GAMEEND")){
    if (message.endsWith(device_identifier)){
      victory = false;
    }
    else{
      victory = true;
    }
    current_state = PostGame;
  }
  else if(message.startsWith("STATE")){
    recive_state_after_reconnect(message);
  }
}

void display_message(String message){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tf);
  u8g2.drawStr(12, 64, message.c_str());
  u8g2.sendBuffer();
  delay(650);
}

void reconnect_to_server(){
  if (!connected){
    connected = client.connect(websockets_server_host, websockets_server_port, "/");
    if (connected) 
    {
      Serial.println("Connected!");
    
      send_client_registration();
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
  String msg = "GRID:"+ grid_to_string(battleship_grid)+ ":" +String(device_identifier);
  client.send(msg);
}

void send_attack(int x, int y){
  String msg = "ATTACK:" + String(device_identifier) +":"+ String(x) + ":" + String(y);
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
//Parsing the state string
//i0  0  0  0  0  1  1  1  1  1  2 
//j0  1  2  3  4  0  1  2  3  4  0
// 10 12 14 16 18 20 22 24 26 28 30
//Example STATE:1:2:2,0,1,1,1,2,0,0,0,0,2,0,1,0,0,3,0,1,0,0,0,0,0,0,0:1,0,1,1,0,1,0,0,0,1,0,0,3,0,1,0,0,0,0,1,0,0,0,0,0
void recive_state_after_reconnect(String state){
  your_turn = String(state[6]).toInt();
  current_state = (GameState)String(state[8]).toInt();
  if (current_state < 2){return;}
  int initial_index = 10;
  for (int i = 0;i < 5;i++){
    for (int j = 0; j<5;j++){
      battleship_grid[j][i] = String(state[initial_index + j*2 + i*10]).toInt();
    }
  }
  initial_index = 60;
  for (int i = 0;i < 5;i++){
    for (int j = 0; j<5;j++){
      enemy_grid[j][i] = String(state[initial_index + j*2 + i*10]).toInt();
    }
  }
}