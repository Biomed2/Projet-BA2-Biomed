#include <WiFi.h>
#include <InfluxDbClient.h>
#define WIFI_SSID "WiFi-2.4-C117_EXT"
#define WIFI_PASSWORD "esc337yy"

//Influxdb setup
#define INFLUXDB_URL "http://influx.biomed.ulb.ovh"
#define INFLUX_DB_NAME "biomed2"
#define INFLUXDB_USER "biomed2"
#define INFLUXDB_PASSWORD "SsUdBB5zicWao8M4"
InfluxDBClient client(INFLUXDB_URL, INFLUX_DB_NAME);

#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER1  "pool.ntp.org"
#define NTP_SERVER2  "time.nis.gov"
#define WRITE_PRECISION WritePrecision::S
#define MAX_BATCH_SIZE 10
#define WRITE_BUFFER_SIZE 30

Point sensorStatus("batch");
int iterations = 0;

void setup() {
  Serial.begin(115200);

  // Setup wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  client.setConnectionParamsV1(INFLUXDB_URL, INFLUX_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
  Serial.println("WiFi connected");

  sensorStatus.addTag("device", "esp32");
  timeSync(TZ_INFO, NTP_SERVER1, NTP_SERVER2);

  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  client.setWriteOptions(WriteOptions().writePrecision(WRITE_PRECISION).batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE));

}

void loop() {
  if (iterations++ >= 360) {
    timeSync(TZ_INFO, NTP_SERVER1, NTP_SERVER2);
    iterations = 0;
  }
  sensorStatus.setTime(time(nullptr));
  sensorStatus.addField("uyfkv", iterations);
  Serial.print("Writing: ");
  Serial.println(sensorStatus.toLineProtocol());

  client.writePoint(sensorStatus);
  sensorStatus.clearFields();
  delay(2000);
}
