// LBRARIES ////////////////////////////////////////////////////////////////////


// global libraries //
#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// global variables ////////////////////////////////////////////////////////////

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;


// definition of all the objects needed to create a bluetooth GATT server //////
////////////////////////////////////////////////////////////////////////////////

#define DEVICE_NAME "AmpelBLE"

#define DEVICE_INFO_SERVICE_UUID "180a"
#define MANUFACTURER_CHARACTERISTIC_UUID "2a29"
#define PNP_CHARACTERISTIC_UUID "2a50"
#define BATTERY_SERVICE_UUID "180f"
#define BATTERY_CHARACTERISTIC_UUID "2a19"


#define AMPEL_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"  // See the following for generating UUIDs:
#define BUTTONS_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"  // https://www.uuidgenerator.net/
#define BUTTONS_DESCRIPTOR_UUID     "beb5483f-36e1-4688-b7f5-ea07361b26a8"
#define LED_CHARACTERISTIC_UUID "beb5482e-36e1-4688-b7f5-ea07361b26a8"
#define LED_DESCRIPTOR_UUID     "beb5482f-36e1-4688-b7f5-ea07361b26a8"
#define TEST_CHARACTERISTIC_UUID "beb5481e-36e1-4688-b7f5-ea07361b26a8"
#define TEST_DESCRIPTOR_UUID     "beb5481f-36e1-4688-b7f5-ea07361b26a8"

BLEServer* ampelBLE = NULL;
  BLEService*	deviceInfoService;			//0x180a
    BLECharacteristic* 	manufacturerCharacteristic;	//0x2a29
    BLECharacteristic* 	pnpCharacteristic;			//0x2a50
  BLEService*			batteryService = 0;			//0x180f
    BLECharacteristic*	batteryLevelCharacteristic;	//0x2a19
  BLEService* ampelService = NULL;      // specific device service, this is supposed to be an Ampel, with three buttons, and a three color led.
    BLECharacteristic* buttonsCharacteristic = NULL;  // here the status of the buttons can be checked.
    BLECharacteristic* LEDsCharacteristic = NULL;      // here the status of the LED can be checked, or even changed.
    BLECharacteristic* testCharacteristic = NULL;     // just to test notifications, can be used for new feature.

////////////////////////////////////////////////////////////////////////////////




class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* ampelBLE) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* ampelBLE) {
      deviceConnected = false;
    }
};
// this callback is supposed to change switch on and off the builtin LED
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *LEDsCharacteristic) {
      std::string value = LEDsCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};

#define LED_BUILTIN_ON LOW
#define LED_BUILTIN_OFF HIGH

void setup() {

  // setup IO pins
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN,LED_BUILTIN_OFF); // led off


  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init(DEVICE_NAME);

  // Create the BLE Server
  ampelBLE = BLEDevice::createServer();
  ampelBLE->setCallbacks(new MyServerCallbacks());

  // Create BLE Service for ampel
  ampelService = ampelBLE->createService(AMPEL_SERVICE_UUID);

  // Create  BLE Characteristic for LED's
  LEDsCharacteristic = ampelService->createCharacteristic(
                      LED_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
LEDsCharacteristic->setValue("0");    // initial value for the characteristic, if not initialized, LED will turn ON !!


  // Create a BLE Characteristic for button handling
  buttonsCharacteristic = ampelService->createCharacteristic(
                      BUTTONS_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      //BLECharacteristic::PROPERTY_WRITE  |    // the value of this characteristic comes determined by buttons, shouldn't be changed.
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

// Create a BLE Characteristic testing purposes
  testCharacteristic = ampelService->createCharacteristic(
                      TEST_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  //pCharacteristic->addDescriptor(new BLE2902());

  BLEDescriptor *buttonsDescriptor = new BLEDescriptor(BUTTONS_DESCRIPTOR_UUID);
  buttonsDescriptor->setValue("Buttons: 1=B1, 2=B2, 4=B3");
  buttonsCharacteristic->addDescriptor(buttonsDescriptor);

  BLEDescriptor *ledsDescriptor = new BLEDescriptor(LED_DESCRIPTOR_UUID);
  ledsDescriptor->setValue("LEDs Characteristic: 0off,1R, 2G, 3Y");
  LEDsCharacteristic->addDescriptor(ledsDescriptor);

  BLEDescriptor *testDescriptor = new BLEDescriptor(TEST_DESCRIPTOR_UUID);
  testDescriptor->setValue("Descriptor for testing");
  testCharacteristic->addDescriptor(testDescriptor);
  // Start the service
  ampelService->start();      // after filling in descriptors, we start the Service

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(AMPEL_SERVICE_UUID);                 // !!! do in need to add also othe rservices here?
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter hmmm !!!
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

}

void loop() {
    // notify changed value
    unsigned char alreadyConnected = 0; 
    if (deviceConnected) {
      if(alreadyConnected == 0){              // whatever inside this if only happens when the connection happens
        Serial.println("device connected");
      }
      alreadyConnected = 1;
        


        // stuff to do with characteristic TEST //
        testCharacteristic->setValue((uint8_t*)&value, 4);
        testCharacteristic->notify();
        value++;

        // stuff to do with characteristic LEDS //         // this will not work, also because the string needs to be 0
                                                        // convert the getValue() into a uint_16 before comparing, also the other side of the equalty.
        if (LEDsCharacteristic->getValue() == "0")      // this might not work, the getvalue() only happens once, IT WORKS !! references last written value!!!
          digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
        else
          digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);

        // stuff to do with characteristic BUTTONS //



        delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        Serial.println("Device Disconnected");
        ampelBLE->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    // if(!deviceConnected){
    //   alreadyConnected = 0;                           // reset the first connection flag
    //   Serial.println("Device disconnected");          // report on serial the device is disconnected
    // }
}
