#include "LCD.h"

//backlight slice/channel info
unsigned int sliceBL, chanBL;

//global status/info
int width, height, rotation;

void setBacklight(byte b) {
  pwm_set_chan_level(sliceBL, chanBL, b);
}

void displaySetup() {
  sliceBL = pwm_gpio_to_slice_num(BLPIN);
  chanBL = pwm_gpio_to_channel(BLPIN);

  //  pwm_config conf = pwm_get_default_config();
  //  pwm_init(sliceBL, &conf, false);

  gpio_set_function(BLPIN, GPIO_FUNC_PWM);
  gpio_set_pulls(BLPIN, false, true);
  pwm_set_wrap(sliceBL, 100);               // = >maps to %
  pwm_set_clkdiv_int_frac(sliceBL, 50, 0);  //25kHz
  pwm_set_chan_level(sliceBL, chanBL, 0);   //0% duty
  pwm_set_enabled(sliceBL, true);           //PWM running

  width = 320;
  height = 480;
  rotation = 0;  //default
  SPI.setRX(MISOPIN);
  SPI.setSCK(SCKPIN);
  SPI.setTX(MOSIPIN);
  SPI.begin();
  SPI.beginTransaction(SPISettings(LCD_SPEED, MSBFIRST, SPI_MODE0));
  pinMode(LCDRST, OUTPUT);
  pinMode(LCDCS, OUTPUT);
  pinMode(LCDDC, OUTPUT);
  pinMode(TOUCHCS, OUTPUT);
  digitalWrite(TOUCHCS, HIGH);
  digitalWrite(LCDRST, HIGH);
  digitalWrite(LCDCS, HIGH);
  digitalWrite(LCDDC, HIGH);
  delay(5);
  digitalWrite(LCDRST, LOW);
  delay(15);
  digitalWrite(LCDRST, HIGH);  //hard reset
  delay(15);
  digitalWrite(LCDCS, LOW);
  cmd8(0x1);  //sw reset
  delay(120);
  cmd8(0x11);  //Sleep out
  delay(120);
  cmd8(0x13);  //normal
  cmd8(0x20);  //no inversion
  cmd8(0x28);  //display off
  cmd8(0x38);  //idle mode off
  cmd8(0xC0);  //power control 1
  data8(0x17);
  data8(0x15);
  cmd8(0xC1);  //power control 2
  data8(0x41);
  cmd8(0xC5);  //VCOM control
  data8(0x0e);
  data8(0x0e);
  cmd8(0x36);  //Memory Access control
  data8(88);   //suits rotation 0
  cmd8(0x3A);  //Pixel interface format
  data8(0x66);
  cmd8(0xB4);  //inversion control
  data8(0x2);
  cmd8(0xB6);  //Function control
  data8(0x2);
  data8(0x2);
  data8(0x3B);
  cmd8(0x29);  //display on
  cmd8(0x2A);  //set column
  data8(0x0);
  data8(0x0);
  data8(0x1);
  data8(0x3F);
  cmd8(0x2B);  //set row
  data8(0x0);
  data8(0x0);
  data8(0x1);
  data8(0xDF);
  cmd8(0x2C);  //draw
  digitalWrite(LCDCS, HIGH);
}

void LCDPrint(int x, int y, const char* c, unsigned long fontColor, unsigned long backColor) {
  SPIforLCD();  //reload LCD SPI settings for use after SD access
  showarray(x, y, c, Arial_round_16x24, fontColor, backColor);
}

void LCDsmallPrint(int x, int y, const char* c, unsigned long fontColor, unsigned long backColor) {
  SPIforLCD();  //reload LCD SPI settings for use after SD access
  showarray(x, y, c, arial_normal, fontColor, backColor);
}

