#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <math.h>
#include "soc/rtc_cntl_reg.h"

#include "webpage.h"

const char* AP_SSID = "ESP32-Control";
const char* AP_PASS = "12345678";
const IPAddress AP_IP(192, 168, 4, 1);

const uint8_t LED_PIN = 2;

enum LedMode { OFF, ON, BLINK, BREATHE };

LedMode mode = OFF;
uint8_t dim = 255;
uint16_t speedMs = 500;

bool levelHigh = false;
unsigned long lastTick = 0;

void pwm(uint8_t duty) {
  analogWrite(LED_PIN, duty);
}

void setLed(bool on) {
  analogWrite(LED_PIN, on ? dim : 0);
}

void switchMode(LedMode m) {
  mode = m;
  levelHigh = false;
  lastTick = millis();
  if (m == OFF)      { pwm(0); }
  else if (m == ON)  { pwm(dim); }
  else if (m == BLINK) { pwm(0); }
}

void updateLed() {
  unsigned long now = millis();
  switch (mode) {
    case BLINK:
      if (now - lastTick >= speedMs) {
        lastTick = now;
        levelHigh = !levelHigh;
        pwm(levelHigh ? dim : 0);
      }
      break;
    case BREATHE: {
      float k = (1 + sin(2 * PI * (float)(now % speedMs) / speedMs)) * 0.5;
      pwm((uint8_t)(k * dim));
      break;
    }
    default:
      break;
  }
}

WebServer web(80);

String jsonEscape(String text) {
  String out;
  for (char c : text) {
    if (c == '"' || c == '\\') out += '\\';
    out += c;
  }
  return out;
}

String secName(wifi_auth_mode_t auth) {
  switch (auth) {
    case WIFI_AUTH_OPEN:            return "open";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-EAP";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3";
    default:                        return "unknown";
  }
}

String net(int i) {
  String s = "{\"ssid\":\""       + jsonEscape(WiFi.SSID(i))      + "\"";
  s += ",\"bssid\":\""            + WiFi.BSSIDstr(i)               + "\"";
  s += ",\"rssi\":"               + String(WiFi.RSSI(i))           + ",";
  s += "\"channel\":"             + String(WiFi.channel(i))        + ",";
  s += "\"security\":\""          + secName(WiFi.encryptionType(i)) + "\"}";
  return s;
}

void scan() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING || n < 0) {
    if (n < 0) WiFi.scanNetworks(true);
    web.send(200, "application/json", "{\"status\":\"running\"}");
    return;
  }

  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i) json += ",";
    json += net(i);
  }
  json += "]";
  WiFi.scanDelete();
  web.send(200, "application/json", json);
}

void led() {
  String m = web.arg("mode");
  if (m == "on")      switchMode(ON);
  else if (m == "off") switchMode(OFF);
  else if (m == "blink") switchMode(BLINK);
  else if (m == "breathe") switchMode(BREATHE);
  web.send(200, "application/json", "{\"ok\":true}");
}

void settings() {
  if (web.hasArg("dim") && web.arg("dim").toInt() >= 0 && web.arg("dim").toInt() <= 255) {
    dim = web.arg("dim").toInt();
    if (mode == ON || (mode == BLINK && levelHigh)) pwm(dim);
    else if (mode == OFF) pwm(0);
  }
  if (web.hasArg("speed") && web.arg("speed").toInt() >= 100 && web.arg("speed").toInt() <= 3000) {
    speedMs = web.arg("speed").toInt();
  }
  web.send(200, "application/json", "{\"ok\":true}");
}

void state() {
  String s = "{\"mode\":";
  switch (mode) {
    case OFF:     s += "\"off\"";    break;
    case ON:      s += "\"on\"";     break;
    case BLINK:   s += "\"blink\"";  break;
    case BREATHE: s += "\"breathe\""; break;
  }
  s += ",\"dim\":"     + String(dim);
  s += ",\"speed\":"   + String(speedMs) + "}";
  web.send(200, "application/json", s);
}

void setup() {
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  WiFi.setTxPower(WIFI_POWER_11dBm);

  analogWriteResolution(8);
  analogWriteFrequency(5000);
  analogWrite(LED_PIN, 0);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, AP_PASS);

  web.on("/",         []() { web.send(200, "text/html", PAGE_HTML); });
  web.on("/style.css", []() { web.send(200, "text/css", STYLE_CSS); });
  web.on("/app.js",    []() { web.send(200, "application/javascript", APP_JS); });
  web.on("/scan",    scan);
  web.on("/led",     led);
  web.on("/set",     settings);
  web.on("/state",   state);
  web.begin();

  Serial.println("AP up: " + String(AP_SSID) + " -> http://192.168.4.1");
}

void loop() {
  web.handleClient();
  updateLed();
}