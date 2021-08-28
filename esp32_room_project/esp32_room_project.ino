/*
  Title  : ESP32 ROOM PROJECT
  Contact: info@tatco.cc
  Done By: TATCO Inc.
  Skecth on Github : https://github.com/rabee2050/esp32-room-project
  Youtube: http://tatco.cc

  Apps:
  iOS    :https://apps.apple.com/us/app/esp32-kit/id1498601142


  Installation instructions using Arduino IDE Boards Manager:
  1- To setup ESP32 with Arduino IDE, Go to this link and follow the instructions:
  https://github.com/espressif/arduino-esp32/blob/master/README.md
  2- Go to Tools/Board/Boards Manager
  3- Search for: ESP32
  4- Install the Package: esp32 by Espress Systems version 1.0.4
  5- Go to Sketch/Include Library/Manage Libriries
  6- Search for: dht sensor library. with all dependency libraries
  7- Go to Tools/Board then select ESP32 Dev Module
  8- Go to Tools/Partition Scheme then select Huge App(3MB No OTA / 1MB SPIFFS)
  9- Update the below WIFI creditntial to your WIFI ssid and password.
  10- That's it, Now you can upload the sketch to your board.

  Connection:
  - Check the wiring diagram on the youtube channel

*/

#include <WiFi.h>
#include "DHT.h"//Temp and Humdidity Library

#define DHTPIN 15     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT11     // there are multiple kinds of DHT sensors
DHT dht(DHTPIN, DHTTYPE);

const char* ssid     = "test";//Must be changed to Your WIFI SSID Name
const char* password = "1231231234";//Must be changed to Your WIFI Password
WiFiServer server(80);
String httpAppJsonOk = "HTTP/1.1 200 OK\r\n content-type:application/json \r\n\r\n";
String httpTextPlainOk = "HTTP/1.1 200 OK\r\n content-type:text/plain \r\n\r\n";
unsigned long serialTimer = millis();

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "ffe0"
#define CHARACTERISTIC_UUID "ffe1"
BLECharacteristic *pCharacteristic;



String appBuildVersion = "1.1";
String boardType = "esp32";
String protectionPassword = ""; //This will prevent people to add or control your board.
int refreshTime = 3; //the data will be updated on the app every 3 seconds.
#define lcdSize 3 //this will define number of virtual LCD display on the app LCD Dashboard tab.
String feedBack ;

char pinsMode[54];
int pinsValue[54];
String lcd[lcdSize];

unsigned long last = millis();
bool deviceConnected = false;
unsigned long lastbutton = millis();
unsigned long sensorTimer = millis();

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Bluetooth is Connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Bluetooth is Disconnected");
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Waiting the app connection to notify...");


  for (byte i = 0; i <= 16; i++) {//Init for PWM & Servo
    ledcSetup(i, 50, 8);
  }

  BLEDevice::init("ESP32-ROOM");//Board BLE Name
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());

  class MyCallbacks: public BLECharacteristicCallbacks {
      void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
          String dataIncoming = "";
          for (int i = 0; i < value.length(); i++) {
            dataIncoming += value[i];
          }
          int commaIndex = dataIncoming.indexOf('/');
          int secondCommaIndex = dataIncoming.indexOf('/', commaIndex + 1);
          int thirdCommaIndex = dataIncoming.indexOf('/', secondCommaIndex + 1);

          String first = dataIncoming.substring(0, commaIndex);//Commands like: mode, digital, analog, servo,etc...
          String second = dataIncoming.substring(commaIndex + 1, secondCommaIndex);//Pin number.
          String third = dataIncoming.substring(secondCommaIndex + 1, thirdCommaIndex );// value of the pin.
          String forth = dataIncoming.substring(thirdCommaIndex + 1 );//not used,it is for future provision.
          process(first, second, third, forth);
          delay(50);
        }
      }
  };

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  //  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();

  //*******************************************
  // We start by connecting to a WiFi network
  Serial.println();
  WiFi.mode(WIFI_AP_STA);//WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF
  WiFi.softAP("ESP32-WIFI-AP", "123456789"); //(APname, password)
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.printf("Connecting to %s........", ssid);
  Serial.println("");

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    notConnectedToWifiNetwork();
  } else {
    connectedToWifiNetwork();
  }

  server.begin();
  //*******************************************
  dht.begin();
  boardInit();
}

