//libraries
//  LCD is in sketch folder
//  SD/SPI should be included in Arduino IDE
//  RTC uses RTClib by Adafruit (search rtclib in Library Manager) https://github.com/adafruit/RTClib
//  Font files   Arial_round_16x24 and arial_normal are in sketch folder
//  Arduino IDE 2.0.1

#include "LCD.h"
#include <SPI.h>
#include <WiFiUdp.h>
#include <AsyncWebServer_RP2040W.h>   //NOTE: install version 1.3.0. later versions are not compatible
#include "RTClib.h"
#include "audio.h"
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <RP2040_SD.h>                //NOTE: SD Cards must be formatted to FAT or FAT32. exFat or NTFS will not work

typedef struct {
  uint32_t ChunkID;
  uint32_t ChunkSize;
  uint32_t Format;
  /// fmt
  uint32_t Subchunk1ID;
  uint32_t Subchunk1Size;
  uint16_t AudioFormat;
  uint16_t NumChannels;
  uint32_t SampleRate;
  uint32_t ByteRate;
  uint16_t BlockAlign;
  uint16_t BitsPerSample;
  /// data
  uint32_t Subchunk2ID;
  uint32_t Subchunk2Size;
} wavHeader;           //__attribute__((packed)) wavHeader;

/*** Function Prototypes ***/
void getWavData(const char * filePath);
void printfHeader(wavHeader * header);
void printfU32String(uint32_t array);
void swapEndiannes(uint32_t *value);
void startWebServer(void);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

RP2040_SDLib::File wavFile;
char * sndBuffer;

unsigned int localPort = 2390;  // local port to listen for UDP packets

//NTP Servers to choose from:
//static const char ntpServerName[] = "ntp.aussiebroadband.com.au";
//static const char ntpServerName[] = "time.nist.gov";
static const char ntpServerName[] = "au.pool.ntp.org";

int timeZone = 11;  //Assume daylight saving time, change it later if it's not

const char *ssid = "NetComm 5456";             //add your routers ssid name
const char *password = "T1b3r@ndr@L30T1m0r@";  //and your routers password

char msg1[32];
bool newMsg = false;  //Indicates a text messsage from the server

bool   AlarmTriggered = false;
time_t AlarmOnTime    = 0;
time_t AlarmOffTime   = 0;
int    AlarmPin       = 1;

Sd2Card SD_Card;
// SdVolume volume;
int sdStatus = 0;

RTC_DS3231 rtc;
int rtcStatus = 0;

WiFiUDP udp;    //used to get ntp time

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000);
  Serial.println("Setting up\n");

  displaySetup();  //this inits LCD controller, touch and backlight PWM
  setrotation(1);
  clear(BLACK);

  LCDPrint(130, 10, "Backpack Clock", YELLOW, BLACK);

  setBacklight(50);

  sdStatus = !SD_Card.init(SPI_HALF_SPEED, SDCS);  //Invert status for our purposes

  if (sdStatus) {
    LCDsmallPrint(10, 15, "NO Card", RED, BLACK);
  } else {
    switch (SD_Card.type()) {  //Display the type of card
      case SD_CARD_TYPE_SD1:
        LCDsmallPrint(10, 15, "SD1", GREEN, BLACK);
        break;
      case SD_CARD_TYPE_SD2:
        LCDsmallPrint(10, 15, "SD2", GREEN, BLACK);
        break;
      case SD_CARD_TYPE_SDHC:
        LCDsmallPrint(10, 15, "SDHC", GREEN, BLACK);
        break;
      default:
        LCDsmallPrint(10, 15, "Unknown", RED, BLACK);
        break;
    }
   }

  Wire1.setSDA(10);
  Wire1.setSCL(11);

  rtcStatus = rtc.begin(&Wire1);  //BackPack uses I2C1 on pins 10 and 11

  if (rtcStatus) {
    LCDsmallPrint(380, 15, "RTC OK", GREEN, BLACK);
  } else {
    LCDsmallPrint(380, 15, "No RTC", RED, BLACK);
  }

  audioInit();
  audioSetRate(8000);   //This gets changed if a wave file on the SD card is accessed

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  LCDsmallPrint(10, 50, "Connecting to WiFi", GREEN, BLACK);

  int x = 0;
  while (!WiFi.connected()) {
    delay(500);
    LCDsmallPrint(10 + x, 75, "+", RED, BLACK);
    x += 15;
  }
  LCDsmallPrint(10, 75, "                                 ", RED, BLACK);

  LCDsmallPrint(10, 100, "WiFi Connected            ", GREEN, BLACK);

  char ip[16];
  sprintf(ip, "%s", WiFi.localIP().toString().c_str());
  LCDsmallPrint(10, 125, "IP Address:", GREEN, BLACK);
  LCDsmallPrint(50, 150, ip, GREEN, BLACK);

  LCDsmallPrint(10, 195, "Waiting for NTP to Sync:", RED, BLACK);
  setSyncProvider(getNtpTime);
  setSyncInterval(3600);  //Now resync time only once per hour

  LCDsmallPrint(10, 195, "Clock Synchronised       ", RED, BLACK);

  delay(2000);
  clear(BLACK);
  draw_clock_face();

  if (rtcStatus) {
    rtc.adjust(now());
  }

  ws.onEvent(onEvent);
  server.addHandler(&ws);
  Serial.println("Starting WebServer");
  startWebServer();
}

