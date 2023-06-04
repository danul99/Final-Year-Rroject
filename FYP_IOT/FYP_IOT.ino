#include "DHT.h"
#include "MQ7.h"
#include <MQ135.h>
#include <GP2YDustSensor.h>
#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <TinyGPS++.h>
TinyGPSPlus gps;
HardwareSerial SerialGPS(1);
#include <LiquidCrystal_I2C.h>
#include <CuteBuzzerSounds.h>

const uint8_t SHARP_LED_PIN = 26;   // Sharp Dust/particle sensor Led Pin (bule)
const uint8_t SHARP_VO_PIN = 27;    // Sharp Dust/particle analog out pin used for reading (purple)

#define DHTPIN 4     
#define DHTTYPE DHT11
#define A_PIN 34
#define VOLTAGE 5
#define BUZZER_PIN 13
#define PIN_MQ135 15

/* 1. Define the WiFi credentials */
//#define WIFI_SSID "SLT-ADSL-5E7DA"
//#define WIFI_PASSWORD "telecom2121"

#define WIFI_SSID "Galaxy M20EA37"
#define WIFI_PASSWORD "Danul098"

//For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */
#define API_KEY "AIzaSyCqWsosYZr3LAH6eEt2EChsTI03q410EyY"

/* 3. Define the RTDB URL */
#define DATABASE_URL "https://myfypapp-ac98b-default-rtdb.firebaseio.com/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

DHT dht(DHTPIN, DHTTYPE);
MQ7 mq7(A_PIN, VOLTAGE);
MQ135 mq135_sensor(PIN_MQ135);

GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1010AU0F, SHARP_LED_PIN, SHARP_VO_PIN);



float COsensorValue = 0;
float NO2 = 0;
float NO2Value = 0;
float latitude , longitude;
String  lat_str , lng_str;
float AQICO,AQINO2,AQIDD,AQIIndex;
float DustDensity;

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

void setup() {
  Serial.begin(115200);
  SerialGPS.begin(9600, SERIAL_8N1, 16, 17);
  
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();

 pinMode(13, OUTPUT);

  lcd.setCursor(3,0);
  lcd.print("Atmospheric");
  lcd.setCursor(4,1);
  lcd.print("Condition");
  delay(1500);
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("Monitoring");
  lcd.setCursor(5,1);
  lcd.print("System");
  delay(1500);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Connecting to");
  lcd.setCursor(3,1);
  lcd.print("Wi-Fi....");
  delay(1500);
  
  
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Connected to");
  lcd.setCursor(5,1);
  lcd.print("Wi-Fi");
  delay(1500);

  cute.init(BUZZER_PIN);

  cute.play(S_HAPPY);
  delay(500);

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  
  Serial.println(F("DHTxx test!"));
 
  /*while (!Serial) {
    ; // wait for serial connection
  }*/

  Serial.println("Calibrating MQ7");
  
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Calibrating");
  lcd.setCursor(1,1);
  lcd.print("MQ7 & MQ135...");
  delay(1500);
  
  mq7.calibrate();    // calculates R0
  Serial.println("Calibration done!");
  
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Calibration");
  lcd.setCursor(5,1);
  lcd.print("Done!!");
  delay(1500);
  

  dht.begin();

  dustSensor.begin();

 

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  config.database_url = DATABASE_URL;

  //////////////////////////////////////////////////////////////////////////////////////////////
  //Please make sure the device free Heap is not lower than 80 k for ESP32 and 10 k for ESP8266,
  //otherwise the SSL connection will fail.
  //////////////////////////////////////////////////////////////////////////////////////////////

  Firebase.begin(DATABASE_URL, API_KEY);

  //Comment or pass false value when WiFi reconnection will control by your code or third party library
 // Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);


}

void loop() {
  // Wait a few seconds between measurements.
  delay(2000);

  gpsdata();
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Failed to read");
  lcd.setCursor(2,1);
  lcd.print("the sensors!");
  delay(1500);
  
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println("%"); 
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println("°C ");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.println("°C ");

 COsensorValue = mq7.readPpm();
 Serial.print("CO Value: ");
 Serial.println(COsensorValue);

  float resistance = mq135_sensor.getResistance();
  float NO2Value = mq135_sensor.getPPM();;


  Serial.print("Dust density: ");
  Serial.print(dustSensor.getDustDensity());
  Serial.print(" ug/m3");

  
  
  AQI();
  gpsdata();
  
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("CO level:");
  lcd.setCursor(6,1);
  lcd.print(COsensorValue);
  delay(3000);
  lcd.clear();

  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("NO2 level:");
  lcd.setCursor(6,1);
  lcd.print(NO2Value);
  delay(3000);
  lcd.clear();

  lcd.setCursor(2, 0);
  lcd.print("Dust Density:");
  lcd.setCursor(5,1);
  lcd.print(DustDensity);
  delay(3000);
  lcd.clear();

  lcd.setCursor(4, 0);
  lcd.print("Humidity:");
  lcd.setCursor(6,1);
  lcd.print(h);
  delay(3000);
  lcd.clear();

  lcd.setCursor(2, 0);
  lcd.print("Temperature:");
  lcd.setCursor(6,1);
  lcd.print(t);
  delay(3000);
  lcd.clear();

  lcd.setCursor(2, 0);
  lcd.print("Heat Index:");
  lcd.setCursor(5,1);
  lcd.print(hic);
  delay(3000);
  lcd.clear();

  lcd.setCursor(3, 0);
  lcd.print("AQI Value:");
  lcd.setCursor(5,1);
  lcd.print(AQIIndex);
  delay(3000);
  

  if(AQIIndex>=300){
  digitalWrite(13, HIGH); 
  delay(1000);       
  digitalWrite(13, LOW);     
  delay(1000);
  digitalWrite(13, HIGH); 
  delay(1000);       
  digitalWrite(13, LOW);     
  delay(1000);
  digitalWrite(13, HIGH); 
  delay(1000);       
  digitalWrite(13, LOW);     
  delay(1000);
  digitalWrite(13, HIGH); 
  delay(1000);       
  digitalWrite(13, LOW);     
  delay(1000);       
  }
  else{
    pinMode(13,LOW);
  }

  AQStatment();
  
if (Firebase.ready()) 
  {

    Serial.print("Firebase Ready");
    
    Firebase.setFloat(fbdo, "/SensorUnit/CO_level", COsensorValue);
    Firebase.setFloat(fbdo, "/SensorUnit/NO2_level", NO2Value);
    Firebase.setFloat(fbdo, "/SensorUnit/Dust_Density",DustDensity);
    Firebase.setFloat(fbdo, "/SensorUnit/Temperture", t );
    Firebase.setFloat(fbdo, "/SensorUnit/Humidity", h );
    Firebase.setFloat(fbdo, "/SensorUnit/Heat_Index", hic );
    Firebase.setFloat(fbdo, "/SensorUnit/Latitude", latitude );
    Firebase.setFloat(fbdo, "/SensorUnit/Longitude", longitude );
    Firebase.setFloat(fbdo, "/SensorUnit/AQIIndex", AQIIndex );
    
    delay(200);

  delay(2500);
    }
}