void loop() {
  checkButtons();
  updateLcdValues();
  updateInputValues();//if it is input then update pin value.
  if (deviceConnected) {
    updateApp();//Send data to mobile app every specific time
  }
  //*****************************
  serialPrintIpAddress();
  WiFiClient client = server.available();

  if (!client) {
    return;
  }
  while (!client.available()) {
    delay(1);
  }
  process_WIFI(client);
  //******************************
}

void updateLcdValues() {

  if (millis() - sensorTimer > 2000) {
    // Read Humidity as % (the default)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();


    lcd[0] = String(h) + "%"; // Send any data to the virtual LCD dashboard in the App.
    lcd[1] = String(t) + "°"; // Send analog value of A0 to the virtual LCD dashboard in the App.
    lcd[2] = random(1, 100);// Send any data to the virtual LCD dashboard in the App.
    sensorTimer = millis();
  }
}

void process(String command, String second, String third, String forth) {

  if (command == "terminal") {//to recieve data from mobile app from terminal text box
    terminalCommand(second);
  }

  if (command == "digital") {//to turn pins on or off
    digitalCommand(second, third);
  }

  if (command == "pwm") {//to write analog value(PWM).
    pwmCommand(second, third);
  }

  if (command == "mode") {//to chang the mode of the pin.
    modeCommand(second, third);
  }

  if (command == "servo") {// to control servo(0°-180°).
    servoCommand(second, third);
  }

  if (command == "allonoff") {//to turn all pins on or off.
    allonoff(second, third);
  }
  if (command == "refresh") {// to change the refresh time.
    refresh(second);
  }

  if (command == "allstatus") {// send JSON object arduino includes all data.
    feedBack = "refresh/";
    allstatus();
  }
}

void terminalCommand(String second) {//Here you recieve data form app terminal
  String data = second;
  Serial.println(data);

  String replyToApp = "Ok from Arduino"; //It can be change to any thing

  if (data.equalsIgnoreCase( "Ip") ) {
    feedBack = "terminal/IP Addr: ";
    feedBack += WiFi.localIP().toString().c_str();
    allstatus();
  }

  else if (data.equalsIgnoreCase("Rest")) {
    feedBack = "terminal/RESTARTED";
    allstatus();
    ESP.restart();
  }

  else {
    feedBack = "terminal/" + replyToApp; //dont change this line.
    allstatus();
  }


}

void digitalCommand(String second, String third) {
  int pin, value;
  pin = second.toInt();
  value = third.toInt();

  digitalWrite(pin, value);
  pinsValue[pin] = value;

}

void pwmCommand(String second, String third) {
  int pin, value;
  pin = second.toInt();
  value = third.toInt();
  ledcWrite(pin, value);
  pinsValue[pin] = value;
}

void servoCommand(String second, String third) {
  int pin, value;
  pin = second.toInt();
  value = third.toInt();
  int valueMapped = map(value, 0, 180, 10, 32);
  ledcWrite(pin, valueMapped);
  pinsValue[pin] = value;
}

void modeCommand(String second, String third ) {
  int pin = second.toInt();
  String mode = third;

  if (mode != "pwm") {
    ledcDetachPin(pin);
  };

  if (mode != "servo") {
    ledcDetachPin(pin);
  };

  if (mode == "output") {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
    pinsMode[pin] = 'o';
    pinsValue[pin] = 0;
  }
  if (mode == "push") {
    pinsMode[pin] = 'm';
    pinsValue[pin] = 0;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
  }
  if (mode == "schedule") {
    pinsMode[pin] = 'c';
    pinsValue[pin] = 0;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
  }

  if (mode == "input") {
    pinsMode[pin] = 'i';
    pinsValue[pin] = 0;
    pinMode(pin, INPUT);
  }

  if (mode == "pwm") {
    pinsMode[pin] = 'p';
    pinsValue[pin] = 0;
    ledcAttachPin(pin, pin);
    ledcWrite(pin, 0);
  }

  if (mode == "servo") {
    pinsMode[pin] = 's';
    pinsValue[pin] = 0;
    ledcAttachPin(pin, pin);
    ledcWrite(pin, 0);
  }

  feedBack = "mode/" + mode + "/" + pin + "/" + pinsValue[pin];
  allstatus();
}

