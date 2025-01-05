#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_ILI9341.h"
#include "boat.h"
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

typedef struct{
  int x;
  int y;
}Vec2;

typedef enum{
  Vertical = 0,
  Horizontal = 1,
}Orientation;

typedef struct{
  int size;
  Orientation orientation;
  Vec2 position[3];
  bool square_hit[3] = {true,true,true};
  bool destroyed = false;
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
Ship ships[] = {{2, Vertical},{3, Vertical}, {3, Vertical}};
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
int boats[3] = {0,0,0};
int enemy_boats[3] = {0,0,0};
// display connected to the HW I2C (this is the best option)
U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

//TFT Screen definition
#define TFT_DC 9
#define TFT_CS 10

// Screen dimensions
int screenWidth = 240;
int screenHeight = 320;

// Grid Varibales
int squareSize = 35; //Can be changed to make grid smaller or bigger
int gridSize = 5;    
// Calculate the grid total width
int gridWidth = squareSize * gridSize;

int startX = 20; // Center horizontally and offset left
int startY = (screenWidth - gridWidth) / 2 + 15;     // Center vertically
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);


bool start_connection = false;
void setup(void) {
  //tft.fillScreen(ILI9341_BLACK);
  pinMode(button_pin, INPUT);
  if (SCREEN_TYPE == "I2C"){
    u8g2.begin();
  }
  else if (SCREEN_TYPE == "TFT"){
    tft.begin();
    tft.setRotation(3);
  }
  Menu_initialise();
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
  if (start_connection){
    reconnect_to_server();
  }
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
  if (SCREEN_TYPE == "I2C"){
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB14_tf);
  }
  if (SCREEN_TYPE == "I2C"){
    render_connecting();
    u8g2.sendBuffer();
  }
  read_button_menu();
}

void PlacementState_update(){
  if (SCREEN_TYPE == "I2C"){
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB14_tf);
  }
  read_button_placement();
  read_joy_placement();
  if (SCREEN_TYPE == "I2C"){
    u8g2.drawStr(12, 124, "Placement");
    draw_grid();
    draw_placement_cursor(ships[current_ship_placement], grid_x, grid_y);
    draw_grid_details(battleship_grid);
    u8g2.sendBuffer();
  }
  else if (SCREEN_TYPE == "TFT"){
    TFT_draw_grid();
    TFT_draw_placement_cursor(ships[current_ship_placement],grid_x, grid_y);
    TFT_draw_legend();
  }
}

void ActiveState_update(){
  if (SCREEN_TYPE == "I2C"){
    u8g2.clearBuffer();
  }
  readJoy();
  read_button();
  if (SCREEN_TYPE == "I2C"){
    draw_grid();
    draw_cursor(grid_x,grid_y);
    render_current_grid();
    u8g2.sendBuffer();
  }
  else if(SCREEN_TYPE == "TFT"){
    TFT_draw_cursor(grid_x, grid_y);
  }
}

void PostGame_update(){
  if (SCREEN_TYPE == "I2C"){
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB14_tf);
    u8g2.drawStr(12, 64, victory?"You Won":"You Lost");
    u8g2.sendBuffer();
  }
}

void Menu_initialise(){
  current_state = MenuState;
  Serial.println(current_state);
  if (SCREEN_TYPE == "TFT"){
    tft.fillScreen(ILI9341_BLACK);
    TFT_draw_menu_state();
  }
}

void Menu_to_Placement_transition(bool check_state){
  if (check_state && current_state != MenuState){return;}
  current_state = PlacementState;
  Serial.println(current_state);
  if (SCREEN_TYPE == "TFT"){
    tft.fillScreen(ILI9341_BLACK);
    TFT_draw_grid();
    TFT_draw_legend();
    TFT_draw_boats(boats);
  }
}

void Placement_to_Active_transition(bool check_state){
  if (check_state && current_state != PlacementState){return;}
  current_state = ActiveState;
  Serial.println(current_state);
  if (SCREEN_TYPE == "TFT"){
    tft.fillScreen(ILI9341_BLACK);
    TFT_draw_grid();
    TFT_draw_legend();
    TFT_render_current_grid();
    TFT_draw_boats(boats);
    TFT_draw_details(your_turn);
  }
}

void Active_to_Postgame_transition(bool check_state){
  if (check_state && current_state != ActiveState){return;}
  current_state = PostGame;
  Serial.println(current_state);
  if (SCREEN_TYPE == "TFT"){
    tft.fillScreen(ILI9341_BLACK);
    TFT_draw_postgame_state();
  }
}