void gpsdata(){
  while (SerialGPS.available() > 0) {
    if (gps.encode(SerialGPS.read()))
    {
      if (gps.location.isValid())
      {
        latitude = gps.location.lat();
        lat_str = String(latitude , 6);
        longitude = gps.location.lng();
        lng_str = String(longitude , 6);
        Serial.print("Latitude = ");
        Serial.println(lat_str);
        Serial.print("Longitude = ");
        Serial.println(lng_str);
        break;
        Serial.println("");
      }
      delay(1000);
    }
  }
}

void AQI (){
  if (COsensorValue <= 1){
      AQICO = COsensorValue * 50 / 1;
    }
        
    else if (COsensorValue <= 2){
      AQICO = 50 + (COsensorValue - 1) * 50 / 1;
    }
        
    else if (COsensorValue <= 10){
     AQICO = 100 + (COsensorValue - 2) * 100 / 8;
    }
        
    else if (COsensorValue <= 17){
      AQICO = 200 + (COsensorValue - 10) * 100 / 7;
    }
        
    else if (COsensorValue <= 34){
       AQICO = 300 + (COsensorValue - 17) * 100 / 17;
    }
       
    else if (COsensorValue> 34){
       AQICO = 400 + (COsensorValue - 34) * 100 / 17;
    }

    if (NO2Value <= 40){
      AQINO2 = NO2Value * 50 / 40;
    
    }
    else if (NO2Value <= 80){
       AQINO2 = 50 + (NO2Value - 40) * 50 / 40;
    }
        
    else if (NO2Value <= 180){
      AQINO2 = 100 + (NO2Value - 80) * 100 / 100;
    }
        
    else if (NO2Value <= 280){
       AQINO2 = 200 + (NO2Value - 180) * 100 / 100;
    }
       
    else if (NO2Value <= 400){
       AQINO2 = 300 + (NO2Value - 280) * 100 / 120;
    }
       
    else if (NO2Value > 400){
       AQINO2 = 400 + (NO2Value - 400) * 100 / 120;
    }
       

    if (DustDensity <= 50){
      AQIDD = DustDensity;
    }
        
    else if (DustDensity <= 100){
      AQIDD = DustDensity;
    }
            
    else if (DustDensity <= 250){
      AQIDD = 100 + (DustDensity - 100) * 100 / 150;
    }
        
    else if (DustDensity <= 350){
      AQIDD = 200 + (DustDensity - 250);
    }
        
    else if (DustDensity <= 430){
      AQIDD = 300 + (DustDensity - 350) * 100 / 80;
    }
        
    else if (DustDensity > 430){
      AQIDD = 400 + (DustDensity - 430) * 100 / 80;
    }
        
       
    AQIIndex = (AQICO+ AQINO2 + AQIDD)/3;
}

  void AQStatment(){

     if (AQIIndex <= 50){
       lcd.clear();
       lcd.setCursor(1, 0);
       lcd.print("Air Condition:");
       lcd.setCursor(6,1);
       lcd.print("Good");
       delay(3000);
       
    }
        
    else if (AQIIndex <= 100){
       lcd.clear();
       lcd.setCursor(1, 0);
       lcd.print("Air Condition:");
       lcd.setCursor(2,1);
       lcd.print("Satisfactory");
       delay(3000);
       
    }
            
    else if (AQIIndex <= 200){
       lcd.clear();
       lcd.setCursor(1, 0);
       lcd.print("Air Condition:");
       lcd.setCursor(4,1);
       lcd.print("Moderate");
       delay(3000);
       
    }
        
    else if (AQIIndex <= 300){
       lcd.clear();
       lcd.setCursor(1, 0);
       lcd.print("Air Condition:");
       lcd.setCursor(6,1);
       lcd.print("Poor");
       delay(3000);
       
    }
        
    else if (AQIIndex <= 400){
       lcd.clear();
       lcd.setCursor(1, 0);
       lcd.print("Air Condition:");
       lcd.setCursor(4,1);
       lcd.print("Very Poor");
       delay(3000);
     
    }
        
    
    else  {
      lcd.clear();
       lcd.setCursor(1, 0);
       lcd.print("Air Condition:");
       lcd.setCursor(5,1);
       lcd.print("Severe");
       delay(3000);
      
    }
  
}
