
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

#define MAX_PWM_VAL 255
#define MIN_MOTOR_VALUE 130

// Motor A connections
int PWMA = 0;
int in1 = 5;
int in2 = 4;

// Motor B connections
int PWMB = 13;
int in3 = 12;
int in4 = 14;

// Replace with your network credentials
const char* ssid = "YOURSID";
const char* password = "YOURPWD";

int x = 0;
int y = 0;
float z = 0;  //magnitude


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void notifyClients() {
  ws.textAll(String("Hello World!"));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    
//// For debugging
//    Serial.print("Incoming data: ");
//    Serial.println((char*)data);

    String tmp = (char*)data;
    int index = tmp.indexOf(",");
    if (index > -1) {
      x = tmp.substring(0, index).toInt();
      y = tmp.substring(index + 1, tmp.length()).toInt();
      z = sqrt(x*x + y*y);
      //// For debugging
      // Serial.println(x);
      // Serial.println(y);
      // Serial.println(z);
    }
    
//// Send data back to the client
//    if (strcmp((char*)data, "toggle") == 0) {
//      ledState = !ledState;
//      notifyClients();
//    }
    calc();
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  return "";
}

void motor1forward() {
  digitalWrite(in1, HIGH); //MOTOR1 - in1 (HIGH) in2 (LOW): Forward 
  digitalWrite(in2, LOW); //MOTOR1 - in1 (LOW) in2 (HIGH): Reverse
}

void motor2forward() {
  digitalWrite(in3, HIGH); //MOTOR2 - in3 (HIGH) in2 (LOW): Forward
  digitalWrite(in4, LOW); //MOTOR2 - in3 (LOW) in2 (HIGH): Revers
}

void motor1reverse() {
  digitalWrite(in1, LOW); //MOTOR1 - in1 (HIGH) in2 (LOW): Forward 
  digitalWrite(in2, HIGH); //MOTOR1 - in1 (LOW) in2 (HIGH): Reverse
}

void motor2reverse() {
  digitalWrite(in3, LOW); //MOTOR2 - in3 (HIGH) in2 (LOW): Forward
  digitalWrite(in4, HIGH); //MOTOR2 - in3 (LOW) in2 (HIGH): Revers
}

void motorsstop() {
    // Turn off motors - Initial state (ALL LOW)
  digitalWrite(in1, LOW); //MOTOR1 - in1 (HIGH) in2 (LOW): Forward 
  digitalWrite(in2, LOW); //MOTOR1 - in1 (LOW) in2 (HIGH): Reverse
  digitalWrite(in3, LOW); //MOTOR2 - in3 (HIGH) in2 (LOW): Forward
  digitalWrite(in4, LOW); //MOTOR2 - in3 (LOW) in2 (HIGH): Reverse
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  
  // Set all the motor control pins to outputs
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  
  motorsstop();

// Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
// Initialize LittleFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  // Open a web 
  Serial.print("Open a web browser to http://");
  Serial.println(WiFi.localIP());

  initWebSocket();

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.ico", "image/png");
  });

  // Route for root / web page
  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/hello.html", String(), false, processor);
  });

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Start server
  server.begin();
}

void calc() {
  int right = 0;
  int left = 0;
  ws.cleanupClients();
  if (y > 30) { //forward

    if ( x >= 0){
      left = (int)(z/100.0 * 255);
      right = (int)(z/100.0 * (100.0 - (float)x)/100.0 * MAX_PWM_VAL);
      if (right < MIN_MOTOR_VALUE) { // prevents from turning too sharp
        right = MIN_MOTOR_VALUE;
      }
    } else {
      left = (int)(z/100.0 * (100.0 + (float)x)/100.0 * MAX_PWM_VAL);
      if (left < MIN_MOTOR_VALUE) { // prevents from turning too sharp
        left = MIN_MOTOR_VALUE;
      }
      right = (int)(z/100.0 * 255);
    }

    motor1forward();
    motor2forward();
  
  } else  if (y < -30) { //backward
    right = MAX_PWM_VAL;
    left = MAX_PWM_VAL;
    motor1reverse();
    motor2reverse();

  } else {
    right = 0;
    left = 0;
    digitalWrite(PWMA, LOW);
    digitalWrite(PWMB, LOW);
  }

  analogWrite(PWMA, left);
  analogWrite(PWMB, right);  
}


void loop() {
  ArduinoOTA.handle();
}
