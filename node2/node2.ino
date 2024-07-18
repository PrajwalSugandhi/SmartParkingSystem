// Include Libraries
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "ThingSpeak.h"
#include <DHT.h>
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>


LiquidCrystal_I2C lcd(0x27, 16, 2);

// Define 
#define DHTPin 12
#define DHTType DHT22
#define IRPin 13
#define threshold 70

// Create DHT Object
DHT dht(DHTPin, DHTType);


// Variables for temperature
float temp;

uint8_t broadcastAddress[] = {0xC8, 0xC9, 0xA3, 0x56, 0x6D, 0xF1};

// Define data structure 
typedef struct struct_message {
  float temp;
  int slot;
  int free_space;
} struct_message;

// Create structured data object
struct_message myData;
struct_message receivedData;

//setting up thingspeak
unsigned long myChannelNumber = 1;
const char* apiKey = "37JDSIZKAKJCTVO7";     // API key from ThingSpeak
 
const char *ssid =  "realme GT";     // Wifi ssid and wpa2 key
const char *pass =  "Prajwal@123";
const char* server = "api.thingspeak.com";


WiFiClient client;

void OnDataSent(uint8_t *mac_addr, uint8_t status)
{
  Serial.println("Status:");
  Serial.println(status);
}

// Callback function
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) 
{
  // Get incoming data
  memcpy(&receivedData, incomingData, sizeof(receivedData));  
}



void setup() {
 
  // Setup Serial monitor
  Serial.begin(9600);
  delay(100);
  
   WiFi.begin(ssid, pass);
 
      while (WiFi.status() != WL_CONNECTED) 
     {
            delay(500);
            Serial.print(".");
     }
  ThingSpeak.begin(client);

  //initialize lcd screen
  lcd.init();
  lcd.clear();         
  lcd.backlight();  
  //lcd.begin(16,2);  
  lcd.setCursor (0,0);
  lcd.print("  Car Parking ");
  lcd.setCursor (0,1);
  lcd.print("   System   ");
  delay (5000);
  
  // Initiate DHT22 
  dht.begin();

  //setting pin mode for ir sensors
  pinMode(IRPin, INPUT);
 
  // Set ESP8266 WiFi mode to Station
  WiFi.mode(WIFI_STA);
 
  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
 
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);


 
}


void loop() {
 
 
  // Read values
  temp = dht.readTemperature();
  delay(10);
  int sensorStatus = digitalRead(IRPin);
 
 //Printing data
  Serial.print("Temperature of 1st Spot: ");
  Serial.println(receivedData.temp);

  Serial.print("Temperature of 2nd Spot: ");
  Serial.println(temp);

  Serial.print("Status of 1st spot: ");
  if(receivedData.slot){
    Serial.println("Vacant");
  }
  else{
    Serial.println("Occupied");
  }

  Serial.print("Status of 2nd spot: ");
  if(sensorStatus){
    Serial.println("Vacant");
  }
  else{
    Serial.println("Occupied");
  }
  
  int x= ThingSpeak.writeField(myChannelNumber, 2, temp, apiKey);
  if(x == 200){
  Serial.println("Channel update successful.");
}
else{
  Serial.println("Problem updating channel. HTTP error code " + String(x));
}
 
  // Add to structured data object
  myData.temp = temp;
  myData.slot = sensorStatus;


  // Send data
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  
  //printing data
  Serial.print("Free Parking Slots available");
  Serial.println(receivedData.free_space);

  //lcd printing
  int space;
  lcd.setCursor (0,0);
  lcd.print(" Parking Slots ");
  space = receivedData.free_space ;
  
  lcd.setCursor(0, 1);
  lcd.print(" Available - ");
  lcd.print(space);


  //alarming if there is fire nearby
  if(receivedData.temp > threshold){
    Serial.println("Fire caught in the near parking spot. Run fighting mechanisms.");
  }
  
  delay(2000);
}