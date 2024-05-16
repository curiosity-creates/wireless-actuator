
#include "AS5600.h"
#include "Wire.h"

AS5600L as5600;   //  use default Wire

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AccelStepper.h>

// Stepper Motor Settings
#define motorInterfaceType 1

const int DIR = 12;
const int STEP = 14;
// const int  steps_per_rev = 200;

AccelStepper motor(motorInterfaceType, STEP, DIR);


// Replace with your network credentials
const char* ssid = "****";
const char* password = "****";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameters in HTTP POST request
const char* PARAM_INPUT_1 = "direction";
const char* PARAM_INPUT_2 = "steps";

// Variables to save values from HTML form
String direction;
String steps;

// Variable to detect whether a new request occurred
bool newRequest = false;

// HTML to build the web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Stepper Motor and Sensor Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function refreshSensorValue() {
      fetch('/sensor')
        .then(response => response.text())
        .then(data => {
          document.getElementById('sensorValue').innerText = data;
        })
        .catch(error => console.error('Error fetching sensor data:', error));
    }
    document.addEventListener('DOMContentLoaded', function() {
      refreshSensorValue(); // Refresh immediately on page load
      setInterval(refreshSensorValue, 50); // Refresh every 50 milliseconds (0.05 seconds)
    });
  </script>
</head>
<body>
  <h1>Stepper Motor Control</h1>
  <form action="/" method="POST">
    <input type="radio" name="direction" value="CW" checked>
    <label for="CW">Clockwise</label>
    <input type="radio" name="direction" value="CCW">
    <label for="CW">Counterclockwise</label><br><br><br>
    <label for="steps">Number of steps:</label>
    <input type="number" name="steps">
    <input type="submit" value="GO!">
  </form>
  <h2>Angle</h2>
  <p id="sensorValue">Loading...</p>
  <!-- Removed the refresh button since it's no longer necessary -->
</body>
</html>
)rawliteral";



// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}



void setup() {
  Serial.begin(115200);

  initWiFi();

  motor.setMaxSpeed(1000);
  motor.setAcceleration(150);
  motor.setSpeed(500);

  Serial.println(__FILE__);
  Serial.print("AS5600_LIB_VERSION: ");
  Serial.println(AS5600_LIB_VERSION);

  Serial.println(as5600.getAddress());
  as5600.setAddress(0x36);
  Serial.println(as5600.getAddress());
  Wire.begin(15, 16); //15=SDA, 16=SCL

  as5600.begin();  //  set direction pin.
  as5600.setDirection(AS5600_CLOCK_WISE);  //  default, just be explicit.
  
  int b = as5600.isConnected();
  Serial.print("Connect: ");
  Serial.println(b);
  delay(1000);

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });
  
  // Handle request (form)
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        // HTTP POST input1 value (direction)
        if (p->name() == PARAM_INPUT_1) {
          direction = p->value().c_str();
          Serial.print("Direction set to: ");
          Serial.println(direction);
        }
        // HTTP POST input2 value (steps)
        if (p->name() == PARAM_INPUT_2) {
          steps = p->value().c_str();
          Serial.print("Number of steps set to: ");
          Serial.println(steps);
        }
      }
    }
    request->send(200, "text/html", index_html);
    newRequest = true;
  });

  // Sensor Value URL
  server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request){
    float sensorValue = as5600.readAngle(); // Assuming this function returns your sensor value
    char str[16];
    dtostrf(sensorValue/10, 1, 2, str); // Convert float to string with 2 decimal places
    request->send(200, "text/plain", str);
  });


  


  server.begin();
}

void loop() {

  if (newRequest){
    if (direction == "CW"){
      // Spin the stepper clockwise direction
      motor.moveTo(steps.toInt());
    }
    else{
      // Spin the stepper counterclockwise direction
      motor.moveTo(-steps.toInt());
    }
    newRequest = false;
  }
  motor.run();
}