void setrotation(byte r) {
  digitalWrite(LCDCS, LOW);
  switch (r) {
    case 0:
      rotation = 0;  //portrait 1
      width = 320;
      height = 480;
      cmd8(0x36);  //Memory access control
      data8(88);   //sets orientation/scan direction/flip
      setarea(0, 0, width - 1, height - 1);
      break;
    case 1:
      rotation = 1;  //landscape 1
      width = 480;
      height = 320;
      cmd8(0x36);  //Memory access control
      data8(56);   //sets orientation/scan direction/flip
      setarea(0, 0, width - 1, height - 1);
      break;
    case 2:
      rotation = 2;  //portrait reversed
      width = 320;
      height = 480;
      cmd8(0x36);  //Memory access control
      data8(152);  //sets orientation/scan direction/flip
      setarea(0, 0, width - 1, height - 1);
      break;
    case 3:
      rotation = 3;  //landscape reversed
      width = 480;
      height = 320;
      cmd8(0x36);  //Memory access control
      data8(248);  //sets orientation/scan direction/flip
      setarea(0, 0, width - 1, height - 1);
      break;
  }
  digitalWrite(LCDCS, HIGH);
}

void setarea(int x0, int y0, int x1, int y1) {
  int t;
  if (x1 < x0) {
    int t = x0;
    x0 = x1;
    x1 = t;
  }  //sort x
  if (y1 < y0) {
    int t = y0;
    y0 = y1;
    y1 = t;
  }  //sort y
  digitalWrite(LCDCS, LOW);
  cmd8(0x2A);  //set column start/end
  data8(x0 >> 8);
  data8(x0);  //start
  data8(x1 >> 8);
  data8(x1);   //end
  cmd8(0x2B);  //set row start/end
  data8(y0 >> 8);
  data8(y0);  //start
  data8(y1 >> 8);
  data8(y1);   //end
  cmd8(0x2C);  //draw!
  digitalWrite(LCDCS, HIGH);
}

void clear(unsigned long c) {
  int x, y;

  setarea(0, 0, width - 1, height - 1);
  digitalWrite(LCDCS, LOW);
  for (x = 0; x < 320; x++) {
    for (y = 0; y < 480; y++) {
      data8(c >> 16);
      data8(c >> 8);
      data8(c);
    }
  }
  digitalWrite(LCDCS, HIGH);
}

void clear(int x0, int y0, int w, int h, unsigned long c) {
  int x, y;

  setarea(x0, y0, (x0 + w) - 1, (y0 + h) - 1);
  digitalWrite(LCDCS, LOW);

  for (x = x0; x < (x0 + w); x++) {
    for (y = y0; y < (y0 + h); y++) {
      data8(c >> 16);
      data8(c >> 8);
      data8(c);
    }
  }
  digitalWrite(LCDCS, HIGH);
  setarea(0, 0, width - 1, height - 1);
}

void point(int x, int y, unsigned long c) {
  setarea(x, y, x, y);
  digitalWrite(LCDCS, LOW);
  data8(c >> 16);
  data8(c >> 8);
  data8(c);
  digitalWrite(LCDCS, HIGH);
}

void data8(byte d) {
  SPI.transfer(d);
}

void cmd8(byte d) {
  digitalWrite(LCDDC, LOW);
  data8(d);
  digitalWrite(LCDDC, HIGH);
}

int touchraw(unsigned char r) {
  long n = 0;
  int i = 0;
  SPI.beginTransaction(SPISettings(TOUCH_SPEED, MSBFIRST, SPI_MODE0));
  digitalWrite(TOUCHCS, LOW);
  SPI.transfer(r);  //init and ignore first
  for (i = 0; i < TOUCH_OVERSAMPLE; i++) { n = n + SPI.transfer16(r); }
  digitalWrite(TOUCHCS, HIGH);
  SPI.beginTransaction(SPISettings(LCD_SPEED, MSBFIRST, SPI_MODE0));
  return (n / TOUCH_OVERSAMPLE) >> 4;  //only 12 bits resolution
}

