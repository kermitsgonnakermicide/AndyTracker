#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <AccelStepper.h>

const char* ssid = "";
const char* password = "";
const char* token = "NUHUH";

// Stepper pins (change if needed)
#define STEPPER1_PIN1 14
#define STEPPER1_PIN2 27
#define STEPPER1_PIN3 26
#define STEPPER1_PIN4 25

#define STEPPER2_PIN1 19
#define STEPPER2_PIN2 18
#define STEPPER2_PIN3 5
#define STEPPER2_PIN4 4

AccelStepper stepperAzimuth(AccelStepper::HALF4WIRE, STEPPER1_PIN1, STEPPER1_PIN3, STEPPER1_PIN2, STEPPER1_PIN4);
AccelStepper stepperElevation(AccelStepper::HALF4WIRE, STEPPER2_PIN1, STEPPER2_PIN3, STEPPER2_PIN2, STEPPER2_PIN4);

//fixed location
float myLat = 28.6139;   
float myLon = 77.2090;
float myAlt = 220;       

float calculateAzimuth(float lat1, float lon1, float lat2, float lon2) {
  float dLon = radians(lon2 - lon1);
  lat1 = radians(lat1);
  lat2 = radians(lat2);

  float y = sin(dLon) * cos(lat2);
  float x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
  float bearing = atan2(y, x);
  return fmod((degrees(bearing) + 360), 360);
}

float calculateElevation(float lat1, float lon1, float alt1, float lat2, float lon2, float alt2) {
  float R = 6371000; 

  float dLat = radians(lat2 - lat1);
  float dLon = radians(lon2 - lon1);
  float a = sin(dLat / 2) * sin(dLat / 2) +
            cos(radians(lat1)) * cos(radians(lat2)) *
            sin(dLon / 2) * sin(dLon / 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  float horizontalDistance = R * c;

  float verticalDifference = alt2 - alt1;
  return degrees(atan2(verticalDifference, horizontalDistance));
}

int degreesToSteps(float degrees) {
  return int(degrees * 2048.0 / 360.0); 
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");

  HTTPClient http;
  http.begin("https://demo.traccar.org/api/devices");
  http.addHeader("Authorization", token);

  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("Device fetch failed: %d\n", httpCode);
    return;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(4096);
  deserializeJson(doc, payload);

  int deviceId = -1;
  for (JsonObject d : doc.as<JsonArray>()) {
    if (d["name"] == "andyphone") {
      deviceId = d["id"];
      break;
    }
  }

  if (deviceId == -1) {
    Serial.println("andyphone not found.");
    return;
  }

  // Get positions
  String url = "https://demo.traccar.org/api/positions?deviceId=" + String(deviceId);
  http.begin(url);
  http.addHeader("Authorization", token);
  httpCode = http.GET();

  if (httpCode != 200) {
    Serial.printf("Position fetch failed: %d\n", httpCode);
    return;
  }

  String posPayload = http.getString();
  http.end();

  DynamicJsonDocument posDoc(2048);
  deserializeJson(posDoc, posPayload);

  if (posDoc.size() == 0) {
    Serial.println("No position data.");
    return;
  }

  float targetLat = posDoc[0]["latitude"];
  float targetLon = posDoc[0]["longitude"];
  float targetAlt = posDoc[0]["altitude"] | 0; // fallback if missing

  Serial.printf("Target: Lat=%.6f Lon=%.6f Alt=%.2f\n", targetLat, targetLon, targetAlt);

  float az = calculateAzimuth(myLat, myLon, targetLat, targetLon);
  float el = calculateElevation(myLat, myLon, myAlt, targetLat, targetLon, targetAlt);

  Serial.printf("Azimuth: %.2f°, Elevation: %.2f°\n", az, el);

  stepperAzimuth.setMaxSpeed(500);
  stepperElevation.setMaxSpeed(500);
  
  stepperAzimuth.moveTo(degreesToSteps(az));
  stepperElevation.moveTo(degreesToSteps(el));
}

void loop() {
  stepperAzimuth.run();
  stepperElevation.run();
}
