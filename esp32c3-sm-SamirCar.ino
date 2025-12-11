#include <WiFi.h>
#include <WebServer.h>

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

// --- Stany wyjściowe (mruganie) ---
bool leftOn = false;
bool rightOn = false;
bool hazardOn = false;

// --- Mruganie ---
unsigned long lastBlink = 0;
const unsigned long blinkInterval = 500; // 500ms = 1Hz

// --- Funkcje toggle ---
void toggleLeft()   { leftRequested = !leftRequested; Serial.println(leftRequested ? "Kierunkowskaz lewy WŁ." : "WYŁ."); }
void toggleRight()  { rightRequested = !rightRequested; Serial.println(rightRequested ? "Kierunkowskaz prawy WŁ." : "WYŁ."); }
void toggleHazard() { hazardRequested = !hazardRequested; Serial.println(hazardRequested ? "Światła awaryjne WŁ." : "WYŁ."); }
void toggleHigh()   { highOn = !highOn; Serial.println(highOn ? "Światła drogowe WŁ." : "WYŁ."); }
void toggleLow()    { lowOn = !lowOn; Serial.println(lowOn ? "Światła mijania WŁ." : "WYŁ."); }
void toggleBrake()  { brakeOn = !brakeOn; Serial.println(brakeOn ? "Stop ON" : "OFF"); }

// --- Strona HTML ---
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <title>Mini kokpit samochodu</title>
    <style>
      body { font-family: "Arial", "Verdana", "Tahoma", "Sans-Serif"; }
      button {
        width: 160px; height: 60px; font-size: 16px; margin: 10px; border: none; color: white; cursor: pointer;
        border-radius: 8px;
      }
      .left { background-color: gray; }
      .right { background-color: gray; }
      .hazard { background-color: gray; }
      .high { background-color: gray; }
      .low { background-color: gray; }
      .stop { background-color: gray; }
      .active-left { background-color: orange; animation: blink 1s infinite; }
      .active-right { background-color: orange; animation: blink 1s infinite; }
      .active-hazard { background-color: orange; animation: blink 1s infinite; }
      .active-high { background-color: white; color: black; }
      .active-low { background-color: lightgray; color: black; }
      .active-stop { background-color: red; }
      @keyframes blink { 50% { opacity: 0; } 100% { opacity: 1; } }
    </style>
  </head>
  <body>
    <h1>Mini kokpit samochodu</h1>
    <button id="left" class="left" onclick="toggle('left')">Kierunkowskaz lewy</button>
    <button id="right" class="right" onclick="toggle('right')">Kierunkowskaz prawy</button>
    <br>
    <button id="hazard" class="hazard" onclick="toggle('hazard')">Światła awaryjne</button>
    <br>
    <button id="high" class="high" onclick="toggle('high')">Światła drogowe</button>
    <button id="low" class="low" onclick="toggle('low')">Światła mijania</button>
    <br>
    <button id="stop" class="stop" onclick="toggle('stop')">Stop / Hamulec</button>

    <script>
      function toggle(control) {
        fetch('/' + control);
      }

      async function updateState() {
        try {
          const resp = await fetch('/state');
          const data = await resp.json();

          document.getElementById('left').className = data.left ? 'active-left' : 'left';
          document.getElementById('right').className = data.right ? 'active-right' : 'right';
          document.getElementById('hazard').className = data.hazard ? 'active-hazard' : 'hazard';
          document.getElementById('high').className = data.high ? 'active-high' : 'high';
          document.getElementById('low').className = data.low ? 'active-low' : 'low';
          document.getElementById('stop').className = data.brake ? 'active-stop' : 'stop';
        } catch(e){ console.log(e); }
      }

      setInterval(updateState, 200);
      updateState();
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html; charset=UTF-8", html);
}

// --- API JSON ---
void handleState() {
  String json = "{";
  json += "\"left\":" + String(leftOn ? "true" : "false") + ",";
  json += "\"right\":" + String(rightOn ? "true" : "false") + ",";
  json += "\"hazard\":" + String(hazardOn ? "true" : "false") + ",";
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
}

void loop() {
  server.handleClient();

  // --- Miganie ---
  unsigned long now = millis();
  if (now - lastBlink >= blinkInterval) {
    lastBlink = now;

    if (hazardRequested) {
      leftOn = !leftOn;
      rightOn = leftOn;
      hazardOn = leftOn;
    } else {
      hazardOn = false;
      if (leftRequested) leftOn = !leftOn; else leftOn = false;
      if (rightRequested) rightOn = !rightOn; else rightOn = false;
    }
  }
}