void loop() {
  static int curSecs = 0;

  if (rtcStatus && (second() != curSecs)) {
    curSecs = second();
    showTime(rtc.now());
  }

  if(AlarmOnTime != 0) {
    if(now() == AlarmOnTime && AlarmTriggered == false) {
      AlarmTriggered = true;
      digitalWrite(AlarmPin, true);
      clear(0, 0, 480, 30, BLACK);
      LCDsmallPrint(0, 10, "Alarm triggered", YELLOW, BLACK);
   } else if(now() == AlarmOffTime && AlarmTriggered == true) {
      AlarmTriggered = false;
      digitalWrite(AlarmPin, false);
      clear(0, 0, 480, 30, BLACK);
      LCDsmallPrint(0, 10, "Alarm Reset", YELLOW, BLACK);
    }
  }

  if (newMsg) {
    clear(0, 0, 480, 30, BLACK);
    LCDsmallPrint(0, 10, msg1, YELLOW, BLACK);
    newMsg = false;
  }

  ws.cleanupClients();
  delay(10);
}

void getWavData(const char * filePath) {
  uint32_t numSamples;
  wavHeader header;

    sdStatus = SD.begin(SDCS);
    wavFile = SD.open(filePath, FILE_READ);

  if (wavFile) {
    wavFile.read(&header, sizeof(header));
    // printfHeader(&header);
    Serial.println(sizeof(header));
    if(header.BitsPerSample != 8 || header.NumChannels > 1) {
      Serial.println("Only 8 bit mono wave files currently supported");
    } else {
      numSamples = header.Subchunk2Size / (header.BitsPerSample / 8);
      sndBuffer = (char *)malloc(numSamples);     //Allocate a buffer on the heap for the sound data (must be free'd later)
      wavFile.readBytes(sndBuffer, numSamples);   //Don't use ::read() as only uint16_t bytes can be read
      audioSetRate(header.SampleRate);
      if (audioSpace()) {
        audioQueue((const char *)sndBuffer, numSamples);
        free(sndBuffer);    
      }
      wavFile.close();
    }  
  } else {
    Serial.println("Cannot open file");
  }
}

void playSound(int sndNum) {
  switch (sndNum) {
  case 1:  getWavData("/goodMorn.wav"); break;
  case 2:  getWavData("/goodAftr.wav"); break;
  case 3:  getWavData("/goodEve.wav"); break;
  case 4:  getWavData("/goodNite.wav"); break;
  case 5:  getWavData("/deleted.wav"); break;
  default: getWavData("/rolls.wav");   //Queues up the data and free's memory
  }

  if (audioSpace()) {
    audioPlay(AUDIO_PLAY_MONO);
  }
}

