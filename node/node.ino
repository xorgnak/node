// LED will blink when in config mode
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>
//for LED status
#include <Ticker.h>

#include <ss_oled.h>

#define RGB 2
#define RGB 15
#define MAT 3

int r = 4;
int g = 4;
int b = 4;
int mode = 0;
int state = 0;
const char * msg = "RIDE";
const char * utc = "";
bool blinking = false;
bool imon = false;
int lvl = 1;
int hp = 1;
int ac = 1;
int xp = 1;
int pip = 0;
int pt = 0;

long last = 0;
long lastText = 0;


const char broker[]    = "vango.me";
int        port        = 1883;
const char willTopic[] = "ping";
///////  ########     ----V pull chipid for network/owner/chip listening
const char inTopic[]   = "vango/7205522104/12345";
const char outTopic[]  = "vango/7205522104";

const long interval = 10000;
unsigned long previousMillis = 0;


#ifndef LED_BUILTIN
#define LED_BUILTIN 15 // 
#endif

int LED = LED_BUILTIN;

int count = 0;

#define LED_PIN    RGB
#define MAT_PIN    MAT
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 1
#define mw 21
#define mh 8
#define BRIGHTNESS 1

// if your system doesn't have enough RAM for a back buffer, comment out
// this line (e.g. ATtiny85)
//#define USE_BACKBUFFER

#ifdef USE_BACKBUFFER
static uint8_t ucBackBuffer[1024];
#else
static uint8_t *ucBackBuffer = NULL;
#endif

// Use -1 for the Wire library default pins
// or specify the pin numbers to use with the Wire library or bit banging on any GPIO pins
// These are the pin numbers for the M5Stack Atom default I2C
#define SDA_PIN 5
#define SCL_PIN 4
// Set this to -1 to disable or the GPIO pin number connected to the reset
// line of your display if it requires an external reset
#define RESET_PIN -1
// let ss_oled figure out the display address
#define OLED_ADDR -1
// don't rotate the display
#define FLIP180 0
// don't invert the display
#define INVERT 0
// Bit-Bang the I2C bus
#define USE_HW_I2C 0

#define MY_OLED OLED_128x64
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

#define DELAY_SCREEN 100
long long last_screen = 0;
bool updateScreen = false;
SSOLED ssoled;

void update_screen() {
   oledFill(&ssoled, 0, 1);
//  oledFill(&ssoled, 0x0, 1);
  last_screen = millis();

    char text_xp[4];
  char text_hp[3];
  char text_ac[3];
  char text_lvl[2];

  itoa(xp, text_xp, 10);
    itoa(hp, text_hp, 10);
  itoa(ac, text_ac, 10);
  itoa(lvl, text_lvl, 10);

  char * line0 = "";
  char * line1 = "";
   strcat(line0, "[");
    strcat(line0, text_xp);
   strcat(line0, "]");
  strcat(line0, text_xp);
   strcat(line0, " ");
  strcat(line0, text_hp);
  strcat(line0, "/");
  strcat(line0, text_ac);
  strcat(line1, utc);  
  oledWriteString(&ssoled, 0, 0, 0, line0, FONT_NORMAL, 0, 1);
  oledWriteString(&ssoled, 0, 0, 1, line1, FONT_NORMAL, 0, 1);
//  oledWriteString(&ssoled, 0, 0, 4, (char *)text_time_secs, FONT_LARGE, 0, 1);
}

Ticker ticker;



DynamicJsonDocument doc(1024);

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoMatrix *matrix = new Adafruit_NeoMatrix(mw, mh, MAT_PIN,
    NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
    NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE       ,
    NEO_GRB            + NEO_KHZ800);

void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

void wipe( uint32_t color ) {
  for (int a = 0; a < strip.numPixels(); a++) {
    strip.setPixelColor(a, color);
  }
  strip.show();
}

void led_blink() {
  //  if ((millis() - last) >= LED_WAIT) {
  last = millis(); // set last incr
  if (pip < lvl) {
    imon = !imon;
    if (imon == true) {
      wipe(strip.Color(r * 32, g * 32, b * 32));
    } else {
      pip++;

      wipe(strip.Color(0, 0, 0));
    }
  } else {
    pip = 0;
    blinking = false;
    imon = false;
  }

  //  }

}

void display_scrollText() {
  //  if ( (lastText - millis() ) >= LED_WAIT) {
  lastText = millis();
  matrix->clear();
  matrix->setTextWrap(false);  // we don't wrap text so it scrolls nicely
  matrix->setTextSize(0);
  matrix->setRotation(0);
  for (int8_t x = 7; x >= -64; x--) {
    matrix->clear();
    matrix->setCursor(x, 0);
    matrix->setTextColor(strip.Color(r * 32, g * 32, b * 32));
    matrix->print(msg);
    matrix->show();
  }
  //  }
}