void allonoff(String second, String third) {
  //  int pin, value;
  //  pin = second.toInt();
  int value = third.toInt();
  for (byte i = 0; i < sizeof(pinsMode); i++) {
    if (pinsMode[i] == 'o') {
      digitalWrite(i, value);
      pinsValue[i] = value;
    }
  }
}


void updateApp() {

  if (refreshTime != 0) {
    unsigned int refreshVal = refreshTime * 1000;
    if (millis() - last > refreshVal) {
      allstatus();
      last = millis();
    }
  }
}

void refresh(String second) {
  int value = second.toInt();
  refreshTime = value;
  allstatus();
}

void allstatus() {
  String dataResponse;
  dataResponse += "{";

  dataResponse += "\"m\":[";//m for mode
  for (byte i = 0; i <= 39; i++) {
    dataResponse += "\"";
    dataResponse += pinsMode[i];
    dataResponse += "\"";
    if (i != 39)dataResponse += ",";
  }
  dataResponse += "],";

  dataResponse += "\"v\":[";//v for value
  for (byte i = 0; i <= 39; i++) {
    dataResponse += pinsValue[i];
    if (i != 39)dataResponse += ",";
  }
  dataResponse += "],";

  dataResponse += "\"a\":[";//a for analog value
  for (byte i = 32; i <= 39; i++) {//(byte i = 32; i <= 39; i++)
    if (i == 37 || i == 38  ) {
    }
    else {
      dataResponse += analogRead(i);

      if (i != 39)dataResponse += ",";
    }
  }
  dataResponse += "],";

  dataResponse += "\"l\":[";//l for LCD value
  for (byte i = 0; i <= lcdSize - 1; i++) {
    dataResponse += "\"";
    dataResponse += lcd[i];
    dataResponse += "\"";
    if (i != lcdSize - 1)dataResponse += ",";
  }
  dataResponse += "],";

  dataResponse += "\"t\":\""; //t for Board Type .
  dataResponse += boardType;
  dataResponse += "\",";
  dataResponse += "\"f\":\""; //f for feedback .
  dataResponse += feedBack;
  dataResponse += "\",";
  dataResponse += "\"r\":\""; //r for refresh time .
  dataResponse += "3";
  dataResponse += "\",";
  dataResponse += "\"b\":\""; //b for app build version .
  dataResponse += appBuildVersion;
  dataResponse += "\",";
  dataResponse += "\"p\":\""; // p for Password.
  dataResponse += protectionPassword;
  dataResponse += "\"";
  dataResponse += "}";

  char dataBuffer[20];//Charachtristic hold 20byte per time.
  int dataResponseLength = dataResponse.length();
  for (int i = 0; i <= dataResponseLength; i = i + 20) {
    for (int x = 0; x < 20; x++) {
      dataBuffer[x] = dataResponse[i + x];
    }
    pCharacteristic->setValue((unsigned char*) dataBuffer, 20);// update the buffer
    pCharacteristic->notify(); // Send the value to the app!
  }
  pCharacteristic->setValue((unsigned char*) "\n", 2);
  pCharacteristic->notify(); // Send the value to the app!;
  feedBack = "";
}


void updateInputValues() {
  for ( unsigned int i = 0; i < sizeof(pinsMode); i++) {
    if (pinsMode[i] == 'i') {
      pinsValue[i] = digitalRead(i);
    }
  }
}

void boardInit() {
  for (byte i = 0 ; i <= 39; i++) {
    if (i == 2 || i == 4 || i == 5 || i == 18 ) {//Output
      pinsMode[i] = 'o';
      pinsValue[i] = 0;
      pinMode(i, OUTPUT);
    }
    else if (i == 12 || i == 13 || i == 14 || i == 26 ) { //Inputs
      pinsMode[i] = 'x';
      pinsValue[i] = 0;
      pinMode(i, INPUT_PULLDOWN);
    }
    else if (i == 32 || i == 33 || i == 34 || i == 35 || i == 36 || i == 39) {//analog inputs i == 32 || i == 33 || i == 34 || i == 35 ||
      pinsMode[i] = 'x';
      pinsValue[i] = 0;
      pinMode(i, INPUT);
    }

    else {
      pinsMode[i] = 'x';
      pinsValue[i] = 0;

    }
  }
}

//********************************************

void connectedToWifiNetwork() {
  Serial.println("");
  Serial.println("WiFi is connected, the IP address: ");
  Serial.println(WiFi.localIP());
}

