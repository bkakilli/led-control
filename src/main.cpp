#include <Arduino.h>

// Common definition and imports
#ifndef LED_BUILTIN
#define LED_BUILTIN 13 // ESP32 DOES NOT DEFINE LED_BUILTIN
#endif
int LED = LED_BUILTIN;


/*****************************************************************************************************
 * FASTLED CONFIGURATION
 * */
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include "led_strip.h"
#include <vector>

#define NUM_LEDS            24      /* The amount of pixels/leds you have */
#define DATA_PIN            D5      /* The pin your data line is connected to */
#define LED_TYPE WS2812B    /* I assume you have WS2812B leds, if not just change it to whatever you have */
#define BRIGHTNESS          255   /* Control the brightness of your leds */
#define SATURATION          255   /* Control the saturation of your leds */
#define COLOR_ORDER         GRB
#define MILLI_AMPS          200 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND   120  // here you can control the speed. With the Access Point / Web Server the animations run a bit slower.

uint8_t draw_period_ms = 1000/FRAMES_PER_SECOND;

std::vector<LedStrip*> strips;

void setup_fastled() {

  // Add as many ports as you like.
  // Each port is controlled independently
  strips.push_back(new LedStripPort<D5>(NUM_LEDS));
  // strips.push_back(new LedStripPort<D6>(NUM_LEDS));
  // strips.push_back(new LedStripPort<D7>(NUM_LEDS));
  // strips.push_back(new LedStripPort<D8>(NUM_LEDS));

  strips[0]->set_palette(CPT_TASHANGEL_GP);
  // strips[1]->set_palette(CPT_RAINBOW_GP);
  // strips[0]->init(NUM_LEDS);
  FastLED.show();
}

void fastled_update() {
  for (auto it=strips.begin(); it != strips.end(); ++it)
    (*it)->update();
  FastLED.show();
}


/******************************************************************************************
 * WIFIMANAGER CONFIGURATION
 * */
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#define WEBSERVER_H

// LED will blink when in config mode
#include <Ticker.h>
Ticker ticker;
void tick() {
  //toggle state
  digitalWrite(LED, !digitalRead(LED));     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void setup_wifi() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // put your setup code here, to run once:
  
  //set led pin as output
  pinMode(LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  //reset settings - for testing
  // wm.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wm.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(LED, HIGH);
}


/******************************************************************************************
 * WEBSERVER CONFIGURATION
 * */

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <stdlib.h>
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
String ledState;

String getTemperature() {
  return String(20.0f);
}
  
String getHumidity() {
  return String(100.0f);
}

String getPressure() {
  return String(72.0f);
}

// Replaces placeholder with LED state value
String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(LED)){
      ledState = "ON";
    }
    else{
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  else if (var == "TEMPERATURE"){
    return getTemperature();
  }
  else if (var == "HUMIDITY"){
    return getHumidity();
  }
  else if (var == "PRESSURE"){
    return getPressure();
  }  
}
void setup_web_server() {

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(LED, LOW);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(LED, HIGH);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){

    if (!request->hasParam("port")) {return;}
    int port_id = atoi(request->getParam("port")->value().c_str());
    if (port_id >= strips.size()) {
      Serial.println("Undefined port is configured!");
      return;
    }
    
    LedStrip *strip = strips[port_id];

    if (request->hasParam("palette")) {
      String palette_id = request->getParam("palette")->value();
      
      Serial.println("Setting palette ID: " + palette_id);

      switch (atoi(palette_id.c_str()))
      {
      case 0:
        strip->set_palette(CPT_RAINBOW_GP);
        break;
      case 1:
        strip->set_palette(CPT_TASHANGEL_GP);
        break;
      case 2:
        strip->set_palette(CPT_SCOUTIE_GP);
        break;
      case 3:
        strip->set_palette(CPT_AQUAMARINEMERMAID_GP);
        break;
      case 4:
        strip->set_palette(CPT_BLUEFLY_GP);
        break;
      case 5:
        strip->set_palette(CPT_BLACKHORSE_GP);
        break;
      case 6:
        strip->set_palette(CPT_BHW2_REDROSEY_GP);
        break;
      case 7:
        strip->set_palette(CPT_DANCES_WITH_DRAGONS_GP);
        break;
      case 8:
        strip->set_palette(CPT_CYAN_MAGENTA_YELLOW_WHITE_GP);
        break;
        
      default:
        break;
      }
      
    }
    
    if (request->hasParam("fill_color")) {
      String fill_color = request->getParam("fill_color")->value();
      Serial.println("Setting solid color " + fill_color);
      const char *str = fill_color.c_str();
      int r, g, b;
      sscanf(str, "%02x%02x%02x", &r, &g, &b);
      strip->set_solid_color(CRGB(r, g, b));
    }
    
    if (request->hasParam("set_bpm")) {
      uint16_t bpm = atoi(request->getParam("set_bpm")->value().c_str());
      Serial.println("Setting bpm " + String(bpm));
      strip->set_bpm(bpm);
    }
    
    if (request->hasParam("set_brightness")) {
      uint8_t brightness = atoi(request->getParam("set_brightness")->value().c_str());
      Serial.println("Setting brightness " + String(brightness));
      strip->set_brightness(brightness);
    }

  });

  server.on("/image", HTTP_GET, [](AsyncWebServerRequest *request){
    
    if (request->hasParam("name")) {
      String image_name = request->getParam("name")->value();
      request->send(SPIFFS, "/images/palettes/"+image_name, "image/svg+xml");
    }
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getTemperature().c_str());
  });
  
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getHumidity().c_str());
  });
  
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getPressure().c_str());
  });

  // Start server
  server.begin();
}

void setup() {
  Serial.begin(115200);

  setup_wifi();
  setup_web_server();
  setup_fastled();
}

void loop() {
  EVERY_N_MILLISECONDS(draw_period_ms) {
    fastled_update();
  }
}