void draw_clock_face(void) {
  int x, y, x1, y1;

  // draw the center of the clock
  circle(clkCentre_x, clkCentre_y, 110, YELLOW);
  fcircle(clkCentre_x, clkCentre_y, 80, BLACK);
  fcircle(clkCentre_x, clkCentre_y, 10, GREY);

  // draw hour pointers around the face of a clock
  for (int i = 0; i < 12; i++) {
    y = (outerRadius * cos(pi - (2 * pi) / 12 * i)) + clkCentre_y;
    x = (outerRadius * sin(pi - (2 * pi) / 12 * i)) + clkCentre_x;
    y1 = (innerRadius * cos(pi - (2 * pi) / 12 * i)) + clkCentre_y;
    x1 = (innerRadius * sin(pi - (2 * pi) / 12 * i)) + clkCentre_x;
    line(x1, y1, x, y, WHITE);
  }

//  circle(innerRadius * sin(pi) + clkCentre_x, (80 * cos(pi)) + clkCentre_y, 6, WHITE);
//  fcircle(innerRadius * sin(pi) + clkCentre_x, (80 * cos(pi)) + clkCentre_y, 5, BLACK);
}

/************* Format parameters for DateTime::toString() ***************
** | specifier | output                                                 |
** |-----------|--------------------------------------------------------|
** | YYYY      | the year as a 4-digit number (2000--2099)              |
** | YY        | the year as a 2-digit number (00--99)                  |
** | MM        | the month as a 2-digit number (01--12)                 |
** | MMM       | the abbreviated English month name ("Jan"--"Dec")      |
** | DD        | the day as a 2-digit number (01--31)                   |
** | DDD       | the abbreviated English day of the week ("Mon"--"Sun") |
** | AP        | either "AM" or "PM"                                    |
** | ap        | either "am" or "pm"                                    |
** | hh        | the hour as a 2-digit number (00--23 or 01--12)        |
** | mm        | the minute as a 2-digit number (00--59)                |
** | ss        | the second as a 2-digit number (00--59)                |
************************************************************************/

static int secsX, secsY, secsXOld, secsYOld;
static int minsX, minsY, minsXOld, minsYOld;
static int hourX, hourY, hourXOld, hourYOld, hourX1, hourY1, hourX1Old, hourY1Old;

void showTime(DateTime t) {
  char s[] = "DD/MM/YYYY hh:mm:ss";

  secsXOld = secsX;  //Save the values for blanking out the old hands
  secsYOld = secsY;
  minsXOld = minsX;
  minsYOld = minsY;
  hourXOld = hourX;
  hourYOld = hourY;
  hourX1Old = hourX1;
  hourY1Old = hourY1;

  fcircle(clkCentre_x, clkCentre_y, 10, GREY);  //Redraw the centre as it gets progressively overwritten

  // Seconds hand display
  secsY = (innerRadius * cos(pi - (2 * pi) / 60 * t.second())) + clkCentre_y;
  secsX = (innerRadius * sin(pi - (2 * pi) / 60 * t.second())) + clkCentre_x;
  line(250, 150, secsXOld, secsYOld, BLACK);  //Wipe out last hand
  line(250, 150, secsX, secsY, GREEN);        //Draw new hand

  //Minutes hand display
  minsY = (innerRadius * cos(pi - (2 * pi) / 60 * t.minute())) + clkCentre_y;
  minsX = (innerRadius * sin(pi - (2 * pi) / 60 * t.minute())) + clkCentre_x;
  line(clkCentre_x - 1, clkCentre_y - 1, minsXOld, minsYOld, BLACK);  //Wipe out last hand
  line(clkCentre_x, clkCentre_y, minsXOld, minsYOld, BLACK);
  line(clkCentre_x + 1, clkCentre_y + 1, minsXOld, minsYOld, BLACK);
  line(clkCentre_x - 1, clkCentre_y - 1, minsX, minsY, YELLOW);  //Draw new hand
  line(clkCentre_x, clkCentre_y, minsX, minsY, YELLOW);
  line(clkCentre_x + 1, clkCentre_y + 1, minsX, minsY, YELLOW);

  //Hour hand display
  hourY = ((innerRadius - 20) * cos(pi - (2 * pi) / 12 * t.hour() - (2 * pi) / 720 * t.minute())) + clkCentre_y;
  hourX = ((innerRadius - 20) * sin(pi - (2 * pi) / 12 * t.hour() - (2 * pi) / 720 * t.minute())) + clkCentre_x;
  hourY1 = ((innerRadius - 20) * cos(pi - (2 * pi) / 12 * t.hour() - (2 * pi) / 720 * t.minute())) + clkCentre_y;
  hourX1 = ((innerRadius - 20) * sin(pi - (2 * pi) / 12 * t.hour() - (2 * pi) / 720 * t.minute())) + clkCentre_x;
  line(clkCentre_x - 1, clkCentre_y - 1, hourXOld, hourYOld, BLACK);  //Wipe out last hand
  line(clkCentre_x, clkCentre_y, hourX1Old, hourY1Old, BLACK);
  line(clkCentre_x + 1, clkCentre_y + 1, hourX1Old, hourY1Old, BLACK);
  line(clkCentre_x - 1, clkCentre_y - 1, hourX, hourY, WHITE);  //Draw new hand
  line(clkCentre_x, clkCentre_y, hourX1, hourY1, WHITE);
  line(clkCentre_x + 1, clkCentre_y + 1, hourX1, hourY1, WHITE);

  //Show full date and time at the bottom
  LCDPrint(100, 295, t.toString(s), WHITE, BLACK);
}

