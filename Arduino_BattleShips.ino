#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

int joy_x = A1;
int joy_y = A0;

int grid_x = 0;
int grid_y = 0;

bool button_pressed =false;
typedef enum{
  Up = 0,
  Down = 1,
  Right = 2,
  Left = 3,
  None = 4,
}JoyPos;

int battleship_grid[5][5];

unsigned long previouse_millis;
// display connected to the HW I2C (this is the best option)
U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

void setup(void) {
  u8g2.begin();
  intializeShips();
  previouse_millis = millis();
}

void loop(void) {

  u8g2.clearBuffer();
  readJoy();
  draw_grid();
  draw_cursor(grid_x,grid_y);
  draw_zeros();
  u8g2.sendBuffer();
}

void intializeShips(){
  battleship_grid[0][0] = 1;
  battleship_grid[0][1] = 1;
  battleship_grid[2][0] = 1;
  battleship_grid[3][0] = 1;
  battleship_grid[4][1] = 1;
  battleship_grid[4][2] = 1;
  battleship_grid[4][3] = 1;
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

void draw_zeros(){
  int start_x = 12;
  int start_y = 12;
  int radius = 10;
  int offset = 20;
  for (int i  = 0; i < 5;i++){
    for (int j = 0;j < 5;j++){
      switch (battleship_grid[i][j]){
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
    if (!button_pressed){
      button_pressed = true;
      if (battleship_grid[grid_x][grid_y] == 1 || battleship_grid[grid_x][grid_y] == 2){
        battleship_grid[grid_x][grid_y] = 2; 
      }
      else{
        battleship_grid[grid_x][grid_y] = 3;
      }
    }
    return;
  }
  button_pressed = false;
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