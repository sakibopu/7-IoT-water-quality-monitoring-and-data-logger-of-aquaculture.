#include "DFRobot_ECPRO.h"

#define SIM Serial2
#define PWRKEY 7

#define BUZZER_PIN 4

#define TE_PIN A3
#define TdsSensorPin A4
#define PH_PIN A0
#define TURBIDITY_PIN A1
#define DO_PIN A2

#define VREF 5.0
#define SCOUNT 30
#define ADC_RES 1024

DFRobot_ECPRO ec;
DFRobot_ECPRO_PT1000 ecpt;

String serverURL = "https://script.google.com/macros/s/AKfycbwMlj3N9FE431sGFFGQbW55KJGWiYdOFxY8opIgTohlQhHGKTSjcSyNPzY6NK9fXxwRfQ/exec";
unsigned long lastSent = 0;
int maxRetries = 3;

int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;

float Temp = 25.0, tdsValue = 0, ecValue = 0, phValue = 7.0, turbidityNTU = 0, doValue = 0;
float tdsFactor = 0.45;
float calibration_value = 21.34-1;

const uint16_t DO_Table[41] = {
  14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
  11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
  9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
  7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410
};

unsigned long startTime = 0;
bool dataSent = false;
bool stabilizePhase = true;
//Extra
bool sendAT(String cmd, String expected, unsigned long timeout =2000) {
  while (SIM.available()) SIM.read();
  SIM.println(cmd);
  unsigned long start = millis();
  String response = "";
  while (millis() - start < timeout) {
    while (SIM.available()) {
      char c = SIM.read();
      response += c;
    }
    if (response.indexOf(expected) != -1) {
      Serial.print("✅ ");
      Serial.println(cmd);
      return true;
    }
  }
  Serial.print("❌ AT Command Failed: ");
  Serial.println(cmd);
  Serial.print("Response: ");
  Serial.println(response);
  return false;
}
//Extra
bool waitFor(String expected, unsigned long timeout = 3000) {
  unsigned long start = millis();
  String response = "";
  while (millis() - start < timeout) {
    while (SIM.available()) {
      char c = SIM.read();
      response += c;
    }
    if (response.indexOf(expected) != -1) {
      return true;
    }
  }
  return false;
}

void powerOnSIM() {
  digitalWrite(PWRKEY, HIGH); delay(100);
  digitalWrite(PWRKEY, LOW); delay(1000);
  digitalWrite(PWRKEY, HIGH);
}

int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++) bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
    for (i = 0; i < iFilterLen - j - 1; i++)
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i]; bTab[i] = bTab[i + 1]; bTab[i + 1] = bTemp;
      }
  return (iFilterLen & 1) ? bTab[(iFilterLen - 1) / 2] : (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
}

int16_t readDO(uint32_t voltage_mv, uint8_t temperature_c) {
  uint16_t V_saturation = (uint32_t)770 + 35 * temperature_c - 25 * 35;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
}