void CheckTimeZone(void) {
  if (timeStatus() != timeNotSet) {
    if (month() == 10 || month() == 4) {  //Possible change of DST
      if (weekday() >= 1 && hour() >= 2) {
        timeZone = (month() == 4) ? 10 : 11;
      }
    } else {
      timeZone = (month() > 10 || month() < 4) ? 11 : 10;
    }
  }
}

const int NTP_PACKET_SIZE = 48;      // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE];  //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  udp.begin(localPort);

  IPAddress ntpServerIP;  // NTP server's ip address
  int count = 15;
  int size;

  while (udp.parsePacket() > 0)
    ;  // discard any previously received packets
  // get a server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);

  while (count-- > 0) {
    sendNTPpacket(ntpServerIP);

    Serial.print("*");
    size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];

      Serial.println("");
      time_t t = secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      setTime(t);
      CheckTimeZone();  //If daylight saving is in play we need to adjust the timezone by +- 1
      t = secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      setTime(t);
      udp.stop();
      return (secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
    }

    uint32_t beginWait = millis();
    while (millis() - beginWait < 1000)
      ;  //Wait a while
  }
  Serial.println("No NTP Response :(");
  udp.stop();
  return 0;  // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for wavFile Delay & wavFile Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123);  //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void printfHeader(wavHeader * header) {
  Serial.println("WAV HEADER");
  Serial.println("--------------------------------------");
  Serial.print("ChunkID : ");
  swapEndiannes(&header->ChunkID);
  printfU32String(header->ChunkID);
  Serial.printf("ChunkSize : %d\n", header->ChunkSize);
  Serial.printf("Format : ");
  swapEndiannes(&header->Format);
  printfU32String(header->Format);
  /// fmt
  Serial.printf("Subchunk1ID : ");
  swapEndiannes(&header->Subchunk1ID);
  printfU32String(header->Subchunk1ID);
  Serial.printf("Subchunk1Size : %d\n", header->Subchunk1Size);
  if (header->AudioFormat == 1) {
    Serial.printf("AudioFormat : PCM\n");
  } else {
    Serial.printf("AudioFormat : Compression\n");
  }
  Serial.printf("NumChannels : %d\n", header->NumChannels);
  Serial.printf("SampleRate : %d\n", header->SampleRate);
  Serial.printf("ByteRate : %d\n", header->ByteRate);
  Serial.printf("BlockAlign : %d\n", header->BlockAlign);
  Serial.printf("BitsPerSample : %d\n", header->BitsPerSample);
  /// data
  Serial.printf("Subchunk2ID : ");
  swapEndiannes(&header->Subchunk2ID);
  printfU32String(header->Subchunk2ID);
  Serial.printf("Subchunk2Size : %d\n", header->Subchunk2Size);
  Serial.printf("Number of Samples : %d\n", header->Subchunk2Size / (header->NumChannels * header->BitsPerSample / 8));
  Serial.println("--------------------------------------");
}

void swapEndiannes(uint32_t *value) {
  uint32_t tmp = *value;
  *value = (tmp >> 24) | (tmp << 24) | ((tmp >> 8) & 0x0000FF00) | ((tmp << 8) & 0x00FF0000);
}

void printfU32String(uint32_t array) {
  char text[5];
  text[0] = (array >> 24) & 0xFF;
  text[1] = (array >> 16) & 0xFF;
  text[2] = (array >> 8) & 0xFF;
  text[3] = array & 0xFF;
  text[4] = 0;
  Serial.printf(" %s\n", text);
}
