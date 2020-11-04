#include <WiFiManager.h>
#include <InfluxDbClient.h>

WiFiManager wm;
const char* ssid = "ESP32Rudy";
const char* password = "123456789";
const char* cred_nom;
const char* cred_prenom;

//Influxdb setup
#define INFLUXDB_URL "http://influx.biomed.ulb.ovh"
#define INFLUX_DB_NAME "biomed2"
#define INFLUXDB_USER "biomed2"
#define INFLUXDB_PASSWORD "SsUdBB5zicWao8M4"

InfluxDBClient client(INFLUXDB_URL, INFLUX_DB_NAME);

int val;

void setup() {
  WiFi.mode(WIFI_STA);
  WiFiManagerParameter nom("nom", "Nom", "", 40);
  wm.addParameter(&nom);
  cred_nom = nom.getValue();
  WiFiManagerParameter prenom("prenom", "Prénom", "", 40);
  wm.addParameter(&prenom);
  cred_prenom = prenom.getValue();
  
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n");

  if(!wm.autoConnect(ssid, password))
    Serial.println("Erreur de connexion.");
  else
    Serial.println("Connexion etablie!");
    Serial.print("Données supplémentaires : "); Serial.print(cred_prenom), Serial.print(", "); Serial.println(cred_nom);

  client.setConnectionParamsV1(INFLUXDB_URL, INFLUX_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
  
  pinMode(34, INPUT);

}

void loop() {
  // Define data point with measurement name 'device_status`
  Point pointStatus("status");
  // Set tags
  pointStatus.addTag("Prénom", cred_prenom);
  pointStatus.addTag("Nom", cred_nom);
  //Set Fields  
  pointStatus.addField("potentiel", analogRead(34));
  Serial.print("Data Sent : ");
  Serial.println(analogRead(34));
  // Write data
  client.writePoint(pointStatus);
  
  delay(500);
}
