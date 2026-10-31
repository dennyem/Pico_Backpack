#ifndef LCD_H  //LCD_H
#define LCD_H  //LCD_H

//Use C SDK for backlight PWM to avoid messing with audio PWM
#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/irq.h"
#include "hardware/pwm.h"

#include "Arial_round_16x24.c"
#include "arial_normal.c"

const float pi = 3.14159267;
#define clkCentre_x 250
#define clkCentre_y 150
#define innerRadius 80
#define outerRadius 100

// struct Alarm{
//   int hour;
//   int min;
//   int sec;
//   int event;
//   bool silent;
// };

// Alarm alarm1;

// for PICO
#define LCDCS (12)
#define LCDRST (13)
#define LCDDC (14)
#define TOUCHCS (15)
#define MISOPIN (16)
#define TOUCHIRQ (17)
#define SCKPIN (18)
#define MOSIPIN (19)
#define BLPIN (20)
#define SDCS (21)

#include <SPI.h>

//colours are 24bit/18bit
#define BLACK 0x000000l
#define BLUE 0x0000FFl
#define RED 0xFF0000l
#define GREEN 0x00FF00l
#define CYAN 0x00FFFFl
#define MAGENTA 0xFF00FFl
#define YELLOW 0xFFFF00l
#define WHITE 0xFFFFFFl
#define GREY 0x808080l

//#define BUTTON_FONT Arial_round_16x24
#define BUTTON_FONT arial_normal
#define BUTTON_IDLE_TEXT WHITE
#define BUTTON_IDLE_BACK BLACK
//#define BUTTON_IDLE_BORDER GREY
#define BUTTON_IDLE_BORDER BLACK
#define BUTTON_PRESS_TEXT BLACK
#define BUTTON_PRESS_BACK WHITE
#define BUTTON_PRESS_BORDER BLACK
//#define BUTTON_PRESS_BORDER GREY
#define BUTTON_BACKGROUND BLACK
#define BUTTON_NO_CHANGE (0)
#define BUTTON_DOWN (1)
#define BUTTON_UP (2)

#define LCD_SPEED (50000000L)
#define TOUCH_SPEED (2000000L)
#define Z_TOUCH_THRESHOLD 20
#define TOUCH_X0 110
#define TOUCH_X1 2001
#define TOUCH_Y0 1993
#define TOUCH_Y1 76
#define TOUCH_OVERSAMPLE 16

//backlight slice/channel info
extern unsigned int sliceBL, chanBL;

//global status/info
extern int width, height, rotation;

//button object
struct button {
  int x;
  int y;
  int w;
  int h;
  const char* text;
  char pressed;
  char visible;
  char lastchange = 0;
};

//slider object
struct slider {
  int x;
  int y;
  int w;
  int h;
  int value;
  char pressed;
  char visible;
  char lastchange = 0;
};

void setBacklight(byte b);

void LCDPrint(int x, int y, const char* c, unsigned long fontColor, unsigned long backColor);
void LCDsmallPrint(int x, int y, const char* c, unsigned long fontColor, unsigned long backColor);
void displaySetup();
void setrotation(byte r);
void setarea(int x0, int y0, int x1, int y1);
void clear(unsigned long c);
void clear(int x0, int y0, int w, int h, unsigned long c);
void point(int x, int y, unsigned long c);
void data8(byte d);
void cmd8(byte d);
int touchraw(unsigned char r);
int touchx();
int touchy();
int showchar(int x0, int y0, char c, const unsigned char* font, unsigned long f, unsigned long b);
int showarray(int x0, int y0, const char* c, const unsigned char* font, unsigned long f, unsigned long b);
int fontwidth(const unsigned char* font);
int fontheight(const unsigned char* font);
int arraywidth(const char* c, const unsigned char* font);
void box(int x0, int y0, int x1, int y1, unsigned long c);
void line(int x1, int y1, int x2, int y2, unsigned long c);
void hline(int x1, int y1, int x2, unsigned long c);
void vline(int x1, int y1, int y2, unsigned long c);
void fcircle(int xo, int yo, int r, unsigned long c);
void circle(int xo, int yo, int r, unsigned long c);
void textbox(int x0, int y0, int w, int h, const char* c, const unsigned char* font, unsigned long tc, unsigned long bc, unsigned long bordc);
int checkpress(button* b);
void drawbutton(button* b);
int slidercheckpress(slider* b);
void drawslider(slider* b);
void SPIforLCD();  //reload LCD SPI settings for use after SD access

#endif  //LCD_H