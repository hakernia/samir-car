#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

/*
 * This version is for LED strip RGBIC, 90 LEDs/m, 1200 lm, 24W, 24V
 * It consists of 25 sections x 20 cm
 * Each section has 6 x WS2812B + 6 x cold white + 6 x warm white
 * Sections are addressable as follows:
 * section 1:
 *     leds[0] = 6 x WS2812B = CRGB(R,G,B)
 *     leds[1] = 6 x (cold + warm) = CRGB(cold, warm, notused)
 * section 2:
 *     leds[2] = 6 x WS2812B = CRGB(R,G,B)
 *     leds[3] = 6 x (cold + warm) = CRGB(cold, warm, notused)
 * etc.
 */

// --- Access Point ---
const char* apSSID = "SamirCar";
const char* apPassword = "12345678";

WebServer server(80);

// --- Stany kontrolek ---
bool leftRequested = false;
bool rightRequested = false;
bool hazardRequested = false;
bool highOn = false;
bool lowOn = false;
bool brakeOn = false;

int testLedNum = 0;  // 0..NUM_LEDS-1
int testColor = 0;   // 0-black,1-red 2-green,3-blue,4-white

// --- Stany wyjściowe (mruganie) ---
bool leftOn = false;
bool rightOn = false;
bool hazardOn = false;

// --- Mruganie ---
unsigned long lastBlink = 0;
const unsigned long blinkInterval = 500; // 500ms = 1Hz




#define NUM_LEDS 32
#define LED_PIN  4   // bezpieczny pin na C3

#define LEFT_REAR_START   0
#define RIGHT_REAR_START  9
#define DIR_REAR_SIZE     2
#define BRAKE_REAR_START  2
#define BRAKE_REAR_SIZE   7

#define LEFT_FRONT_START  11
#define RIGHT_FRONT_START 13
#define LIGHT_SIZE        2

#define EVEN  1
#define ODD   2

#define TURN_COLOR    CRGB(255, 40, 0)   // pomarańcz
#define TAIL_COLOR    CRGB(60, 0, 0)      // ciemna czerwień
#define BRAKE_COLOR   CRGB(255, 0, 0)     // stop
#define LOW_BEAM_COLOR    CRGB(0, 100, 180) // przód (opcjonalnie)
#define HIGH_BEAM_COLOR    CRGB(180, 0, 0) // przód (opcjonalnie)

CRGB leds[NUM_LEDS];

bool blinkState = false;

void updateBlink() {
  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    blinkState = !blinkState;
  }
}
void resetBlink() {
  lastBlink = millis();
  blinkState = true;
}

void fillLight(int start, int size, int evenOddAll, CRGB color) {
  if(evenOddAll & 1)
  for (int i = 0; i < size; i++) {
    leds[start*2 + i*2] = color;
  }
  if(evenOddAll & 2)
  for (int i = 0; i < size; i++) {
    leds[start*2+1 + i*2] = color;
  }
}

void drawRearLights() {
  // pozycja
  CRGB frontColor;
  if(lowOn)
    frontColor = CRGB(highOn ? 100 : 0, lowOn ? 100 : 0, 0);
  fillLight(LEFT_FRONT_START,  LIGHT_SIZE, ODD, frontColor);
  fillLight(RIGHT_FRONT_START,  LIGHT_SIZE, ODD, frontColor);

  if (lowOn || highOn ) {
    fillLight(BRAKE_REAR_START,  BRAKE_REAR_SIZE, EVEN, TAIL_COLOR);
  }
  else {
    fillLight(BRAKE_REAR_START,  BRAKE_REAR_SIZE, EVEN, 0);
  }

  // stop = jaśniej
  if (brakeOn) {
    fillLight(BRAKE_REAR_START,  BRAKE_REAR_SIZE, EVEN, BRAKE_COLOR);
  }

  // kierunki / awaryjne
  if (blinkState) {
    if (hazardRequested || leftRequested)
      fillLight(LEFT_REAR_START, DIR_REAR_SIZE, EVEN, TURN_COLOR);

    if (hazardRequested || rightRequested)
      fillLight(RIGHT_REAR_START, DIR_REAR_SIZE, EVEN, TURN_COLOR);
  }
/*
  if(testLedNum >= 0) {
    CRGB testColorRGB;
    switch(testColor) {
      case 0: testColorRGB = CRGB(0, 0, 0); break;
      case 1: testColorRGB = CRGB(100, 0, 0); break;
      case 2: testColorRGB = CRGB(0, 100, 0); break;
      case 3: testColorRGB = CRGB(0, 0, 100); break;
      case 4: testColorRGB = CRGB(100, 100, 100); break;
    }
    fillLight(testLedNum, 1, EVEN, testColorRGB);
  }
  */
}

