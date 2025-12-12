#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SparkFunLSM6DSO.h>
#include <ArduinoJson.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


const char* ssid = "<ssid>>";
const char* password = "<password>";
String serverURL = "http://<public_ip>:8000/ingest";

const int buttonPin = 5;
const int lightPin = 35;
const int buzzerPin = 26;

const int ledcChannel = 0;
const int ledcFreq = 1000;
const int ledcResolution = 10;

bool calibrated = false;
bool wasDark = true;

int calibratedDark = 0;
int lightThreshold = 0;

LSM6DSO imu;
float baseX, baseY, baseZ;

void calibratePosture(float x, float y, float z) {
    float mag = sqrt(x*x + y*y + z*z);
    baseX = x / mag;
    baseY = y / mag;
    baseZ = z / mag;

    Serial.println("Posture calibrated!");
    Serial.print("Base X: "); Serial.println(baseX, 3);
    Serial.print("Base Y: "); Serial.println(baseY, 3);
    Serial.print("Base Z: "); Serial.println(baseZ, 3);
}

float computeAngle(float x, float y, float z, String &postureOut) {
    float mag = sqrt(x*x + y*y + z*z);
    x /= mag;
    y /= mag;
    z /= mag;

    float dot = baseX*x + baseY*y + baseZ*z;
    dot = constrain(dot, -1.0f, 1.0f);

    float angle = acos(dot) * 180.0 / PI;

    if (angle < 5) postureOut = "Upright";
    else if (angle < 8) postureOut = "Slight Lean";
    else if (angle < 15) postureOut = "Moderate Forward Lean";
    else postureOut = "Heavy Slouch";

    return angle;
}

void sendTelemetry(float angle, String posture, int lightValue, bool buzzed, bool calibratedEvent) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi NOT connected, skipping upload.");
        return;
    }

    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    // ----------- JSON PAYLOAD -----------
    String jsonPayload = "{";
    jsonPayload += "\"tilt_angle\":" + String(angle, 2) + ",";
    jsonPayload += "\"posture_label\":\"" + posture + "\",";
    jsonPayload += "\"light\":" + String(lightValue) + ",";
    jsonPayload += "\"buzzer\":" + String(buzzed ? "1" : "0") + ",";
    jsonPayload += "\"calibration\":" + String(calibratedEvent ? "1" : "0");
    jsonPayload += "}";

    Serial.println("Uploading JSON:");
    Serial.println(jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);

    Serial.print("Server response: ");
    Serial.println(httpResponseCode);

    http.end();
}


// =====================================================
//                     SETUP
// =====================================================
void setup() {
    Serial.begin(115200);
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    delay(500);

    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(lightPin, INPUT);

    ledcSetup(ledcChannel, ledcFreq, ledcResolution);
    ledcAttachPin(buzzerPin, ledcChannel);

    // ----------- WIFI CONNECT -----------
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // ----------- IMU INIT -----------
    Wire.begin(21, 22);

    Serial.println("Initializing LSM6DSO...");
    if (imu.begin()) {
        Serial.println("LSM6DSO connected!");
        imu.initialize(BASIC_SETTINGS);
    } else {
        Serial.println("ERROR: LSM6DSO not detected!");
        while (1);
    }

    Serial.println("READY. Press button to calibrate.");
}


void loop() {
    if (digitalRead(buttonPin) == LOW) {
        Serial.println("\n=== CALIBRATING ===");

        float ax = imu.readFloatAccelX();
        float ay = imu.readFloatAccelY();
        float az = imu.readFloatAccelZ();
        calibratePosture(ax, ay, az);

        calibratedDark = analogRead(lightPin);
        lightThreshold = calibratedDark + 300;

        Serial.print("Dark baseline: ");
        Serial.println(calibratedDark);
        Serial.print("Light threshold: ");
        Serial.println(lightThreshold);

        calibrated = true;

        sendTelemetry(0, "CALIBRATED", calibratedDark, false, true);

        delay(700); 
    }

    if (!calibrated) return;

    int lightValue = analogRead(lightPin);
    bool isDark = (lightValue < lightThreshold);

    bool buzzed = false;

    if (!isDark && wasDark) {
        Serial.println("LIGHT detected — Buzzing!");
        ledcWrite(ledcChannel, 512);
        delay(300);
        ledcWrite(ledcChannel, 0);
        buzzed = true;
    }

    wasDark = isDark;

    float ax = imu.readFloatAccelX();
    float ay = imu.readFloatAccelY();
    float az = imu.readFloatAccelZ();

    String postureText;
    float angle = computeAngle(ax, ay, az, postureText);

    Serial.print("Angle: ");
    Serial.print(angle);
    Serial.print(" | Posture: ");
    Serial.println(postureText);

    sendTelemetry(angle, postureText, lightValue, buzzed, false);

    delay(1000);
}