int touchx() {
  int n, p, z;
  z = touchraw(0xB1);                        //use n = 0x91 for x, n = 0xD1 for y, 0xB1 for z
  if (z < Z_TOUCH_THRESHOLD) { return -1; }  //no touch
  switch (rotation) {
    case 0:
      n = touchraw(0xD1);  //use n = 0x91 for x, n = 0xD1 for y, 0xB1 for z
      p = map(n, TOUCH_Y1, TOUCH_Y0, 0, width - 1);
      if (p < 0) { return -1; }
      if (p > width - 1) { return -1; }
      return p;
      break;
    case 1:
      n = touchraw(0x91);  //use n = 0x91 for x, n = 0xD1 for y, 0xB1 for z
      p = map(n, TOUCH_X1, TOUCH_X0, 0, width - 1);
      if (p < 0) { return -1; }
      if (p > width - 1) { return -1; }
      return p;
      break;
    case 2:
      n = touchraw(0xD1);
      p = map(n, TOUCH_Y0, TOUCH_Y1, 0, width - 1);
      if (p < 0) { return -1; }
      if (p > width - 1) { return -1; }
      return p;
      break;
    case 3:
      n = touchraw(0x91);
      p = map(n, TOUCH_X0, TOUCH_X1, 0, width - 1);
      if (p < 0) { return -1; }
      if (p > width - 1) { return -1; }
      return p;
      break;
    default:
      return -1;
  }
}

int touchy() {
  int n, p, z;
  z = touchraw(0xB1);                        //use n = 0x91 for x, n = 0xD1 for y, 0xB1 for z
  if (z < Z_TOUCH_THRESHOLD) { return -1; }  //no touch
  switch (rotation) {
    case 0:
      n = touchraw(0x91);
      p = map(n, TOUCH_X1, TOUCH_X0, 0, height - 1);
      if (p < 0) { return -1; }
      if (p > height - 1) { return -1; }
      return p;
      break;
    case 1:
      n = touchraw(0xD1);
      p = map(n, TOUCH_Y0, TOUCH_Y1, 0, height - 1);
      if (p < 0) { return -1; }
      if (p > height - 1) { return -1; }
      return p;
      break;
    case 2:
      n = touchraw(0x91);
      p = map(n, TOUCH_X0, TOUCH_X1, 0, height - 1);
      if (p < 0) { return -1; }
      if (p > height - 1) { return -1; }
      return p;
      break;
    case 3:
      n = touchraw(0xD1);
      p = map(n, TOUCH_Y1, TOUCH_Y0, 0, height - 1);
      if (p < 0) { return -1; }
      if (p > height - 1) { return -1; }
      return p;
      break;
    default:
      return -1;
  }
}

int showchar(int x0, int y0, char c, const unsigned char* font, unsigned long f, unsigned long b) {
  unsigned int w = font[0];
  unsigned int h = font[1];
  unsigned int i = 0;
  unsigned char d;
  unsigned char bt = 128;
  const unsigned char* ff;
  unsigned int cc = c;
  if (cc < font[2]) { return 0; }  //valid in font?
  if (cc >= (font[2] + font[3])) { return 0; }
  cc = cc - font[2];      //adjust start offset in chars
  cc = (cc * w * h) / 8;  //adjust offset in bytes
  ff = font + cc + 4;     //at start of character, 4 is header offset
  setarea(x0, y0, x0 + w - 1, y0 + h - 1);
  digitalWrite(LCDCS, LOW);
  d = ff[0];
  while (i < w * h) {
    i++;
    if (d & bt) {
      SPI.transfer(f >> 16);
      SPI.transfer(f >> 8);
      SPI.transfer(f >> 0);
    } else {
      SPI.transfer(b >> 16);
      SPI.transfer(b >> 8);
      SPI.transfer(b >> 0);
    }
    bt = (bt >> 1);
    if (bt == 0) {
      bt = 128;
      ff++;
      d = ff[0];
    }
  }
  digitalWrite(LCDCS, HIGH);
  return w;
}

int showarray(int x0, int y0, const char* c, const unsigned char* font, unsigned long f, unsigned long b) {
  int x = x0;
  unsigned char w = font[0];  //use these here
  unsigned char h = font[1];
  while (*c) {
    if (x > width + w) {
      x = 0;
      y0 = y0 + h;
    }  //wrap around (will probably look ugly, but better than nothing)
    x = x + showchar(x, y0, *c++, font, f, b);
  }
  return x - x0;  //results undefined on wraparound
}

int fontwidth(const unsigned char* font) {
  return font[0];
}

int fontheight(const unsigned char* font) {
  return font[1];
}

int arraywidth(const char* c, const unsigned char* font) {
  int x = 0;
  while (*c) {
    if ((*c >= font[2]) && (*c < (font[2] + font[3]))) { x = x + fontwidth(font); }
    c++;
  }
  return x;
}