void updateLights() {
  FastLED.clear();

  updateBlink();
  drawRearLights();
  //drawFrontLights();   // możesz wykomentować

  FastLED.show();
}

void updateLights_min() {
  //FastLED_min<LED_PIN>.clear(); 

  updateBlink();
  drawRearLights();
  //drawFrontLights();   // możesz wykomentować

  //FastLED_min<LED_PIN>.show(); 
}


void LedsSetup() {
  //FASTLED_MIN_SETUP(LED_PIN, leds, NUM_LEDS);
  //FastLED_min<LED_PIN>.setBrightness(70);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS); 

  //FastLED.setExclusiveDriver("RMT");

  for(int ii=0; ii<NUM_LEDS;ii++) {
    leds[ii] = CRGB(255, ii*5, 0);
  }
  //FastLED_min<LED_PIN>.show();
  FastLED.show();
  delay(1000);
  for(int ii=0; ii<NUM_LEDS;ii++) {
    leds[ii] = CRGB(0, 255, 0);
  }
  //FastLED_min<LED_PIN>.show(); 
  FastLED.show();
  delay(1000);
  for(int ii=0; ii<NUM_LEDS;ii++) {
    leds[ii] = CRGB(0, 0, 255);
  }
  //FastLED_min<LED_PIN>.show();
  FastLED.show();
  delay(1000);
}


// --- Funkcje toggle ---
void toggleLeft()   { leftRequested = !leftRequested; rightRequested = false; resetBlink(); Serial.println(leftRequested ? "Kierunkowskaz lewy WŁ." : "WYŁ."); }
void toggleRight()  { rightRequested = !rightRequested; leftRequested = false; resetBlink(); Serial.println(rightRequested ? "Kierunkowskaz prawy WŁ." : "WYŁ."); }
void toggleHazard() { hazardRequested = !hazardRequested; resetBlink(); Serial.println(hazardRequested ? "Światła awaryjne WŁ." : "WYŁ."); }
void toggleHigh()   { highOn = !highOn; Serial.println(highOn ? "Światła drogowe WŁ." : "WYŁ."); }
//void toggleHigh()   { testLedNum++; if(testLedNum == NUM_LEDS) testLedNum = 0; Serial.print("testLedNum: "); Serial.println(testLedNum);}
void toggleLow()    { lowOn = !lowOn; if(lowOn && highOn) highOn = false; Serial.println(lowOn ? "Światła mijania WŁ." : "WYŁ."); }
//void toggleLow()    { testColor++; if(testColor == 5) testColor = 0; Serial.print("testColor: "); Serial.println(testColor);}
void toggleBrake()  { brakeOn = !brakeOn; Serial.println(brakeOn ? "Stop ON" : "OFF"); }

// --- Strona HTML ---
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<style>
  body {
    margin: 0;
    background: #111;
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100vh;
  }

  .panel {
    display: flex;
    flex-direction: column;
    gap: 24px;
    align-items: center;
  }

  .row {
    display: flex;
    gap: 20px;
    justify-content: center;
  }

  .title {
    font-size: 50px;
    color: antiquewhite;
  }

  button {
    background: #222;
    border: none;
    border-radius: 12px;
    padding: 12px;
    width: 72px;
    height: 72px;
    display: flex;
    justify-content: center;
    align-items: center;
  }

  svg {
    width: 48px;
    height: 48px;
    fill: #000;
  }

  /* ===== PODŚWIETLENIA ===== */

  .on-left svg,
  .on-right svg {
    fill: #00ff66;
  }

  .on-hazard svg {
    fill: #ff3333;
  }

  .on-low svg {
    fill: #00ff66;
  }

  .on-high svg {
    fill: #3399ff;
  }

  .on-stop svg {
    fill: #ff3333;
  }
</style>
</head>