void notConnectedToWifiNetwork() {
  WiFi.mode(WIFI_AP);//WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF
  WiFi.softAP("ESP32-WIFI-AP", "123456789"); //(APname, password)
  Serial.println("");
  Serial.printf("WiFi connection failed to the network '%s', but you can still connect to board as it is Access Point:", ssid);
  Serial.println("");
  Serial.println("1- Go to your mobile WiFi settings and connect to a network Called: 'ESP32-WIFI-AP'.");
  Serial.println("2- The default WiFi password is: '123456789'.");
  Serial.print("3- Open the ESP32 Kit app and insert this IP Address: ");
  Serial.println("192.168.4.1");
  Serial.println("4- Press the check button then you will be able to control your board!.");

}

void serialPrintIpAddress() {
  if (Serial.available()) {
    if (Serial.read() > 0) {
      if (millis() - serialTimer > 2000) {
        if (WiFi.status() == WL_CONNECTED) {
          connectedToWifiNetwork();
        } else {
          notConnectedToWifiNetwork();
        }
      }
      serialTimer = millis();
    }
  }
}

void process_WIFI(WiFiClient client) {
  String getString = client.readStringUntil('/');
  String arduinoString = client.readStringUntil('/');
  String command = client.readStringUntil('/');

  if (command == "digital") {
    digitalCommand_WIFI(client);
  }

  if (command == "pwm") {
    pwmCommand_WIFI(client);
  }

  if (command == "servo") {
    servoCommand_WIFI(client);
  }

  if (command == "terminal") {
    terminalCommand_WIFI(client);
  }

  if (command == "mode") {
    modeCommand_WIFI(client);
  }

  if (command == "allonoff") {
    allonoff_WIFI(client);
  }

  if (command == "password") {
    changePassword_WIFI(client);
  }

  if (command == "allstatus") {
    allstatus_WIFI(client);
  }

}

void terminalCommand_WIFI(WiFiClient client) {//Here you recieve data form app terminal
  String data = client.readStringUntil('/');
  //  client.print(httpAppJsonOk + "Ok from Arduino " + String(random(1, 100)));
  Serial.println(data);
  String feedBack = "";
  if (data.equalsIgnoreCase( "Ip") ) {
    feedBack = "IP Addr: ";
    feedBack += WiFi.localIP().toString().c_str();
    client.print(httpAppJsonOk + feedBack);
  }

  else if (data.equalsIgnoreCase("Rest")) {
    feedBack = "RESTART AFTER 3 SEC";
    client.print(httpAppJsonOk + feedBack);
    ESP.restart();
  }
  else {
    client.print(httpAppJsonOk + "Ok from Arduino " + String(random(1, 100)));
  }
}

void digitalCommand_WIFI(WiFiClient client) {
  int pin, value;
  pin = client.parseInt();
  if (client.read() == '/') {
    value = client.parseInt();
    digitalWrite(pin, value);
    pinsValue[pin] = value;
    client.print(httpAppJsonOk + value);
  }
}

void pwmCommand_WIFI(WiFiClient client) {
  int pin, value;
  pin = client.parseInt();
  if (client.read() == '/') {
    value = client.parseInt();
    ledcWrite(pin, value);
    pinsValue[pin] = value;
    client.print(httpAppJsonOk + value);
  }
}

void servoCommand_WIFI(WiFiClient client) {
  int pin, value;
  pin = client.parseInt();
  if (client.read() == '/') {
    value = client.parseInt();
    int valueMapped = map(value, 0, 180, 10, 35);
    ledcWrite(pin, valueMapped);
    pinsValue[pin] = value;
    client.print(httpAppJsonOk + value);
  }
}