void box(int x0, int y0, int x1, int y1, unsigned long c) {   //also for horizontal, vertical lines
  unsigned long p = (abs(x0 - x1) + 1) * (abs(y0 - y1) + 1);  //pixel count
  unsigned long i;
  setarea(x0, y0, x1, y1);
  digitalWrite(LCDCS, LOW);
  for (i = 0; i < p; i++) {
    SPI.transfer(c >> 16);
    SPI.transfer(c >> 8);
    SPI.transfer(c >> 0);
  }
  digitalWrite(LCDCS, HIGH);
}

void line(int x1, int y1, int x2, int y2, unsigned long c) {  //free line (Bresenham)
  int steps, stepsx, stepsy, xinc, yinc, x, y, d;
  stepsx = abs(x1 - x2);
  stepsy = abs(y1 - y2);
  steps = max(stepsx, stepsy) + 1;   //if start and end are the same, there's still 1 point
  xinc = constrain(x2 - x1, -1, 1);  //sign
  yinc = constrain(y2 - y1, -1, 1);  //sign
  x = x1;
  y = y1;
  if (stepsx > stepsy) {
    d = stepsx / 2;
    for (int i = 0; i < steps; i++) {
      point(x, y, c);
      x = x + xinc;
      d = d + stepsy;
      if (d > stepsx) {
        d = d - stepsx;
        y = y + yinc;
      }
    }
  } else {
    d = stepsy / 2;
    for (int i = 0; i < steps; i++) {
      point(x, y, c);
      y = y + yinc;
      d = d + stepsx;
      if (d > stepsy) {
        d = d - stepsy;
        x = x + xinc;
      }
    }
  }
}

void hline(int x1, int y1, int x2, unsigned long c) {
  if (x2 < x1) {
    int i = x1;
    x1 = x2;
    x2 = i;
  }
  setarea(x1, y1, x2, y1);
  x2++;
  digitalWrite(LCDCS, LOW);
  for (int x = x1; x < x2; x++) {
    SPI.transfer(c >> 16);
    SPI.transfer(c >> 8);
    SPI.transfer(c >> 0);
  }
  digitalWrite(LCDCS, HIGH);
}

void vline(int x1, int y1, int y2, unsigned long c) {
  if (y2 < y1) {
    int i = y1;
    y1 = y2;
    y2 = i;
  }
  setarea(x1, y1, x1, y2);
  y2++;
  digitalWrite(LCDCS, LOW);
  for (int y = y1; y < y2; y++) {
    SPI.transfer(c >> 16);
    SPI.transfer(c >> 8);
    SPI.transfer(c >> 0);
  }
  digitalWrite(LCDCS, HIGH);
}

void fcircle(int xo, int yo, int r, unsigned long c) {  //https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
  int e = 0;
  int x = r;
  int y = 0;
  while (x >= y) {
    vline(xo - y, yo + x, yo - x, c);
    vline(xo + y, yo + x, yo - x, c);
    e = e + 1 + 2 * y;
    y = y + 1;
    if (2 * (e - x) + 1 > 0) {
      y = y - 1;
      vline(xo - x, yo - y, yo + y, c);
      vline(xo + x, yo - y, yo + y, c);
      y = y + 1;
      x = x - 1;
      e = e + 1 - 2 * x;
    }
  }
}

void circle(int xo, int yo, int r, unsigned long c) {  //https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
  int e = 0;
  int x = r;
  int y = 0;
  while (x >= y) {
    point(xo + x, yo + y, c);  //eight 45 degree sectors
    point(xo - x, yo + y, c);
    point(xo + x, yo - y, c);
    point(xo - x, yo - y, c);
    point(xo + y, yo + x, c);
    point(xo - y, yo + x, c);
    point(xo + y, yo - x, c);
    point(xo - y, yo - x, c);
    e = e + 1 + 2 * y;
    y = y + 1;
    if (2 * (e - x) + 1 > 0) {
      x = x - 1;
      e = e + 1 - 2 * x;
    }
  }
}

