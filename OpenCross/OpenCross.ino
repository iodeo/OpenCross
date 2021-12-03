#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Ticker.h>

// DEBUG
#define DEBUG false

// BRANCHEMENT
#define PWM_PIN 4 //D2
#define RPM_PIN 5 //D1

// HACHAGE BUILT-IN PWM
// commenter ci dessous pour utiliser le mode ticker
#define FREQ 100 //de 100 to 40000
#ifdef FREQ
// level map -- pour avoir une resistance ressentie progressive  
const int pwm[10] = {0, 28, 50, 67, 81, 93, 103, 112, 120, 127};
#endif

#ifndef FREQ
// HACHAGE TICKER PWM (if built-in is too quick)
/* Le hachage se fait sur 10 niveau
 *  - de 0 : 0% duty cycle
 *  - à  9 : 50% duty cycle*/
#define TIME_UNIT 5 //10ms 
#define TIME_PERIOD 90 // 9*TIME_UNIT * 2*/
Ticker tickerPwmHigh;
Ticker tickerPwmLow;
#endif

const IPAddress apIp(192, 168, 100, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

const char * ssid = "OpenCross";
const char * pass = "password";

int minutes = 15;
int level = 0;
int temp = 0;

bool reqTime = false;
bool gotTime = false;
bool gotLevel = false;
bool getDisconnected = false;

void httpDefault() {
  server.sendHeader("Location", "http://opencross.local/", true);
  server.send(302, "text/plain", "");
  server.client().stop();
}

void httpHome() {
  if (server.hostHeader() != String("opencross.local")) {
    return httpDefault();
  }
  File file = SPIFFS.open("/index.htm", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void httpMain() {
  File file = SPIFFS.open("/run.htm", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void httpStyle() {
  File file = SPIFFS.open("/style.css", "r");
  server.streamFile(file, "text/css");
  file.close();
}

void httpScript() {
  File file = SPIFFS.open("/script.js", "r");
  server.streamFile(file, "text/js");
  file.close();
}

void httpManifest() {
  File file = SPIFFS.open("/manifest.json", "r");
  server.streamFile(file, "application/json");
  file.close();
}

void httpImgFavicon() {
  File file = SPIFFS.open("/img/favicon.svg", "r");
  server.streamFile(file, "image/svg+xml");
  file.close();
}

void httpImgMinus() {
  File file = SPIFFS.open("/img/minus.png", "r");
  server.streamFile(file, "image/png");
  file.close();
}

void httpImgPlus() {
  File file = SPIFFS.open("/img/plus.png", "r");
  server.streamFile(file, "image/png");
  file.close();
}

void httpImgPlay() {
  File file = SPIFFS.open("/img/play.png", "r");
  server.streamFile(file, "image/png");
  file.close();
}

void httpImgPause() {
  File file = SPIFFS.open("/img/pause.png", "r");
  server.streamFile(file, "image/png");
  file.close();
}

void httpImgStop() {
  File file = SPIFFS.open("/img/stop.png", "r");
  server.streamFile(file, "image/png");
  file.close();
}

void setup() {
#if DEBUG  
  Serial.begin(115200);
  Serial.println();
  Serial.println();
#endif

  SPIFFS.begin();

  WiFi.persistent(false);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIp, apIp, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, pass, 1, false, 1);

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIp);

  server.on("/", httpHome);
  server.on("/run.htm", httpMain);
  server.on("/style.css", httpStyle);
  server.on("/script.js", httpScript);
  server.on("/manifest.json", httpManifest);
  server.on("/img/favicon.svg", httpImgFavicon);
  server.on("/img/minus.png", httpImgMinus);
  server.on("/img/plus.png", httpImgPlus);
  server.on("/img/play.png", httpImgPlay);
  server.on("/img/pause.png", httpImgPause);
  server.on("/img/stop.png", httpImgStop);
  
  server.onNotFound(httpDefault);
  server.begin();

  pinMode(PWM_PIN, OUTPUT);
  pinMode(RPM_PIN, INPUT);

#if FREQ
  analogWriteFreq(FREQ);
#endif

  setPwmLow();

  minutes = getValueFromFile("/minutes.dat");
  
#if DEBUG    
  Serial.print("Get time: "); Serial.println(minutes);
#endif

}

void loop() {
  
  dnsServer.processNextRequest();
  webSocket.loop();
  server.handleClient();

  if (reqTime) { // send saved time to client
    reqTime = false;
    String msg = "m" + String(minutes);
    webSocket.sendTXT(0, msg);
#if DEBUG      
    Serial.print("SendTime: ");
    Serial.println(msg);
#endif    
  }

  if (gotTime) { // update default time
    gotTime = false;
    if (minutes != temp) {
      minutes = temp;
      setValueToFile("/minutes.dat", minutes);
#if DEBUG        
      Serial.print("Save time: "); Serial.println(minutes);
#endif      
    }
  }

  if (gotLevel) { // change pwm level
    gotLevel = false;
    if (level != temp) {
      level = temp;
#if DEBUG        
      Serial.print("SetPwmLevel: ");
      Serial.println(level);
#endif      
      setPwmLevel(level);
    }
  }

  if (getDisconnected) {
    getDisconnected = false;
    setPwmLevel(0);
  }
  
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
#if DEBUG      
      Serial.printf("[%u] Disconnected!\n", num);
#endif      
      getDisconnected = true;
      break;
    case WStype_CONNECTED: 
      {
        IPAddress ip = webSocket.remoteIP(num);
#if DEBUG          
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
#endif        
      }
      break;
    case WStype_TEXT:
#if DEBUG      
      Serial.printf("[%u] get Text: %s\n", num, payload);
#endif      
      if(payload[0] == 'm') { //time
        if(payload[1] == '?') { //request
          reqTime = true;
        }
        else { //update
          temp = atoi((const char *) &payload[1]);
          gotTime = true;
        }
      }
      if(payload[0] == 'l') { //level
        temp = atoi((const char *) &payload[1]);
        gotLevel = true;
      }
      break;
    default:
      break;
  }
}

void setPwmLevel(byte level) {


#ifdef FREQ 
// BUILTIN PWM
  if (level >= 0 && level < 10) {
    analogWrite(PWM_PIN, pwm[level]);
  }
  else {
    digitalWrite(PWM_PIN, LOW);
  }
#else 
// TICKER PWM
  if (level > 0 && level < 10) {
    tickerPwmHigh.attach_ms(TIME_PERIOD, setPwmHigh, level);
  }
  else {
    tickerPwmHigh.detach();
    setPwmLow();
  }
#endif
}

#ifndef FREQ
// TICKER PWM
void setPwmHigh(byte level) {
  digitalWrite(PWM_PIN, HIGH);
  tickerPwmLow.once_ms(TIME_UNIT*level, setPwmLow);
}
#endif

void setPwmLow() {
  digitalWrite(PWM_PIN, LOW);
}

int getValueFromFile(const char * path) {
  File file = SPIFFS.open(path, "r");
  int val = file.readStringUntil('\n').toInt();
  file.close();
  return val;
}

void setValueToFile(const char * path, int  val) {
  File file = SPIFFS.open(path, "w+");
  file.println(val);
  file.close();
}