void read_button_menu(){
  PinStatus button = digitalRead(button_pin);
  if (button == HIGH){
    button_pressed = false;
  }
  else{
    if (!button_pressed){
      start_connection = true;
    }
    button_pressed = true;
  }
}

void read_button_placement(){
  PinStatus button = digitalRead(button_pin);
  if (button == HIGH){
    button_pressed = false;
  }
  else{
    if (!button_pressed){
      if (can_place_ships(ships[current_ship_placement], grid_x, grid_y)){
        add_ship_to_grid(&ships[current_ship_placement], grid_x, grid_y);
        if (SCREEN_TYPE == "TFT"){
          TFT_draw_placement_after_cursor(ships[current_ship_placement], grid_x, grid_y);
        }
        current_ship_placement++;
        if (current_ship_placement > 2){
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

void add_ship_to_grid(Ship* ship, int x, int y){
  int ship_square = 0;
  if (ship->orientation == Vertical){
    for (int i = y;i<=(y + ship->size - 1);i++){
      battleship_grid[grid_x][i] = 1;
      ship->position[ship_square].x = x;
      ship->position[ship_square].y = i;
      ship->square_hit[ship_square] = false;
      ship_square++;
    }
  }
  if (ship->orientation == Horizontal){
    for (int i = x;i<=(x + ship->size - 1);i++){
      battleship_grid[i][grid_y] = 1;
      ship->position[ship_square].x = i;
      ship->position[ship_square].y = y;
      ship->square_hit[ship_square] = false;
      ship_square++;
    }
  }
  for (int i = 0;i<3;i++){
    Serial.print(ship->square_hit[i]);
  }
  Serial.print("\n");
}

bool check_if_ship_destroyed(Ship* ship){
  if (ship->destroyed){
    return false;
  }
  bool destroyed = false;
  for (int i = 0; i<3;i++){
    if (!(ship->square_hit[i])){
      return destroyed;
    }
  }
  destroyed = true;
  ship->destroyed = true;
  return destroyed;
}

void attack_ship_square(int x, int y){
  Vec2 attack_pos = {x,y};
  for (int ship_index = 0; ship_index<3;ship_index++){
    for (int ship_square = 0; ship_square<3;ship_square++){
      Ship* ship = &(ships[ship_index]);
      if (ship->position[ship_square].x == attack_pos.x && ship->position[ship_square].y == attack_pos.y){
        ship->square_hit[ship_square] = true;
      }
    }
  }
}

void destroyed_ships(){
  int prev_boats[3] = {boats[0],boats[1],boats[2]};
  for (int i = 0; i<3;i++){
    if (check_if_ship_destroyed(&(ships[i]))){
      boats[i] = 1;
      if (boats != prev_boats && SCREEN_TYPE == "TFT"){
        TFT_render_current_grid();
      }
      send_boat_information();
    }
  }
}

String send_boat_information(){
  String message = "BOATS";
  for (int i =0;i<3;i++){
    message+= ":" + String(boats[i]);
  }
  message += String(device_identifier);
  Serial.println(message);
  client.send(message);
  return message;
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
      if (SCREEN_TYPE == "TFT"){
        TFT_draw_placement_after_cursor(ships[current_ship_placement], grid_x, grid_y);
      }
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
  if (SCREEN_TYPE == "TFT"){
    TFT_draw_placement_after_cursor(ships[current_ship_placement], grid_x, grid_y);
  }
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
  if (SCREEN_TYPE == "TFT"){
    TFT_render_grid_position(grid_x,grid_y,your_turn?enemy_grid:battleship_grid);
  }
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
}

void parse_message(String message){
  Serial.println(message);
  if (message.startsWith("ATTACKRESULT")){
    //12 15 17
    enemy_grid[String(message[13]).toInt()][String(message[15]).toInt()] = String(message[17]).toInt();
    if (SCREEN_TYPE == "TFT"){
      TFT_render_grid_position(String(message[13]).toInt(),String(message[15]).toInt(),your_turn?enemy_grid:battleship_grid);
    }
  }
  else if(message.startsWith("TURN")){
    if (message.endsWith("True")){
      your_turn = true;
    }
    else if (message.endsWith("False")){
      your_turn = false;
    }
    if (SCREEN_TYPE == "I2C"){
      display_message(your_turn?"Your Turn":"Enemy Turn");
    }
    else if (SCREEN_TYPE == "TFT"){
      TFT_render_current_grid();
    }
    Placement_to_Active_transition(true);
  }
  else if (message.startsWith("GAMESTART")){
    // if (current_state == MenuState){
    //   current_state = PlacementState;
    // }
    if (SCREEN_TYPE == "I2C"){
      display_message("Starting Game");
    }
    else if (SCREEN_TYPE == "TFT"){
      TFT_display_message("Starting Game");
    }
    Menu_to_Placement_transition(true);
  }
  else if (message.startsWith("DEFENCE")){
    //DEFENCE:3:3:3
    battleship_grid[String(message[8]).toInt()][String(message[10]).toInt()] = String(message[12]).toInt();
    attack_ship_square(String(message[8]).toInt(), String(message[10]).toInt());
    destroyed_ships();
    for (int i = 0;i<3;i++){
      Serial.print(boats[i]);
    }
    Serial.print("\n");
    if (SCREEN_TYPE == "TFT"){
      TFT_render_grid_position(String(message[8]).toInt(),String(message[10]).toInt(),your_turn?enemy_grid:battleship_grid);
    }
  }
  else if (message.startsWith("GAMEEND")){
    if (message.endsWith(device_identifier)){
      victory = false;
    }
    else{
      victory = true;
    }
    Active_to_Postgame_transition(true);
  }
  else if(message.startsWith("STATE")){
    recive_state_after_reconnect(message);
  }
  else if(message.startsWith("ENEMYBOATS")){
    //ENEMYBOATS :  1  :  0  :  1
    //0123456789 10 11 12 13 14 15
    int start = 11;
    for (int i = 0;i<3;i++){
      enemy_boats[i] = String(message[start+2*i]).toInt();
    }
    Serial.print("Enemy boats");
    for (int i = 0;i<3;i++){
      Serial.print(enemy_boats[i]);
    }
    Serial.print("\n");
    if (SCREEN_TYPE == "TFT"){
      TFT_render_current_grid();
    }
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
  GameState new_state = (GameState)String(state[8]).toInt();
  
  if (new_state < 2){
    switch (new_state){
      case 0:
        Menu_initialise();
        break;
      case 1:
        Menu_to_Placement_transition(false);
        break;
    }
    return;
  }
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
  switch (new_state){
      case 2:
        Placement_to_Active_transition(false);
        break;
      case 3:
        Active_to_Postgame_transition(false);
        break;
    }

}


/////////////////////////////////////////////////////////////////////
//________________________________________________________________//
//________________________________________________________________//
//___________________TFT SCREEN FUNCTIONS_________________________//
//________________________________________________________________//
//________________________________________________________________//
////////////////////////////////////////////////////////////////////


void TFT_draw_grid_cover(){
  tft.fillRect(startX, startY, gridWidth, gridWidth, ILI9341_BLACK);
}

void TFT_draw_grid(){
  //Draws the grid.
  for (int i = 0; i <= gridSize; i++) {
      int pos = startX + i * squareSize;
      tft.drawFastHLine(startX, startY + i * squareSize, gridWidth, ILI9341_WHITE); 
      tft.drawFastVLine(pos, startY, gridWidth, ILI9341_WHITE);                    
  }
}

void TFT_render_grid_position(int x, int y, int grid[5][5]){
  int squareX = startX + x * squareSize;        // Top-left X of the square
  int squareY = startY + y * squareSize;
  tft.fillRect(squareX, squareY, squareSize, squareSize, ILI9341_BLACK);
  tft.drawRect(squareX, squareY, squareSize, squareSize, ILI9341_WHITE);
  switch (grid[x][y]) {
    case (1): {
      TFT_drawShade(squareX, squareY, squareSize, squareSize); // Shade the square
      break; }
    case (2): {
      TFT_drawCross(squareX, squareY, squareSize, squareSize); // Draw an X in the square
      break; }
    case (3): {
      int circleX = startX + x * squareSize + squareSize / 2; // X-center of the square
      int circleY = startY + y * squareSize + squareSize / 2; // Y-center of the square
      int radius = 15;
      tft.drawCircle(circleX, circleY, radius, ILI9341_BLUE); // Draw circle in the center // Circle miss
      break; }
    }
}

void TFT_draw_grid_details(int grid[5][5]) { 
  int radius = 15; // Circle radius
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      switch (grid[i][j]) {
        case (1): {
          int squareX = startX + i * squareSize;        // Top-left X of the square
          int squareY = startY + j * squareSize;        // Top-left Y of the square
          TFT_drawShade(squareX, squareY, squareSize, squareSize); // Shade the square
          break; }
        case (2): {
          int squareX = startX + i * squareSize;        // Top-left X of the square
          int squareY = startY + j * squareSize;        // Top-left Y of the square
          TFT_drawCross(squareX, squareY, squareSize, squareSize); // Draw an X in the square
          break; }
        case (3): {
          int circleX = startX + i * squareSize + squareSize / 2; // X-center of the square
          int circleY = startY + j * squareSize + squareSize / 2; // Y-center of the square
          tft.drawCircle(circleX, circleY, radius, ILI9341_BLUE); // Draw circle in the center // Circle miss
          break; }
      }
    }
  }
}


void TFT_render_current_grid(){
  tft.fillScreen(ILI9341_BLACK);
  TFT_draw_details(your_turn);
  TFT_draw_grid();
  TFT_draw_legend();
  if (your_turn){
    TFT_draw_grid_details(enemy_grid);
    TFT_draw_boats(enemy_boats);
  }
  else{
    TFT_draw_grid_details(battleship_grid);
    TFT_draw_boats(boats);
  }
}

void TFT_drawShade(int16_t start_x, int16_t start_y, int16_t width, int16_t height) {
    int16_t spacing = 5; 
    // Draw diagonal lines over grid 
    for (int16_t x = 0; x <= width; x += spacing) {
        tft.drawLine(start_x + x, start_y, start_x, start_y + x, ILI9341_YELLOW);
    }
    for (int16_t y = spacing; y <= height; y += spacing) {
        tft.drawLine(start_x + width, start_y + y, start_x + y, start_y + height, ILI9341_YELLOW);
    }
}


void TFT_drawCross(int16_t start_x, int16_t start_y, int16_t width, int16_t height) {
    tft.drawLine(start_x, start_y, start_x + width, start_y + height, ILI9341_RED); 
    tft.drawLine(start_x, start_y + height, start_x + width, start_y, ILI9341_RED); 
}

void TFT_draw_placement_cursor(Ship ship, int x, int y){
  if (ship.orientation == Vertical){
    for (int i = y;i<=(y + ship.size - 1);i++){
      int squareX = startX + x * squareSize;        // Top-left X of the square
      int squareY = startY + i * squareSize;        // Top-left Y of the square
      tft.fillRect(squareX, squareY, squareSize, squareSize, ILI9341_WHITE);
    }
  }
  if (ship.orientation == Horizontal){
    for (int i = x;i<=(x + ship.size - 1);i++){
      //u8g2.drawBox(12+20*i, 12+20*y, 20, 20);
      int squareX = startX + i * squareSize;        // Top-left X of the square
      int squareY = startY + y * squareSize;        // Top-left Y of the square
      tft.fillRect(squareX, squareY, squareSize, squareSize, ILI9341_WHITE);
    }
  }
}

void TFT_draw_placement_after_cursor(Ship ship, int x, int y){
  if (ship.orientation == Vertical){
    for (int i = y;i<=(y + ship.size - 1);i++){
      TFT_render_grid_position(x, i, battleship_grid);
    }
  }
  if (ship.orientation == Horizontal){
    for (int i = x;i<=(x + ship.size - 1);i++){
      TFT_render_grid_position(i, y, battleship_grid);
    }
  }
}


void TFT_draw_details(bool turn){
  switch (turn){
    case (false): {
      tft.setCursor(50, (screenWidth - gridWidth) / 2 - 25);
      tft.setTextColor(ILI9341_RED);
      tft.setTextSize(2); 
      tft.println("Enemy Turn");

      tft.setCursor(50, (screenWidth - gridWidth) / 2 - 8);
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(2); 
      tft.println("Your Fleet");

      tft.setCursor(35, (screenWidth - gridWidth) / 2 + 195);
      tft.setTextColor(ILI9341_RED);
      tft.setTextSize(1); 
      tft.println("Waiting for enemy action..."); 

      tft.setCursor(screenWidth - 40, (screenWidth - gridWidth) / 2 - 8);
      tft.setTextColor(ILI9341_GREEN);
      tft.setTextSize(2); 
      tft.println("Your Ships"); 
      break; }
    case (true): {
      tft.setCursor(50, (screenWidth - gridWidth) / 2 - 25);
      tft.setTextColor(ILI9341_GREEN);
      tft.setTextSize(2); 
      tft.println("Your Turn");

      tft.setCursor(45, (screenWidth - gridWidth) / 2 - 8);
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(2); 
      tft.println("Enemy Fleet");

      tft.setCursor(50, (screenWidth - gridWidth) / 2 + 195);
      tft.setTextColor(ILI9341_RED);
      tft.setTextSize(1); 
      tft.println("Choose your target!"); 

      tft.setCursor(screenWidth - 55, (screenWidth - gridWidth) / 2 - 8);
      tft.setTextColor(ILI9341_RED);
      tft.setTextSize(2); 
      tft.println("Enemy Ships"); 
      break; }
  }
}

void TFT_draw_menu_state(){
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(3); // Text size
  tft.setCursor(80 + 10, screenWidth/2 + 10);
  tft.fillRect(75,screenWidth/2, 170, 40, ILI9341_WHITE);
  tft.print("Connect");
}

void TFT_draw_postgame_state(){
  TFT_win_loss(victory);
}

void TFT_win_loss(bool win) {
  if (win) {
    // Fill the screen with green color
    tft.fillScreen(ILI9341_GREEN);

    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(3); // Text size
    tft.setCursor(80, screenWidth/2);
    tft.print("You Win!!");
  } else{
    // Fill the screen with green color
    tft.fillScreen(ILI9341_RED);

    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(3); // Text size
    tft.setCursor(80, screenWidth/2);
    tft.print("You Lose!!");
  }
}

void TFT_display_message(String message){
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(25, screenWidth/2);
  tft.setTextSize(3);
  tft.println(message);
  delay(650);
}

void TFT_draw_cursor(int x, int y) {
  int centerX = startX + x * squareSize + squareSize / 2; // Center X of the grid box
  int centerY = startY + y * squareSize + squareSize / 2; // Center Y of the grid box

  int circleRadius = 12; 
  int lineLength = 15;   
  int repeat = 3;     

  for (int i = 0; i < repeat; i++) {
    tft.drawCircle(centerX, centerY, circleRadius + i, ILI9341_WHITE);
  }

  for (int i = -repeat / 2; i <= repeat / 2; i++) {
    tft.drawFastVLine(centerX + i, centerY - lineLength, 2 * lineLength, ILI9341_WHITE);
  }

  for (int i = -repeat / 2; i <= repeat / 2; i++) {
    tft.drawFastHLine(centerX - lineLength, centerY + i, 2 * lineLength, ILI9341_WHITE);
  }
}

void TFT_draw_legend() {
  // Draw the legend box
  int legendX = screenWidth - 40; // Hardcoded for the bottom-right corner
  int legendY = 183; 
  int boxWidth = 120;
  int boxHeight = 40;

  // Draw the outline
  tft.drawRect(legendX, legendY, boxWidth, boxHeight, ILI9341_WHITE);
  
  // Legend item dimensions
  int iconX = legendX + 5;   // Starting X position for icons
  int iconY = legendY + 5;   // Starting Y position for icons
  int iconSpacingX = 55;     // Horizontal spacing between items
  int iconSpacingY = 20;     // Vertical spacing between rows

  // Row 1: Shaded Square and Cross (SHIP & HIT)
  // Case 1: Shaded Square (SHIP)
  TFT_drawShade(iconX, iconY, 10, 10); // Draw a small shaded square
  tft.setCursor(iconX + 15, iconY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.print("Ship");

  // Cross (HIT)
  int iconX2 = iconX + iconSpacingX;
  TFT_drawCross(iconX2, iconY, 10, 10); // Draw a small cross
  tft.setCursor(iconX2 + 15, iconY);
  tft.print("Hit");

  // Row 2: Circle and Crosshair (MISS & CURSOR)
  iconY += iconSpacingY;
  // Circle (MISS)
  tft.drawCircle(iconX + 5, iconY + 5, 5, ILI9341_BLUE); 
  tft.setCursor(iconX + 15, iconY);
  tft.print("Miss");

  // Crosshair (CURSOR)
  int iconX3 = iconX + iconSpacingX;
  int iconY3 = iconY; 
  int centerX = iconX3 + 5;  
  int centerY = iconY3 + 5;  
  int circleRadius = 5; 
  int lineLength = 7;      

  tft.drawCircle(centerX, centerY, circleRadius, ILI9341_WHITE); 

  tft.drawFastVLine(centerX, centerY - lineLength, 2 * lineLength, ILI9341_WHITE);
  tft.drawFastHLine(centerX - lineLength, centerY, 2 * lineLength, ILI9341_WHITE);

  tft.setCursor(iconX3 + 15, iconY);
  tft.print("Cursor");
}

void TFT_draw_boats(int boatState[3]){
  int boatX = screenWidth - 30;
  int s_boatX = screenWidth - 30 + (BOAT_MEDIUM_WIDTH - BOAT_SMALL_WIDTH) / 2; //Center small boat
  int boatY = 45;
  //Draw the small boat
  tft.drawBitmap(s_boatX, boatY, small_boat, BOAT_SMALL_WIDTH, BOAT_HEIGHT, ILI9341_WHITE);
  //Draw lines under small boat to display size 
  tft.drawFastHLine(s_boatX, boatY+6 + BOAT_HEIGHT, BOAT_SMALL_WIDTH, ILI9341_WHITE);
  tft.drawFastVLine(s_boatX, boatY+1 + BOAT_HEIGHT, 10, ILI9341_WHITE);
  tft.drawFastVLine(s_boatX+35, boatY+1 + BOAT_HEIGHT, 10, ILI9341_WHITE);
  tft.drawFastVLine(s_boatX+35+35, boatY+1 + BOAT_HEIGHT, 10, ILI9341_WHITE);
  //If small boat destroyed -> draw cross over it
  if(boatState[0] == 1){
    int x = s_boatX+35/2;
    TFT_drawCross(x, boatY, BOAT_HEIGHT, BOAT_HEIGHT);
    TFT_drawCross(x-1, boatY, BOAT_HEIGHT, BOAT_HEIGHT);
    TFT_drawCross(x+1, boatY, BOAT_HEIGHT, BOAT_HEIGHT);
  }

  boatY += 45; //Increment space between boats
  tft.drawBitmap(boatX, boatY, medium_boat, BOAT_MEDIUM_WIDTH, BOAT_HEIGHT, ILI9341_WHITE);
  //Draw lines under medium boat to display size 
  tft.drawFastHLine(boatX, boatY+6 + BOAT_HEIGHT, BOAT_MEDIUM_WIDTH, ILI9341_WHITE);
  tft.drawFastVLine(boatX, boatY+1 + BOAT_HEIGHT, 10, ILI9341_WHITE);
  tft.drawFastVLine(boatX+35, boatY+1 + BOAT_HEIGHT, 10, ILI9341_WHITE);
  tft.drawFastVLine(boatX+35+35, boatY+1 + BOAT_HEIGHT, 10, ILI9341_WHITE);
  tft.drawFastVLine(boatX+35+35+35, boatY+1 + BOAT_HEIGHT, 10, ILI9341_WHITE);
  //If medium boat destroyed -> draw cross over it
  if(boatState[1] == 1){
    int x = boatX+70/2;
    TFT_drawCross(x, boatY, BOAT_HEIGHT, BOAT_HEIGHT);
    TFT_drawCross(x-1, boatY, BOAT_HEIGHT, BOAT_HEIGHT);
    TFT_drawCross(x+1, boatY, BOAT_HEIGHT, BOAT_HEIGHT);
  }

  boatY += 45;
  tft.drawBitmap(boatX, boatY, medium_boat, BOAT_MEDIUM_WIDTH, BOAT_HEIGHT, ILI9341_WHITE);
  //Draw lines under medium boat 2 to display size 
  tft.drawFastHLine(boatX, boatY+6 + BOAT_HEIGHT, BOAT_MEDIUM_WIDTH, ILI9341_WHITE);
  tft.drawFastVLine(boatX, boatY+1 + BOAT_HEIGHT, 10, ILI9341_WHITE);
  tft.drawFastVLine(boatX+35, boatY+1 + BOAT_HEIGHT, 10, ILI9341_WHITE);
  tft.drawFastVLine(boatX+35+35, boatY+1 + BOAT_HEIGHT, 10, ILI9341_WHITE);
  tft.drawFastVLine(boatX+35+35+35, boatY+1 + BOAT_HEIGHT, 10, ILI9341_WHITE);
  //If the second medium boat destroyed -> draw cross over it
  if(boatState[2] == 1){
    int x = boatX+70/2;
    TFT_drawCross(x, boatY, BOAT_HEIGHT, BOAT_HEIGHT);
    TFT_drawCross(x-1, boatY, BOAT_HEIGHT, BOAT_HEIGHT);
    TFT_drawCross(x+1, boatY, BOAT_HEIGHT, BOAT_HEIGHT);
  }
}


