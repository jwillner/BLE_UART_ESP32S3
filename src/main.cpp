#include <Arduino.h>

// inspired by https://openlabpro.com/guide/ble-notify-on-esp32-controller/
// inspired by https://github.com/rixnco/ESP32_BLE_NUS_C/blob/main/src/main.cpp
// inspired by https://www.esp32.com/viewtopic.php?t=20293
// inspired by https://github.com/nkolban/esp32-snippets/blob/master/Documentation/BLE%20C%2B%2B%20Guide.pdf
// inspired by https://github.com/nkolban/esp32-snippets/blob/master/Documentation/BLE%20C%2B%2B%20Guide.pdf
// inspired by https://github.com/nkolban/esp32-snippets/issues/945

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define RX_CHARACTERISTIC_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_CHARACTERISTIC_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define SERVER_NAME "JoWisUartServer"

bool deviceConnected = false;
bool deviceAdvertising = false;
bool dataReceived = false;
BLECharacteristic *pTxCharacteristic;

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    deviceAdvertising = true;
  }
};

char receivedData[80];

char inputString[80];

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();

    sprintf(receivedData, "                                                                  ");
    if (rxValue.length() > 0)
    {
      for (int i = 0; i < rxValue.length(); i++)
        receivedData[i] = rxValue[i];
    }
    dataReceived = true;
  }
};

void setup()
{
  Serial.begin(115200);
  // initialize the ble enviroment
  BLEDevice::init(SERVER_NAME);

  // create the server
  BLEServer *pUartServer = BLEDevice::createServer();
  pUartServer->setCallbacks(new MyServerCallbacks());

  // create the service
  BLEService *pUartService = pUartServer->createService(SERVICE_UUID);

  // create the characteristics
  BLECharacteristic *pRxCharacteristic = pUartService->createCharacteristic(RX_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  pTxCharacteristic = pUartService->createCharacteristic(TX_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pTxCharacteristic->addDescriptor(new BLE2902());
  pRxCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pUartService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Wait for client....");
}

int loop_counter = 0;
char payload[20];
int cnt = 0;
int counter = 0;

void loop()
{
  if (deviceConnected == true)
  {
    counter++;

    sprintf(payload, "send: %d", counter);
    pTxCharacteristic->setValue(payload);
    pTxCharacteristic->notify();
    Serial.println(payload);

    if (dataReceived)
    {
      sprintf(inputString, "received: %s", receivedData);
      Serial.println(inputString);
      dataReceived=false;
    }

    delay(1000);
  }

  if (deviceAdvertising == true)
  {
    BLEDevice::startAdvertising();
    deviceAdvertising = false;
    delay(1000);
  }
}
