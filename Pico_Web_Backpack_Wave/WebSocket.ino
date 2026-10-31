#include <stdlib.h>
#include <AsyncWebServer_RP2040W.h>
#include "Arduino.h"
#include "index.h"         //The full web page HTML

bool msgSent = false;
// extern char   msg1[32];
// extern time_t AlarmOn;
// extern time_t AlarmOff;

extern AsyncWebServer server;
extern AsyncWebSocket ws;
void   handleWebSocketMessage(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len);

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\r\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\r\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(client, arg, data, len);
     Serial.printf("WebSocket client #%u - %s\r\n", client->id(), data);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleWebSocketMessage(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  char s[] = "DD/MM/YYYY hh:mm:ss";
  char ret[80];
  char * msg;
  char * param1;
  char * param2;

  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;

    char * cmd = strtok((char*)data, ","); //cmd, param1 : param2...
    char * param  = strtok(NULL, ",");

    ret[0] = 0;
    
//Serial.printf("%s %s/r/n", cmd, param);
    
    if (strcmp(cmd, "putMsg") == 0) {
      msgSent = false;                         //Don't send btn response if already sent
      strcpy(msg1, strtok((char*)param, "#")); //Format: msg#Yes/No/Ok
      strcpy(ret, "Message Recieved");
      newMsg = true;
    } else if (strcmp(cmd, "setAlarm") == 0) {
      param1   = strtok((char*)param, "#");    //Format: AlarmOn#AlarmOff
      param2   = strtok(NULL, "#");
      tmElements_t tmOn;
      breakTime(now(), tmOn);    //Get the major elements of the current date time, i.e. Template;
      tmOn.Hour   = atoi(strtok((char*)param1, ":"));   //Now set our desired 'on' time values
      tmOn.Minute = atoi(strtok(NULL, ":"));
      tmOn.Second = 0;
      AlarmOnTime  = makeTime(tmOn);
      tmElements_t tmOff;
      tmOff = tmOn;    //Copy tmOn to tmOff
      tmOff.Hour   = atoi(strtok((char*)param2, ":"));   //Now set our desired 'off' time values
      tmOff.Minute = atoi(strtok(NULL, ":"));
      AlarmOffTime = makeTime(tmOff);
      sprintf(msg1, "Alarm- On %02d:%02d Off %02d:%02d", tmOn.Hour, tmOn.Minute, tmOff.Hour, tmOff.Minute);
      newMsg = true;
      strcpy(ret, "Alarm Set");
    } else if (strcmp(cmd, "playSound") == 0) {
      playSound(atoi(param));
      strcpy(ret, "Sound played");
    } else if (strcmp(cmd, "getPiStatus") == 0) {
      DateTime dt;
      int wd;

      if (rtcStatus) {
        dt = rtc.now();
        wd = dt.dayOfTheWeek() + 1;   //rtc dayofweek is zero based
      } else {
        dt = now();
        wd = dayOfWeek(now());
      }
      sprintf(ret, "Device Time: %s %02d/%02d/%04d %02d:%02d:%02d",
                   dayStr(wd), dt.day(), dt.month(), dt.year(), dt.hour(), dt.minute(), dt.second());
      ws.text(client->id(), String("Status;" + String(ret)));

      if(AlarmTriggered) {
        strcpy(ret, "Alarm has been triggered");
      } else
        ret[0] = 0;
    }

    //Event Response to be returned to only requesting client
    if(ret[0] != 0) {
      ws.text(client->id(), String("Response;" + String(ret)));
    }
  }
}

void startWebServer(void){
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });
  
  // server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(FS, "/favicon.ico", "image/ico");
  // });

  // Start server
  server.begin();
}