void setup() {
  pinMode(PWRKEY, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(TURBIDITY_PIN, INPUT);
  
 
  Serial.begin(115200);
  SIM.begin(115200);
  ec.setCalibration(1.2);
  delay(3000);
  
  powerOnSIM();
  delay(10000);

  sendAT("AT", "OK");
  sendAT("AT+CPIN?", "READY");
  sendAT("AT+CREG?", "0,1");
  sendAT("AT+CSQ", "OK");
  sendAT("AT+CGATT=1", "OK");
  sendAT("AT+CGDCONT=1,\"IP\",\"internet\"", "OK");
  sendAT("AT+NETCLOSE", "OK");
  delay(1000);
  sendAT("AT+NETOPEN", "OK", 3000);
  sendAT("AT+IPADDR", "+IPADDR:");
  sendAT("AT+HTTPTERM", "OK");
  sendAT("AT+HTTPINIT", "OK");

  startTime = millis();
}

void loop() {
  static unsigned long lastSampleTime = 0;

  if (stabilizePhase) {
    // 40 ms পরপর ADC sample
    if (millis() - lastSampleTime > 40U) {
      lastSampleTime = millis();
      analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
      analogBufferIndex = (analogBufferIndex + 1) % SCOUNT;
    }

    // 1 মিনিট পার হয়ে গেলে
    if (millis() - startTime >= 2000) {
      stabilizePhase = false;
      Serial.println("✅ Stabilization complete. Ready to send data.");
    }

    return;
  }

  if (!dataSent) {
    for (int i = 0; i < 100; i++) 
    {

    for (int i = 0; i < SCOUNT; i++) analogBufferTemp[i] = analogBuffer[i];
    float averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * VREF / 1024.0;

    uint16_t TE_Voltage = analogRead(TE_PIN) * 5000UL / 1024;
    Temp = ecpt.convVoltagetoTemperature_C((float)TE_Voltage / 1000);

    float compCoef = 1.0 + 0.02 * (Temp - 25.0);
    float compVolt = averageVoltage / compCoef;

    tdsValue = (133.42 * pow(compVolt, 3) - 255.86 * pow(compVolt, 2) + 857.39 * compVolt) * tdsFactor;
    ecValue = tdsValue / tdsFactor;

    // pH
    int buffer_arr[10], phTemp;
    unsigned long avgval = 0;
    for (int i = 0; i < 10; i++) { buffer_arr[i] = analogRead(PH_PIN); delay(30); }
    for (int i = 0; i < 9; i++) for (int j = i + 1; j < 10; j++)
      if (buffer_arr[i] > buffer_arr[j]) { phTemp = buffer_arr[i]; buffer_arr[i] = buffer_arr[j]; buffer_arr[j] = phTemp; }
    for (int i = 2; i < 8; i++) avgval += buffer_arr[i];
    float phVolt = (float)avgval * 5.0 / 1024 / 6;
    phValue = -5.70 * phVolt + calibration_value;

    // Turbidity
    float turbVolt = analogRead(TURBIDITY_PIN) * 5.0 / 1024.0;
    turbidityNTU = (turbVolt >= 4.0) ? 0 : (turbVolt <= 0.09) ? 3000 : (-857.14 * turbVolt + 3728.57);

    // DO
    uint16_t DO_adc = analogRead(DO_PIN);
    uint16_t DO_mv = 5000UL * DO_adc / ADC_RES;
    doValue = readDO(DO_mv, (uint8_t)Temp);
    //Serial.print(Temp);  Serial.print("   ");  Serial.print(tdsValue);  Serial.print("   ");  Serial.print(doValue); Serial.print("   ");  Serial.print(turbidityNTU);  Serial.print("   ");  Serial.print(ecValue); Serial.print("   ");  Serial.print(phValue); Serial.println("   ");
    
    }

    String json = "{\"temp\":" + String(Temp, 1) +
                  ",\"tds\":" + String(tdsValue, 1) +
                  ",\"do\":" + String(doValue, 0) +
                  ",\"turb\":" + String(turbidityNTU, 0) +
                  ",\"ec\":" + String(ecValue, 1) +
                  ",\"ph\":" + String(phValue, 2) + "}";

    Serial.println("Sending JSON: " + json);

    int attempts = 0;
    bool sent = false;
    sendAT("AT+HTTPINIT", "OK");
   // data na gele koyekbar re-try korbe
    while (attempts < maxRetries && !sent) {
      if (
        sendAT("AT+HTTPPARA=\"URL\",\"" + serverURL + "\"", "OK") &&
        sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK")) {
        SIM.print("AT+HTTPDATA="); SIM.print(json.length()); SIM.println(",10000"); delay(500);
        if (waitFor("DOWNLOAD")) {
          SIM.print(json); delay(500);
          if (sendAT("AT+HTTPACTION=1", "OK", 5000)) {
            sent = true;
            Serial.println("✅ Data Sent Successfully");

            // Data gese, buzzer beep
            for (int i = 0; i < 50; i++)
            {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(500);
            digitalWrite(BUZZER_PIN, LOW);
            delay(500);
            }
          }
        }
      }
      if (!sent) { Serial.println("❌ Send failed. Retrying..."); delay(1000); attempts++; }
    }

    sendAT("AT+HTTPTERM", "OK");
    Serial.println("---- Done ----");

    dataSent = true;
  }
}