void loop_led() {

  // wait for next tick
  //  if ((millis() - last) >= LED_WAIT) {
  last = millis(); // set last incr
  strip.clear(); // Set all pixel colors to 'off'
  pt = pt + (lvl - (lvl / 2) );
  bool op = false;
  for (int ox = (0 - strip.numPixels()); ox < strip.numPixels(); ox++ ) {
    for (int x = 0; x < lvl; x++) {
      if (op == true) {
        op = false;
        strip.setPixelColor((pt + x) + (2 ^ ox), strip.Color(r * 32, g * 32, b * 32)); // show pip
      } else {
        op = true;
        strip.setPixelColor((pt + x) + (2 ^ ox), strip.Color(0, 0, 0));
      }
    }
  }
  strip.show();   // show our pips.
  if (pt > strip.numPixels()) {
    blinking = true;
    pt = 0 - lvl;
  } // reset loop
  //  }

}

void tick()
{
  //toggle state
  digitalWrite(LED, !digitalRead(LED));     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  ticker.attach(0.2, tick);
}

void setup() {

  strip.begin();
  strip.show();
  strip.setBrightness(BRIGHTNESS);
  matrix->begin();
  matrix->setTextWrap(false);
  matrix->setBrightness(BRIGHTNESS);
  matrix->show();
  matrix->clear();

  //set led pin as output
  pinMode(LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.9, tick);

  int rc;  
  rc = oledInit(&ssoled, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, SDA_PIN, SCL_PIN, RESET_PIN, 400000L); // use standard I2C bus at 400Khz
  if (rc != OLED_NOT_FOUND)
  {
     oledFill(&ssoled, 0, 1);
//    oledFill(&ssoled, 0x0, 1);
    oledWriteString(&ssoled, 0, ((128 - (8  *  8)) / 2), 0, (char *)"op watch", FONT_NORMAL, 0, 1);
    oledWriteString(&ssoled, 0, ((128 - (8  * 11)) / 2), 1, (char *)"v. 0.0.0.a2", FONT_NORMAL, 0, 1);
    oledWriteString(&ssoled, 0, ((128 - (8  * 5)) / 2),  2, (char *)"=^.^=", FONT_NORMAL, 1, 1);
    oledWriteString(&ssoled, 0, ((128 - (8  * 8)) / 2),  3, (char *)"ridesafe", FONT_NORMAL, 1, 1);
    oledWriteString(&ssoled, 0, ((128 - (16 * 5)) / 2),  4, (char *)"READY", FONT_LARGE, 1, 1);
//    oledSetBackBuffer(&ssoled, ucBackBuffer);
    delay(2000);
  }
  
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
  if (!wm.autoConnect("NODE")) {
    ESP.restart();
    delay(1000);
  }

  // You can provide a unique client ID, if not set the library uses Arduin-millis()
  // Each client must have a unique client ID
  mqttClient.setId("12345");

// mqttClient.setUsernamePassword("username", "password");

// mqttClient.setCleanSession(false);
  String willPayload = "{\"state\": -1, \"id\": \"12345\" }";
  bool willRetain = true;
  int willQos = 1;

  mqttClient.beginWill(willTopic, willPayload.length(), willRetain, willQos);
  mqttClient.print(willPayload);
  mqttClient.endWill();


  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }

  mqttClient.onMessage(onMqttMessage);
  mqttClient.subscribe("time", 1);
  mqttClient.subscribe(inTopic, 1);

  mqttSend();

  ticker.detach();
    updateScreen = true;
  //keep LED on
  digitalWrite(LED, LOW);
}



void loop_screen() {
  if (updateScreen == true) {
    update_screen();
    updateScreen = false;
  }
}

void loop() {
  mqttClient.poll();
  if (blinking == true) {
    led_blink();
 } else {
    loop_led();
  }
  display_scrollText();
  loop_screen();
}

void mqttSend() {
  String payload;
  doc["millis"] = millis();
  doc["r"] = r;
  doc["g"] = g;
  doc["b"] = b;
  doc["state"] = state;
  doc["mode"] = mode;
  doc["lvl"] = lvl;
  doc["hp"] = hp;
  doc["ac"] = ac;
  doc["xp"] = xp;
  doc["msg"] = msg;
  JsonArray aps = doc.createNestedArray("nodes");
  WiFi.scanNetworks();
  if (WiFi.scanComplete() > 0) {
    for (int i = 1; i < WiFi.scanComplete(); i++) {
      char s[64];
      sprintf(s, "%s %d %d",  WiFi.BSSIDstr(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
      if (WiFi.SSID(i).startsWith("ESP")) {
        aps.add(s);
      }
    }
  }
  serializeJson(doc, payload);

  bool retained = false;
  int qos = 1;
  bool dup = false;

  mqttClient.beginMessage(outTopic, payload.length(), retained, qos, dup);
  mqttClient.print(payload);
  mqttClient.endMessage();
  xp++;
}

void onMqttMessage(int messageSize) {
  blinking = true;
  char m[20];
  // use the Stream interface to print the contents
  for (int i = 0; i < mqttClient.available(); i++) {
    m[i] = (char)mqttClient.read();
    m[i + 1] = '\0';
  }
  DeserializationError error = deserializeJson(doc, m);
  // Test if parsing succeeds.
  if (error) {
    return;
  }     
  utc = doc["utc"];
  r = doc["r"];
  g = doc["g"];
  b = doc["b"];
  mode = doc["mode"];
  state = doc["state"];
  lvl = doc["lvl"];
  hp = doc["hp"];
  ac = doc["ac"];
  xp = doc["xp"]; 
  msg = doc["msg"];
  updateScreen = true;
}
