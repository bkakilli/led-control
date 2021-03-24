#include <Arduino.h>

// Common definition and imports
#ifndef LED_BUILTIN
#define LED_BUILTIN 13 // ESP32 DOES NOT DEFINE LED_BUILTIN
#endif
int LED = LED_BUILTIN;


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


/*****************************************************************************************************
 * FASTLED CONFIGURATION
 * */
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>

#define NUM_LEDS            20      /* The amount of pixels/leds you have */
#define DATA_PIN            D5      /* The pin your data line is connected to */
#define LED_TYPE WS2812B    /* I assume you have WS2812B leds, if not just change it to whatever you have */
#define BRIGHTNESS          255   /* Control the brightness of your leds */
#define SATURATION          255   /* Control the saturation of your leds */
#define COLOR_ORDER         GRB
#define MILLI_AMPS          200 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)
#define FRAMES_PER_SECOND   120  // here you can control the speed. With the Access Point / Web Server the animations run a bit slower.

CRGB leds[NUM_LEDS];
uint8_t hue_value = 0;
uint8_t led_bpm = 10;
uint8_t draw_period_ms = 1000/FRAMES_PER_SECOND;

void setup_fastled() {
  
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);         // for WS2812 (Neopixel)
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS); // for APA102 (Dotstar)
  FastLED.setDither(false);
  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
  FastLED.clear();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void fastled_update() {
  int delta_hue = 255/NUM_LEDS;
  hue_value = beat8(led_bpm);
  fill_rainbow(leds, NUM_LEDS, hue_value, delta_hue);
  // leds[0] = CRGB(255, 0, 0);
  // leds[1] = CRGB(0, 255, 0);
  // leds[2] = CRGB(0, 0, 255);
  FastLED.show();
}

void setup() {
  Serial.begin(115200);

  setup_wifi();
  setup_web_server();
  setup_fastled();
}

void loop() {
  // put your main code here, to run repeatedly:
  // for (int i=0; i<3; i++) {
  //   digitalWrite(LED, LOW);
  //   delay(150);
  //   digitalWrite(LED, HIGH);
  //   delay(150);
  // }
  // delay(1100);
  fastled_update();
}