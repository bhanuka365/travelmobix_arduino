#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>

#include "ESP8266WiFi.h"
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

char ssid[] = "iphone";         // your network SSID name

char pass[] = "sahan0011";          // your network password



//Credentials for Google GeoLocation API...


const char* Host = "www.googleapis.com";

String thisPage = "/geolocation/v1/geolocate?key=";

String key = "AIzaSyCFTMWY2GZrDLUTHB-sMUCQAhlIAfGT6xM";
//const char* sslFingerprint = "B3 BE 6E 56 56 97 0F 87 DC 80 76 32 40 F4 0A 6D 33 3D 99 C6";

// Insert Firebase project API Key
#define API_KEY "AIzaSyAJhxzgN8a8NPhRFnFYHk19cZfPY8Ze6sU"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://travelmobix-7d56c-default-rtdb.asia-southeast1.firebasedatabase.app/" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;


unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
int status = WL_IDLE_STATUS;

String jsonString = "{\n";


double latitude    = 0.0;

double longitude   = 0.0;

double accuracy    = 0.0;

int more_text = 1;    // set to 1 for more debug output



void setup()   {

  Serial.begin(115200);

  Serial.println("Start");

 

 // Set WiFi to station mode and disconnect from an AP if it was previously connected

  WiFi.mode(WIFI_STA);

  WiFi.disconnect();

  delay(100);

  Serial.println("Setup done");

  

// We start by connecting to a WiFi network

  Serial.print("Connecting to ");

  Serial.println(ssid);

  WiFi.begin(ssid, pass);


  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");

  }

  Serial.println(".");
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

}



void loop() {


  char bssid[6];

  DynamicJsonBuffer jsonBuffer;

  Serial.println("scan start");

  

// WiFi.scanNetworks will return the number of networks found

  int n = WiFi.scanNetworks();

  Serial.println("scan done");

  if (n == 0)

    Serial.println("no networks found");

  else

  {

    Serial.print(n);

    Serial.println(" networks found...");


    if (more_text) {

      Serial.println("\"wifiAccessPoints\": [");

      for (int i = 0; i < n; ++i)

      {

        Serial.println("{");

        Serial.print("\"macAddress\" : \"");

        Serial.print(WiFi.BSSIDstr(i));

        Serial.println("\",");

        Serial.print("\"signalStrength\": ");

        Serial.println(WiFi.RSSI(i));

        if (i < n - 1)

        {

          Serial.println("},");

        }

        else

        {

          Serial.println("}");

        }

      }

      Serial.println("]");

      Serial.println("}");

    }

    Serial.println(" ");

  }
// now build the jsonString...
jsonString = "{\n";
jsonString += "\"wifiAccessPoints\": [\n";

for (int j = 0; j < n; ++j) {
  jsonString += "{\n";
  jsonString += "\"macAddress\" : \"";
  jsonString += WiFi.BSSIDstr(j);
  jsonString += "\",\n";
  jsonString += "\"signalStrength\": ";
  jsonString += WiFi.RSSI(j);
  jsonString += "\n";
  jsonString += (j < n - 1) ? "},\n" : "}\n";
}

jsonString += ("]\n");
jsonString += ("}\n");

  //-------------------------------------------------------------------- Serial.println("");

// Connect to the client and make the API call
WiFiClientSecure client;
 // client.setFingerprint(sslFingerprint);
client.setInsecure();

Serial.print("Requesting URL: ");
Serial.println("https://" + (String)Host + thisPage + key);
Serial.println(" ");

Serial.println(jsonString);

Serial.print("WiFi status: ");
Serial.println(WiFi.status());
//IPAddress serverIP(216, 58, 195, 78);  // Example IP address for google.com


if (client.connect(Host, 443)) {
    Serial.println("Connected to server successfully.");

  client.println("POST " + thisPage + key + " HTTP/1.1");
  client.println("Host: " + (String)Host);
  client.println("Connection: close");
  client.println("Content-Type: application/json");
  client.println("User-Agent: Arduino/1.0");
  client.print("Content-Length: ");
  client.println(jsonString.length());
  client.println();
  client.print(jsonString);

  delay(500);

  // Read and print the HTTP response code
  Serial.println("Response Code: " + String(client.peek()));

} else {
  // Failed to connect to the server
  Serial.println("Failed to connect to server.");
    Serial.print("Connection error code: ");
  Serial.println(client.getWriteError());
}

  // Read and print all the lines of the reply from the server
  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (more_text) {
      Serial.print(line);
    }

    JsonObject& root = jsonBuffer.parseObject(line);
    if (root.success()) {
      latitude = root["location"]["lat"];
      longitude = root["location"]["lng"];
      accuracy = root["accuracy"];
    }
  }

  Serial.println("closing connection");
  Serial.println();
  client.stop();

  Serial.print("Latitude = ");
  Serial.println(latitude, 6);
  Serial.print("Longitude = ");
  Serial.println(longitude, 6);
  Serial.print("Accuracy = ");
  Serial.println(accuracy);
  int responseCode = client.peek();
Serial.print("Response Code: ");
Serial.println(responseCode);

if ( accuracy<100.0){
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    if (Firebase.RTDB.setFloat(&fbdo, "buses/bus1/location/latitude", latitude) && Firebase.RTDB.setFloat(&fbdo, "buses/bus1/location/longitude", longitude)){
      Serial.println("Location data sent to firebase");
     }
    else {
      Serial.println("Failed");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
  delay(2000);
}

delay(1000);


}