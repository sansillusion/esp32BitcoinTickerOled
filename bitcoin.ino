#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>          //https://github.com/esp8266/Arduino
#endif
#include <DNSServer.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Ticker.h>
Ticker ticker;

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);// D1 et D2 sur esp8266

const int httpPort = 80;
const char* host = "api.coindesk.com";
int checkleprix = 0;
String denprix;
String leprix;
int dernbon;
int ladir = 0;
int netroule = 0;
int pas = 0;

void tick() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvR14_tf);
    u8g2.setCursor(5, 25);
    if (pas == 0) {
      u8g2.print("Use ton cell");
    }
    if (pas == 1) {
      u8g2.print("Connect a \:");
    }
    if (pas == 2) {
      u8g2.print("BitcoinAP");
    }
  } while ( u8g2.nextPage() );
  pas++;
  if (pas == 3) {
    pas = 0;
  }
}

void setup() {
  Serial.begin(9600);
  u8g2.begin();
  u8g2.setFontMode(0);    // enable transparent mode, which is faster
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvR14_tf);
    u8g2.setCursor(5, 25);
    u8g2.print("Connection !");
  } while ( u8g2.nextPage() );
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setTimeout(240);
  if (!wifiManager.autoConnect("BitcoinAP")) {
    Serial.println("failed to connect and hit timeout");
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_helvR14_tf);
      u8g2.setCursor(5, 25);
      u8g2.print("Impossible !");
    } while ( u8g2.nextPage() );
    //reset and try again, or maybe put it to deep sleep
    delay(1000);
    ESP.restart();
    delay(1000);
  }
  ticker.detach();
  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.println("Addresse IP: ");
  Serial.println(WiFi.localIP());
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvR14_tf);
    u8g2.setCursor(5, 25);
    u8g2.print("Demarage !");
  } while ( u8g2.nextPage() );
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Mode Sta pour config.");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  ticker.attach(2, tick);
}

char* string2char(String command) {
  if (command.length() != 0) {
    char *p = const_cast<char*>(command.c_str());
    return p;
  }
}

void loop() {
  while (netroule == 0) {
    WiFiClient client;
    if (!client.connect(host, httpPort)) {
      Serial.println("Erreur de connection !");
      return;
    }
    String url = "/v1/bpi/currentprice.json";
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    String reponce;
    while (client.available()) {
      String ligne = client.readStringUntil('\r');
      reponce += ligne;
    }
    client.stop();
    String lejson;
    int lindex;
    for (int i = 0; i < reponce.length(); i++) {
      if (reponce[i] == '{') {
        lindex = i;
        break;
      }
    }
    lejson = reponce.substring(lindex);
    lejson.trim();
    int indprix = lejson.indexOf("rate_float");
    leprix = lejson.substring(indprix + 12, indprix + 18);
    leprix.trim();
    float prix = leprix.toFloat();
    if (prix == 0) {
      if (checkleprix == 0) {
        leprix = "Pas encore !";
      } else {
        leprix = denprix;
      }
    } else {
      Serial.println("Prix du Bitcoin en US: ");
      Serial.println(prix);
      denprix = leprix;
      checkleprix = 1;
      netroule = 1;
    }
    if (checkleprix == 1) {
      if (dernbon < prix) {
        ladir = 1;
      }
      if (dernbon > prix) {
        ladir = 2;
      }
      if (dernbon == prix) {
        ladir = 3;
      }
      dernbon = prix;
    }
    if (netroule == 0) {
      delay(5000);
    }
  }
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_9x15_t_symbols);
    if (ladir == 1) {
      u8g2.drawUTF8(120, 25, "↑");
    }
    if (ladir == 2) {
      u8g2.drawUTF8(120, 25, "↓");
    }
    if (ladir == 3) {
      u8g2.drawUTF8(120, 25, "↕");
    }
    u8g2.setFont(u8g2_font_helvR24_tf);
    u8g2.setCursor(1, 30);
    u8g2.print(leprix);
  } while ( u8g2.nextPage() );
  delay(300000);
  netroule = 0;
}