void textbox(int x0, int y0, int w, int h, const char* c, const unsigned char* font, unsigned long tc, unsigned long bc, unsigned long bordc) {
  //textbox, optimised for versatility (separate text/background/border colours) and flicker-free redraw (no pixels overdrawn)
  //no text width checking, so will appear wrong for text too long
  int tw, th;
  tw = arraywidth(c, font);
  th = fontheight(font);
  int x1, y1;
  x1 = x0 + w - 1;
  y1 = y0 + h - 1;
  int tx0, ty0, tx1, ty1;
  tx0 = x0 + (w - tw) / 2;
  tx1 = tx0 + tw - 1;
  ty0 = y0 + (h - th) / 2;
  ty1 = ty0 + th - 1;
  //border
  vline(x0, y0, y1, bordc);
  vline(x1, y0, y1, bordc);
  x0++;
  x1--;
  hline(x0, y0, x1, bordc);
  hline(x0, y1, x1, bordc);
  y0++;
  y1--;
  //this order roughly scans from top to bottom
  box(x0, y0, x1, ty0 - 1, bc);          //above text (full width)
  box(x0, ty0, tx0 - 1, ty1, bc);        //left of text (text height only)
  showarray(tx0, ty0, c, font, tc, bc);  //text
  box(tx1 + 1, ty0, x1, ty1, bc);        //right of text (text height only)
  box(x0, ty1 + 1, x1, y1, bc);          //below text (full width)
}

int checkpress(button* b) {
  int x, y;
  char r, p;
  x = touchx();
  y = touchy();
  r = BUTTON_NO_CHANGE;     //default
  p = 1;                    //assume touch
  if (x < b->x) { p = 0; }  //then rule out
  if (x > b->x + b->w) { p = 0; }
  if (y < b->y) { p = 0; }
  if (y > b->y + b->h) { p = 0; }
  if ((!b->pressed) && (p)) { r = BUTTON_DOWN; }  //detect changes
  if ((b->pressed) && (!p)) { r = BUTTON_UP; }
  b->pressed = p;
  b->lastchange = r;
  return r;  //return for quick
}

void drawbutton(button* b) {
  if (b->visible) {
    if (b->pressed) {
      textbox(b->x, b->y, b->w, b->h, b->text, BUTTON_FONT, BUTTON_PRESS_TEXT, BUTTON_PRESS_BACK, BUTTON_PRESS_BORDER);
    } else {
      textbox(b->x, b->y, b->w, b->h, b->text, BUTTON_FONT, BUTTON_IDLE_TEXT, BUTTON_IDLE_BACK, BUTTON_IDLE_BORDER);
    }
  } else {
    box(b->x, b->y, b->x + b->w - 1, b->y + b->h - 1, BUTTON_BACKGROUND);
  }
}

int slidercheckpress(slider* b) {
  int x, y;
  char r, p;
  x = touchx();
  y = touchy();
  r = BUTTON_NO_CHANGE;     //default
  p = 1;                    //assume touch
  if (x < b->x) { p = 0; }  //then rule out
  if (x > b->x + b->w) { p = 0; }
  if (y < b->y) { p = 0; }
  if (y > b->y + b->h) { p = 0; }
  if ((!b->pressed) && (p)) { r = BUTTON_DOWN; }  //detect changes
  if ((b->pressed) && (!p)) { r = BUTTON_UP; }
  b->pressed = p;
  b->lastchange = r;
  if (p) {  //update on touch
    b->value = x - b->x;
    if (b->value < 0) { b->value = 0; }
    if (b->value > b->w) { b->value = b->w; }
  }
  return r;  //return for quick
}

void drawslider(slider* b) {
  int i;
  if (b->visible) {
    for (i = 0; i < (b->w + 1); i++) {
      if (abs(b->value - i) < 3) {
        vline(b->x + i, b->y, b->y + b->h - 1, BUTTON_IDLE_TEXT);
      } else {
        vline(b->x + i, b->y, b->y + b->h - 1, BUTTON_IDLE_BORDER);
      }
    }
  } else {
    box(b->x, b->y, b->x + b->w - 1, b->y + b->h - 1, BUTTON_BACKGROUND);
  }
}

void SPIforLCD() {  //reload LCD SPI settings for use after SD access
  SPI.beginTransaction(SPISettings(LCD_SPEED, MSBFIRST, SPI_MODE0));
}