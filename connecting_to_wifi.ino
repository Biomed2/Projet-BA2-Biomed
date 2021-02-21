#include <WiFiManager.h>
#include <InfluxDbClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//WiFiManager setup
WiFiManager wm;
const char* ssid = "ESP32y";
const char* password = "123456789";
const char* PATIENT_ID;

//Influxdb setup
#define INFLUXDB_URL "http://influx.biomed.ulb.ovh"
#define INFLUX_DB_NAME "biomed2"
#define INFLUXDB_USER "biomed2"
#define INFLUXDB_PASSWORD "SsUdBB5zicWao8M4"
InfluxDBClient client(INFLUXDB_URL, INFLUX_DB_NAME);

//Deep Sleep setup
#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP  50  
RTC_DATA_ATTR int bootCount = 0;

//Temperature sensor setup
OneWire oneWire(4);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

void setup() {
  pinMode(19, OUTPUT);
  pinMode(23, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(13, OUTPUT);
  
  if(bootCount == 0) {  
    //WiFi Station setup
    digitalWrite(18, HIGH);
    WiFi.mode(WIFI_STA);
    WiFiManagerParameter PatientID("PatientID", "PatientID", "", 16);
    wm.addParameter(&PatientID);
    PATIENT_ID = PatientID.getValue();
  }
  ++bootCount;
  
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n");

  wm.autoConnect(ssid, password);

  client.setConnectionParamsV1(INFLUXDB_URL, INFLUX_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);

  //Temperature sensors
  sensors.begin();
   
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 

  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();
  
  sensors.setResolution(insideThermometer, 12);
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();
}

void loop() {
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  if(WiFi.status() == 3) {
    digitalWrite(23, HIGH);
  }
  else {
    digitalWrite(23, LOW);
  }  
  //while(analogRead(35) <= 1500) {
  //  digitalWrite(23, LOW);
  //  digitalWrite(19, HIGH);
  //}
  int bat = map(analogRead(35), 1500, 2050, 0, 100);
  // Read Temperature
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(insideThermometer);
  int tempSend = 100*tempC;
  
  // Define data point with measurement name 'status'
  Point pointStatus("status");
  
  // Set tags
  pointStatus.addTag("PatientID", PATIENT_ID);
  
  // Set Fields  
  pointStatus.addField("Temperature", tempSend);
  pointStatus.addField("Batterie", bat);
    
  Serial.print("Data Sent : ");
  Serial.println(tempC);
  // Write data
  client.writePoint(pointStatus);
  //esp_deep_sleep_start();

}

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