<body>
<div class="panel">
  <div class="row title">
  SamirCar
  </div>
  <!-- RZĄD 1: kierunki + awaryjne -->
  <div class="row">
    <button id="left" class="on-left" onclick="send('left')">
      <!-- LEWY -->
      <svg viewBox="0 0 100 100">
        <polygon points="65,10 15,50 65,90"/>
      </svg>
    </button>

    <button id="hazard" onclick="send('hazard')">
      <!-- AWARYJNE -->
      <svg viewBox="0 0 100 100">
        <polygon points="50,10 90,90 10,90"/>
      </svg>
    </button>

    <button id="right" onclick="send('right')">
      <!-- PRAWY -->
      <svg viewBox="0 0 100 100">
        <polygon points="35,10 85,50 35,90"/>
      </svg>
    </button>
  </div>

  <!-- RZĄD 2: światła -->
  <div class="row">
    <button id="low" onclick="send('low')">
      <!-- MIJANIA -->
      <svg viewBox="0 0 100 100">
        <path d="M20 30 H55 L75 50 L55 70 H20 Z"/>
        <line x1="60" y1="35" x2="85" y2="30" stroke-width="6"/>
        <line x1="60" y1="50" x2="85" y2="50" stroke-width="6"/>
        <line x1="60" y1="65" x2="85" y2="70" stroke-width="6"/>
      </svg>
    </button>

    <button id="high" onclick="send('high')">
      <!-- DROGOWE -->
      <svg viewBox="0 0 100 100">
        <path d="M20 30 H55 L75 50 L55 70 H20 Z"/>
        <line x1="60" y1="35" x2="85" y2="35" stroke-width="6"/>
        <line x1="60" y1="50" x2="85" y2="50" stroke-width="6"/>
        <line x1="60" y1="65" x2="85" y2="65" stroke-width="6"/>
      </svg>
    </button>
  </div>

  <!-- RZĄD 3: STOP -->
  <div class="row">
    <button id="stop" onclick="send('stop')">
      <svg viewBox="0 0 100 100">
        <polygon points="
          30,5 70,5
          95,30 95,70
          70,95 30,95
          5,70 5,30
        "/>
      </svg>
    </button>
  </div>

    <script>
      let state = {};
      let blinkPhase = false;

      function send(control) {
        fetch('/' + control);
      }

      async function fetchState() {
        try {
          const resp = await fetch('/state');
          state = await resp.json();

          //document.getElementById('left').className = state.left ? 'on-left' : 'left';
          //document.getElementById('right').className = state.right ? 'on-right' : 'right';
          //document.getElementById('hazard').className = state.hazard ? 'on-hazard' : 'hazard';
          //document.getElementById('high').className = state.high ? 'on-high' : 'high';
          //document.getElementById('low').className = state.low ? 'on-low' : 'low';
          //document.getElementById('stop').className = state.brake ? 'on-stop' : 'stop';
        } catch(e){ console.log(e); }
      }

      function render() {
        setSolid('left', state.left || false);
        setSolid('right', state.right || false);
        setSolid('hazard', state.hazard || false);

        setSolid('low', state.low || false);
        setSolid('high', state.high || false);
        setSolid('stop', state.brake || false);
      }

      function setBlink(id, enabled) {
        const el = document.getElementById(id);
        el.classList.toggle('on-' + id, enabled && blinkPhase);
      }

      function setSolid(id, enabled) {
        const el = document.getElementById(id);
        el.classList.toggle('on-' + id, enabled);
      }


      setInterval(fetchState, 50);
      fetchState();

      setInterval(() => {
        blinkPhase = !blinkPhase;
        render();
      }, 50);
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html; charset=UTF-8", html);
}

// --- API JSON ---
void handleState() {
  String json = "{";
  json += "\"left\":" + String(leftRequested && blinkState ? "true" : "false") + ",";
  json += "\"right\":" + String(rightRequested && blinkState ? "true" : "false") + ",";
  json += "\"hazard\":" + String(hazardRequested && blinkState ? "true" : "false") + ",";
  json += "\"high\":" + String(highOn ? "true" : "false") + ",";
  json += "\"low\":" + String(lowOn ? "true" : "false") + ",";
  json += "\"brake\":" + String(brakeOn ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

// --- Mapowanie tras ---
void setupRoutes() {
  server.on("/", handleRoot);
  server.on("/left", [](){ toggleLeft(); server.send(200,"text/plain","OK"); });
  server.on("/right", [](){ toggleRight(); server.send(200,"text/plain","OK"); });
  server.on("/hazard", [](){ toggleHazard(); server.send(200,"text/plain","OK"); });
  server.on("/high", [](){ toggleHigh(); server.send(200,"text/plain","OK"); });
  server.on("/low", [](){ toggleLow(); server.send(200,"text/plain","OK"); });
  server.on("/stop", [](){ toggleBrake(); server.send(200,"text/plain","OK"); });
  server.on("/state", handleState);
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(apSSID, apPassword);
  Serial.println("AP uruchomiony!");
  Serial.print("IP AP: "); Serial.println(WiFi.softAPIP());

  setupRoutes();
  server.begin();

  LedsSetup();
}

void loop() {
  server.handleClient();

  // --- Miganie ---
  // unsigned long now = millis();
  // if (now - lastBlink >= blinkInterval) {
  //   lastBlink = now;

    static unsigned long last = 0;
    if (millis() - last > 30) {
      last = millis();
      updateLights();
    }

}
