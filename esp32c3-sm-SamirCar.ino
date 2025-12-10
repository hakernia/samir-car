#include <WiFi.h>
#include <WebServer.h>

// --- Access Point ---
const char* apSSID = "SamirCar";
const char* apPassword = "12345678";

WebServer server(80);

// --- Stany kontrolek ---
bool leftOn = false;
bool rightOn = false;
bool hazardOn = false;
bool highOn = false;
bool lowOn = false;
bool brakeOn = false;

// --- Funkcje kontrolne ---
void leftIndicator()   { leftOn = !leftOn; Serial.println(leftOn ? "Kierunkowskaz lewy ON" : "OFF"); }
void rightIndicator()  { rightOn = !rightOn; Serial.println(rightOn ? "Kierunkowskaz prawy ON" : "OFF"); }
void hazardLights()    { hazardOn = !hazardOn; Serial.println(hazardOn ? "Światła awaryjne ON" : "OFF"); }
void highBeam()        { highOn = !highOn; Serial.println(highOn ? "Światła drogowe ON" : "OFF"); }
void lowBeam()         { lowOn = !lowOn; Serial.println(lowOn ? "Światła mijania ON" : "OFF"); }
void brakeLight()      { brakeOn = !brakeOn; Serial.println(brakeOn ? "Stop ON" : "OFF"); }

// --- Strona HTML z JS do odpytywania stanu ---
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>Kontrolki samochodu</title>
    <style>
      body { font-family: Arial; text-align: center; margin-top: 50px; }
      button { width: 150px; height: 50px; font-size: 16px; margin: 10px; }
      .active { background-color: yellow; }
    </style>
  </head>
  <body>
    <h1>Symulator kontrolek samochodu</h1>
    <button id="left" onclick="toggle('left')">Kierunkowskaz lewy</button>
    <button id="right" onclick="toggle('right')">Kierunkowskaz prawy</button>
    <br>
    <button id="hazard" onclick="toggle('hazard')">Światła awaryjne</button>
    <br>
    <button id="high" onclick="toggle('high')">Światła drogowe</button>
    <button id="low" onclick="toggle('low')">Światła mijania</button>
    <br>
    <button id="stop" onclick="toggle('stop')">Stop / Hamulec</button>

    <script>
      function toggle(control) {
        fetch('/' + control);
      }

      async function updateState() {
        try {
          const resp = await fetch('/state');
          const data = await resp.json();
          
          document.getElementById('left').className = data.left ? 'active' : '';
          document.getElementById('right').className = data.right ? 'active' : '';
          document.getElementById('hazard').className = data.hazard ? 'active' : '';
          document.getElementById('high').className = data.high ? 'active' : '';
          document.getElementById('low').className = data.low ? 'active' : '';
          document.getElementById('stop').className = data.brake ? 'active' : '';
        } catch (e) { console.log(e); }
      }

      setInterval(updateState, 300); // odpytywanie co 300ms
      updateState();
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// --- API zwracające stan kontrolek w JSON ---
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

// --- Mapowanie ścieżek na funkcje ---
void setupRoutes() {
  server.on("/", handleRoot);
  server.on("/left", [](){ leftIndicator(); server.send(200,"text/plain","OK"); });
  server.on("/right", [](){ rightIndicator(); server.send(200,"text/plain","OK"); });
  server.on("/hazard", [](){ hazardLights(); server.send(200,"text/plain","OK"); });
  server.on("/high", [](){ highBeam(); server.send(200,"text/plain","OK"); });
  server.on("/low", [](){ lowBeam(); server.send(200,"text/plain","OK"); });
  server.on("/stop", [](){ brakeLight(); server.send(200,"text/plain","OK"); });
  server.on("/state", handleState); // JSON ze stanem kontrolek
}

void setup() {
  Serial.begin(115200);

  // --- Uruchom AP ---
  WiFi.softAP(apSSID, apPassword);
  Serial.println("AP uruchomiony!");
  Serial.print("IP AP: ");
  Serial.println(WiFi.softAPIP());

  setupRoutes();
  server.begin();
}

void loop() {
  server.handleClient();
}
