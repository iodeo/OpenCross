#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

#define PWM_PIN 4 //D2
#define RPM_PIN 5 //D1

unsigned int pwmFreq = 10000; // (en hertz) valeur par défaut de run.htm
unsigned int pwmValue = 127; // (max 255) valeur par defaut de run.htm
unsigned int temp = 0;
bool gotFreq = false;
bool gotValue = false;
bool gotRpm = false;
bool rpm = false;

const IPAddress apIp(192, 168, 100, 1);
DNSServer dnsServer;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

const char * ssid = "OpenCross";
const char * pass = "password";

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

void setup() {
  
  Serial.begin(115200);
  Serial.println();
  Serial.println();

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
  
  server.onNotFound(httpDefault);
  server.begin();

  pinMode(PWM_PIN, OUTPUT);
  pinMode(RPM_PIN, INPUT);

  analogWriteFreq(pwmFreq);
  analogWrite(PWM_PIN, pwmValue);

}

void loop() {
  
  dnsServer.processNextRequest();
  webSocket.loop();
  server.handleClient();

  if (gotFreq) {
    gotFreq = false;
    if ((temp >= 100) && (temp <= 40000)) pwmFreq = temp;
    Serial.print("WriteFreq: ");Serial.println(pwmFreq);
    analogWriteFreq(pwmFreq);
  }

  if (gotValue) {
    gotValue = false;
    if ((temp >= 0) && (temp <= 255)) pwmValue = temp;
    Serial.print("WritePwm: ");Serial.println(pwmValue);
    analogWrite(PWM_PIN, pwmValue);
  }

  if (digitalRead(RPM_PIN)) {
    if (!rpm) gotRpm = true;
  }
  else {
    if (rpm) gotRpm = true;
  }

  if (gotRpm) {
    gotRpm = false;
    rpm = !rpm;
    Serial.print("GotRpm: ");Serial.println(rpm);
    if (rpm) webSocket.sendTXT(0, "rpmUp");
    if (!rpm) webSocket.sendTXT(0, "rpmDown");
  }
  
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            webSocket.sendTXT(num, "Connected");
        }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\n", num, payload);

            if(payload[0] == 'f') {
                temp = atoi((const char *) &payload[1]);
                webSocket.sendTXT(num, payload);
                gotFreq = true;
            }
            if(payload[0] == 'r') {
                temp = atoi((const char *) &payload[1]);
                webSocket.sendTXT(num, payload);
                gotValue = true;
            }
            
            break;
    }
}
