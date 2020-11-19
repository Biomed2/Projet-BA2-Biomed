#include <WiFi.h>

const char* ssid = "WiFi-2.4-C117";
const char* password = "esc337yy";
void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n");
  
  WiFi.begin(ssid, password);
  Serial.print("Tentative de connexion...");
  
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  
  Serial.println("\n");
  Serial.println("Connexion etablie!");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());

  pinMode(13, OUTPUT);
}

void loop()
{
    if(WiFi.status() == 3) {
    digitalWrite(13, HIGH);
  }
  else {
    digitalWrite(13, LOW);
  }  
}
