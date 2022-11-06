#include <Arduino.h>
#include "WiFi.h"
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <EEPROM.h>

#define LED_PIN 13
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

const byte EPPROM_ADDRESS = 0;
bool firstConnection = false;
byte lastState = 0;
int radius = 0;

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;

void httpRequest()
{
  HTTPClient http;
  String url = "https://jsonplaceholder.typicode.com/todos/1";
  http.begin(url.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    // String payload = http.getString();
    // Serial.println(payload);
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}

void BLEConnection()
{
  BLEDevice::init("Coleira");
  pServer = BLEDevice::createServer();
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setValue("Conectando...");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void WifiConnection()
{
  const char *ssid = "Lais's Galaxy S20 FE"; // Enter SSID
  const char *password = "xpag6125";         // Enter Password

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for wifi to be connected
  uint32_t notConnectedCounter = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    if (notConnectedCounter % 3 == 0)
    {
      Serial.println("Wifi connecting...");
    }
    // notConnectedCounter++;
    // if (notConnectedCounter > 150)
    // { // Reset board if not connected after 5s
    //   Serial.println("Resetting due to Wifi not connecting...");
    //   ESP.restart();
    // }
  }
  pCharacteristic->setValue(radius);
  Serial.println("");
  Serial.println("WiFi connection Successful");
  firstConnection = true;
}

void saveData(int data)
{
  EEPROM.write(EPPROM_ADDRESS, data * -1);
  EEPROM.commit();
}

int getData()
{
  int data = EEPROM.read(EPPROM_ADDRESS);
  return data * -1;
}

int setMaxRadius()
{
  int rssi = WiFi.RSSI();
  std::string value = pCharacteristic->getValue().c_str();
  if (value == "trigger")
  {
    Serial.print("Set new radius: ");
    Serial.println(rssi);
    saveData(rssi);
    pCharacteristic->setValue("");
    radius = rssi;
  }
  return radius;
}

void controlOutput()
{

  if (WiFi.RSSI() < radius)
  {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Fora do raio");
    if (lastState == 0)
    {
      // httpRequest();
      lastState = 1;
    }
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
    Serial.println("Dentro do raio");
    if (lastState == 1)
    {
      // httpRequest();
      lastState = 0;
    }
  }
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  EEPROM.begin(8);

  radius = getData();

  Serial.print("EPPROM: ");
  Serial.println(radius);

  // Bluetooth
  BLEConnection();

  // Connect to WiFi
  WifiConnection();
}

void loop()
{

  setMaxRadius();

  if (WiFi.status() == WL_CONNECTED)
  {
    controlOutput();
  }
  else if (WiFi.status() != WL_CONNECTED && firstConnection == true)
  {
    Serial.println("Fora de alcance");
    digitalWrite(LED_PIN, HIGH);
    WiFi.disconnect();
    WiFi.reconnect();
  }
}