void modeCommand_WIFI(WiFiClient client) {
  String  pinString = client.readStringUntil('/');
  int pin = pinString.toInt();
  String mode = client.readStringUntil('/');

  if (mode != "servo") {
    ledcDetachPin(pin);
  };

  if (mode != "pwm") {
    ledcDetachPin(pin);
  };

  if (mode == "output") {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
    pinsMode[pin] = 'o';
    pinsValue[pin] = 0;
    allstatus_WIFI(client);
  }
  if (mode == "push") {
    pinsMode[pin] = 'm';
    pinsValue[pin] = 0;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
    allstatus_WIFI(client);
  }
  if (mode == "schedule") {
    pinsMode[pin] = 'c';
    pinsValue[pin] = 0;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, 0);
    allstatus_WIFI(client);
  }

  if (mode == "input") {
    pinsMode[pin] = 'i';
    pinsValue[pin] = 0;
    pinMode(pin, INPUT);
    allstatus_WIFI(client);
  }

  if (mode == "pwm") {
    pinsMode[pin] = 'p';
    pinsValue[pin] = 0;
    ledcAttachPin(pin, pin);
    ledcWrite(pin, 0);
    allstatus_WIFI(client);
  }

  if (mode == "servo") {
    pinsMode[pin] = 's';
    pinsValue[pin] = 0;
    ledcAttachPin(pin, pin);
    ledcWrite(pin, 0);
    allstatus_WIFI(client);
  }

}

void allonoff_WIFI(WiFiClient client) {
  int pin, value;
  value = client.parseInt();
  for (byte i = 0; i <= 39; i++) {
    if (pinsMode[i] == 'o') {
      digitalWrite(i, value);
      pinsValue[i] = value;
    }
  }
  client.print(httpTextPlainOk + value);
}

void changePassword_WIFI(WiFiClient client) {
  String data = client.readStringUntil('/');
  protectionPassword = data;
  client.print(httpAppJsonOk);
}

void allstatus_WIFI(WiFiClient client) {
  String dataResponse;
  dataResponse += F("HTTP/1.1 200 OK \r\n");
  dataResponse += F("content-type:application/json \r\n\r\n");
  dataResponse += "{";

  dataResponse += "\"m\":[";//m for mode
  for (byte i = 0; i <= 39; i++) {
    dataResponse += "\"";
    dataResponse += pinsMode[i];
    dataResponse += "\"";
    if (i != 39)dataResponse += ",";
  }
  dataResponse += "],";

  dataResponse += "\"v\":[";//v for value
  for (byte i = 0; i <= 39; i++) {
    dataResponse += pinsValue[i];
    if (i != 39)dataResponse += ",";
  }
  dataResponse += "],";

  dataResponse += "\"a\":[";//a for analog value
  for (byte i = 32; i <= 39; i++) {
    if (i == 37 || i == 38  ) {

    }
    else {
      dataResponse += analogRead(i);
      if (i != 39)dataResponse += ",";
    }
  }
  dataResponse += "],";

  dataResponse += "\"l\":[";//l for LCD value
  for (byte i = 0; i <= lcdSize - 1; i++) {
    dataResponse += "\"";
    dataResponse += lcd[i];
    dataResponse += "\"";
    if (i != lcdSize - 1)dataResponse += ",";
  }
  dataResponse += "],";
  dataResponse += "\"t\":\""; //t for Board Type .
  dataResponse += boardType;
  dataResponse += "\",";
  dataResponse += "\"b\":\""; //b for app build version .
  dataResponse += appBuildVersion;
  dataResponse += "\",";
  dataResponse += "\"p\":\""; // p for Password.
  dataResponse += protectionPassword;
  dataResponse += "\"";
  dataResponse += "}";
  client.print(dataResponse);

}

void checkButtons() {

  if (digitalRead(12) == HIGH ) { // node pin 2
    if (millis() - lastbutton > 250) {
      int xbut = !digitalRead(2);
      digitalWrite(2, xbut ? HIGH : LOW);// node pin 6
      pinsValue[2] = xbut;
    }
    lastbutton = millis();
  }

  if (digitalRead(13) == HIGH ) { // node pin 1
    if (millis() - lastbutton > 250) {
      int xbut = !digitalRead(4);
      digitalWrite(4, xbut ? HIGH : LOW);// node pin 7
      pinsValue[4] = xbut;
    }
    lastbutton = millis();
  }

  if (digitalRead(14) == HIGH ) { // node pin 0
    if (millis() - lastbutton > 250) {
      int xbut = !digitalRead(5);
      digitalWrite(5, xbut ? HIGH : LOW);// node pin 8
      pinsValue[5] = xbut;
    }
    lastbutton = millis();
  }

  if (digitalRead(26) == HIGH ) { // node pin 0
    if (millis() - lastbutton > 250) {
      int xbut = !digitalRead(18);
      digitalWrite(18, xbut ? HIGH : LOW);// node pin 8
      pinsValue[18] = xbut;
    }
    lastbutton = millis();
  }

}
