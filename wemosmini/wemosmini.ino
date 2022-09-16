
// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "index_html.h"

#define MIN_MOTOR_VALUE 130
#define MAX_PWM_VAL 255
#define PWMA 0  //Right side 
#define PWMB 13  //Left side 

// Motor A connections
int enA = 0;
int in1 = 5;
int in2 = 4;
// Motor B connections
int enB = 13;
int in3 = 12;
int in4 = 14;

// Replace with your network credentials
const char* ssid = "YourWiFi";
const char* password = "YourWifiPassword";

int x = 0;
int y = 0;
float z = 0;  //magnitude


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void notifyClients() {
  ws.textAll(String("somevalue"));
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
  // Set all the motor control pins to outputs
  pinMode(enA, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  
  motorsstop();

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

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Start server
  server.begin();
}

void loop() {
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

    analogWrite(PWMA, left);
    analogWrite(PWMB, right);
    motor1forward();
    motor2forward();
  
  } else  if (y < -30) { //backward
    motor1reverse();
    motor2reverse();
  } else {
    digitalWrite(PWMA, LOW);
    digitalWrite(PWMB, LOW);
  }
}

// This function lets you control spinning direction of motors
void directionControl() {
  // Set motors to maximum speed
  // For PWM maximum possible values are 0 to 255
  analogWrite(enA, 255);
  analogWrite(enB, 255);

  // Turn on motor A & B
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  delay(2000);
  
  // Now change motor directions
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  delay(2000);
  
  // Turn off motors
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

// This function lets you control speed of the motors
void speedControl() {
  // Turn on motors
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  
  // Accelerate from zero to maximum speed
  for (int i = 0; i < 256; i++) {
    analogWrite(enA, i);
    analogWrite(enB, i);
    delay(20);
  }
  
  // Decelerate from maximum speed to zero
  for (int i = 255; i >= 0; --i) {
    analogWrite(enA, i);
    analogWrite(enB, i);
    delay(20);
  }
  
  // Now turn off motors
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}
