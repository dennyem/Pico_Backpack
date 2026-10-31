#ifndef AUDIO_C  //AUDIO_C
#define AUDIO_C  //AUDIO_C

#include "audio.h"

#define PWMAL (9)
#define PWMAR (8)

unsigned int sliceAL, sliceAR, chanAL, chanAR;
unsigned int freqdiv = 11;  //11=>44100ish
volatile const char* nowData;
volatile const char* nextData;
volatile unsigned int nowSize, nextSize, nowPtr;
unsigned int audioState = 0;

unsigned int audioSetRate(unsigned int s) {  //returns actual rate
  unsigned int pwmf = clock_get_hz(clk_sys) / 256;
  if (s > 0) {
    freqdiv = (pwmf + (s / 2)) / s;  //+s/2 gets us nearest rounding
  }
  if (freqdiv < 1) { freqdiv = 1; }
  return pwmf / freqdiv;
}

unsigned int audioBytes() {  //number of bytes left to play
  unsigned int r = 0;
  if (nextData && nextSize) {  //if data in second
    r = nextSize;
  }
  if (nowData && nowSize) {  //continue if valid
    return nowSize - nowPtr + r;
  }
  return 0;  //nothing to play if current buffer is empty
}

unsigned int audioSpace() {    //is there space to Queue?
  if (nextData && nextSize) {  //if data in second, no room
    return 0;
  } else {
    return 1;
  }
}

unsigned int audioQueue(const char* b, unsigned int s) {
  if (audioState == AUDIO_STOPPED) {
    if (nowData && nowSize) {      //if data in 1st
      if (nextData && nextSize) {  //if data in second, no room
        return 0;
      } else {
        nextData = b;
        nextSize = s;
      }
    } else {
      nowData = b;
      nowSize = s;
    }
  } else {
    if (nextData && nextSize) {  //if data in second, no room
      return 0;
    } else {
      nextData = b;
      nextSize = s;
    }
  }
  return s;  //s bytes queued
}

unsigned int audioPlay(unsigned int s) {  //takes 1/2 mono/stereo, will resume current state if paused
  if (audioState == AUDIO_STOPPED) {
    if (nowData && nowSize) {  //continue if valid
      if (s == AUDIO_PLAY_MONO) { audioState = AUDIO_PLAY_MONO; }
      if (s == AUDIO_PLAY_STEREO) { audioState = AUDIO_PLAY_STEREO; }
    }
  } else {
    if (audioState == AUDIO_PAUSE_MONO) { audioState = AUDIO_PLAY_MONO; }
    if (audioState == AUDIO_PAUSE_STEREO) { audioState = AUDIO_PLAY_STEREO; }
  }
  return audioState;  //return resulting state
}

void audioStop() {  //stop and put everything in an empty state
  audioState = AUDIO_STOPPED;
  nowData = 0;
  nowSize = 0;
  nowPtr = 0;
  nextData = 0;
  nextSize = 0;
}

unsigned int audioPause() {
  if (audioState == AUDIO_PLAY_MONO) { audioState = AUDIO_PAUSE_MONO; }
  if (audioState == AUDIO_PLAY_STEREO) { audioState = AUDIO_PAUSE_STEREO; }
  return audioState;  //return resulting state
}

void audioInit() {
  //set null
  nowData = 0;
  nextData = 0;
  audioState = AUDIO_STOPPED;
  freqdiv = 11;  //11=>44100ish
                 //use C SDK to get access to interrupts
  gpio_set_function(PWMAL, GPIO_FUNC_PWM);
  gpio_set_function(PWMAR, GPIO_FUNC_PWM);
  //hardware could have the same PWM slice, but we won't assume it
  sliceAL = pwm_gpio_to_slice_num(PWMAL);
  sliceAR = pwm_gpio_to_slice_num(PWMAR);
  chanAL = pwm_gpio_to_channel(PWMAL);
  chanAR = pwm_gpio_to_channel(PWMAR);
  //use these globals to init irq using left PWM slice
  pwm_clear_irq(sliceAL);
  pwm_set_irq_enabled(sliceAL, true);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, audioPWMINT);
  irq_set_enabled(PWM_IRQ_WRAP, true);
  //set up PWM output
  pwm_set_wrap(sliceAL, 255);
  pwm_set_wrap(sliceAR, 255);
  pwm_set_clkdiv_int_frac(sliceAL, 1, 0);  //use highest available freq and divide down in IRQ
  pwm_set_clkdiv_int_frac(sliceAR, 1, 0);
  pwm_set_chan_level(sliceAL, chanAL, 0);  //off
  pwm_set_chan_level(sliceAR, chanAR, 0);  //off
  pwm_set_enabled(sliceAL, true);          //PWM running
  pwm_set_enabled(sliceAR, true);          //PWM running
}

void audioPWMINT() {
  static char level = 0;
  static unsigned int fcount = 0;
  static const char wave[] = {
    0, 0, 0, 0, 0
    //0,10,20,30,40,50,60,70,80,90,100,110,120,130,140,150,160,170,180,190,200,210,220,230,240,250,
    //240,230,220,210,200,190,180,170,160,150,140,130,120,110,100,90,80,70,60,50,40,30,20,10
  };
  pwm_clear_irq(sliceAL);
  fcount++;
  if (fcount >= freqdiv) {
    fcount = 0;
    level++;
    if (level >= sizeof(wave)) { level = 0; }
    if (nowData) {  //valid data
      if (audioState == AUDIO_PLAY_MONO) {
        pwm_set_chan_level(sliceAL, chanAL, nowData[nowPtr]);
        pwm_set_chan_level(sliceAR, chanAR, nowData[nowPtr++]);
      } else if (audioState == AUDIO_PLAY_STEREO) {
        pwm_set_chan_level(sliceAL, chanAL, nowData[nowPtr++]);
        pwm_set_chan_level(sliceAR, chanAR, nowData[nowPtr++]);
      }
      if (nowPtr >= nowSize) {
        if (nextData && nextSize) {  //continue if valid
          nowData = nextData;
          nowSize = nextSize;
          nowPtr = 0;
          nextData = 0;
          nextSize = 0;
        } else {
          nowData = 0;
          nowSize = 0;
          nowPtr = 0;
          audioState = AUDIO_STOPPED;
        }
      }
    }
  }
}

#endif  //AUDIO_C