#include <WiFiManager.h>
#include <InfluxDbClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "BLEDevice.h"
#include "dbgout.h"
//WiFiManager setup
WiFiManager wm;
const char* ssid = "ESP32y";
const char* password = "123456789";
const char* cred_nom;
const char* cred_prenom;

//Influxdb setup
#define INFLUXDB_URL "http://influx.biomed.ulb.ovh"
#define INFLUX_DB_NAME "biomed2"
#define INFLUXDB_USER "biomed2"
#define INFLUXDB_PASSWORD "SsUdBB5zicWao8M4"
InfluxDBClient client(INFLUXDB_URL, INFLUX_DB_NAME);

//Deep Sleep setup
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP  10  
RTC_DATA_ATTR int bootCount = 0;

//Temperature sensor setup
OneWire oneWire(4);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

//Bluetooth
//If you know the oxy BLE address, you can replace by it, so it will always connect to a specific one
#define OXYADDRESS "D5:13:D5:50:08:6D"

//Device name of the oxymeter (don't change it)
#define OXYNAME "SP001"

//NULL BLUTOOTH MAC ADDRESS
#define BLENULLADDRESS "00:00:00:00:00:00"

// BLE Address of the oxymeter
static BLEAddress OxyAddr(OXYADDRESS);
// The remote service we wish to connect to (only visible after connection to the device !)
static BLEUUID serviceUUID("f000efe0-0451-4000-0000-00000000b000");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("f000efe3-0451-4000-0000-00000000b000");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
static int OxyRSSI = 0;
//timestamps when oxymeter was respectively on and off
unsigned long laston = 0, lastoff = 0;

static void pointInflux ( String prenom, String nom, String stat, String pointName, float variable ){
  Point pointStatus(stat);
  pointStatus.addTag(prenom, nom);
  pointStatus.addField(pointName, variable);
  client.writePoint(pointStatus);  
}

//callback when the oxymeter is on or off
static void onoffCallback(boolean is_on,long duration) {
  if (is_on) {
    Serial.print("The oxymeter is now on and was off during ");
    Serial.print(-duration/1000);
  } else {
    Serial.print("The oxymeter is now off and was on during ");
    Serial.print(duration/1000);
  }
  Serial.println("sec");
}

//callbacks on connection and disconnection of the oxymeter
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
    }

    void onDisconnect(BLEClient* pclient) {
      connected = false; //do not remove this !!
    }
};

//Notification callback from the oxymeter (do not change it !)
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  dbgout("raw data notification : ");
  for (int i = 0; i < length; i++) {
    dbgout(pData[i]);
    dbgout(", ");
  }
  dbgoutln(); 
  switch (pData[0]) {
    case 18:
      pointInflux ("Jean Michel", "Dupont", "statusSpo2", "concentrationOxy", pData[12]);
      pointInflux ("Jean Michel", "Dupont", "statusPi", "pulse", pData[14]/10);
      
      break;
    case 10:
      pointInflux ("Jean Michel", "Dupont", "statusBpm", "bpm", pData[12]);
      break;
    case 252:
      if (pData[2] == 2) laston = millis();
      else lastoff=millis();
      onoffCallback(pData[2] == 2,lastoff-laston);
      break;    
  }  
}

//Connection to the oxymeter
bool connectToServer() {
  dbgout("Forming a connection to ");
  dbgoutln(myDevice->getAddress().toString().c_str());

  BLEClient*  pClient  = BLEDevice::createClient();
  dbgoutln(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  dbgoutln(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    dbgout("Failed to find our service UUID: ");
    dbgoutln(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  dbgoutln(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    dbgout("Failed to find our characteristic UUID: ");
    dbgoutln(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  dbgoutln(" - Found our characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead()) {
    std::string value = pRemoteCharacteristic->readValue();
    dbgout("The characteristic value was: ");
    dbgoutln(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
  return true;
}
/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      dbgout("BLE Advertised Device found: ");
      dbgoutln(advertisedDevice.toString().c_str());

      // We have found a device, let us now see if it is the oxymeter we are looking for
      // If the ble address is specified, it has priority vs. the oxy name and also, in case of disconnection, reconnect to the same one
      if ((OxyAddr.toString()==BLENULLADDRESS && advertisedDevice.getName() == OXYNAME) || OxyAddr.equals(advertisedDevice.getAddress())) {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;
        OxyAddr =  advertisedDevice.getAddress(); 
        OxyRSSI =  advertisedDevice.getRSSI();      
      } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks

void setup() {
  if(bootCount == 0) {  
    //WiFi Station setup
    WiFi.mode(WIFI_STA);
    WiFiManagerParameter nom("nom", "Nom", "", 40);
    wm.addParameter(&nom);
    cred_nom = nom.getValue();
    WiFiManagerParameter prenom("prenom", "Prénom", "", 40);
    wm.addParameter(&prenom);
    cred_prenom = prenom.getValue();
  }
  ++bootCount;
  pinMode(13, OUTPUT);
  
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n");

  wm.autoConnect(ssid, password);

  client.setConnectionParamsV1(INFLUXDB_URL, INFLUX_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);

  //Temperature sensors
  sensors.begin();
  
  sensors.setResolution(insideThermometer, 12);

  dbgoutln("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 10 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  dbgoutln("BLE intensive scan for 10sec");
  pBLEScan->start(10, false);
}

void loop() {
  
  if(WiFi.status() == 3) {
    digitalWrite(13, HIGH);
  }
  else {
    digitalWrite(13, LOW);
  }  
  // Read Temperature
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(insideThermometer);
  int tempSend = 100*tempC;

  pointInflux ("Jean Michel", "Dupont", "statusTemp", "temperature", tempC);
 
  
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server (i.e. Oxymeter) with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      dbgoutln("We are now connected to the BLE Server.");
    } else {
      dbgoutln("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {    
    //we may do something here since we know we are connected to the oxymeter
  } else if (doScan) {
    //not connected anymore --> infinite scan 
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect (infinite scan !), most likely there is better way to do it in arduino
  }  
  
  //delay(500);  Delay half a second between loops/this delay can probably be removed
}
