// Include required libraries
#include <ESP8266WiFi.h>
#include "ThingSpeak.h"
#include <espnow.h>
#include <DHT.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <BlynkSimpleEsp8266.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);


uint8_t broadcastAddress[] = {0x84, 0xf3, 0xeb, 0xe4, 0x4b, 0x8d};


Servo myservo;

// Define 
#define BLYNK_TEMPLATE_ID           "TMPL3hcZLjLs9"
#define BLYNK_TEMPLATE_NAME         "Quickstart Device"
#define BLYNK_AUTH_TOKEN            "YJmDqi7T6Clrh9J85L2GZR2G20ao7iJY"
#define BLYNK_PRINT Serial
#define DHTPin 12
#define DHTType DHT22
#define IR_enter 4
#define IRPin 14
#define IR_exit 13
#define threshold 70


//setting up thingspeak
unsigned long myChannelNumber = 1;
const char* apiKey = "37JDSIZKAKJCTVO7";     //  API key from ThingSpeak
 
const char *ssid =  "realme GT";     //  wifi ssid and wpa2 key
const char *pass =  "Prajwal@123";
const char* server = "api.thingspeak.com";

// Create DHT Object
DHT dht(DHTPin, DHTType);



WiFiClient client;

// Define data structure
typedef struct struct_message {
    float temp;
    int slot;
    int free_space;
} struct_message;

// Create structured data object
struct_message receivedData;
struct_message myData;

// Callback function
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) 
{
  // Get incoming data
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  
}


void OnDataSent(uint8_t *mac_addr, uint8_t status)
{
  Serial.println("Status:");
  Serial.println(status);
}


//defining variables for future use
int flag1=0, flag2=0;
int slot = 2;



void setup() {
  // Set up Serial Monitor
  Serial.begin(9600);
  dht.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, "realme GT", "Prajwal@123");

 
  // Start ESP8266 in Station mode
  WiFi.mode(WIFI_STA);

  //setting pin mode for ir sensors
  pinMode(IRPin, INPUT);
  pinMode(IR_enter, INPUT);
  pinMode(IR_exit, INPUT);


  //servo motor initialize
  myservo.attach(0);
  myservo.write(90);




  // Initalize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
   
  
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  WiFi.begin(ssid, pass);
 
      while (WiFi.status() != WL_CONNECTED) 
     {
            delay(500);
            Serial.print(".");
     }
  ThingSpeak.begin(client);

}

void loop() {

  // Read values
  float temp2 = dht.readTemperature();
  delay(50);
  int entry = digitalRead(IR_enter);
  Blynk.run();

  int x = ThingSpeak.writeField(myChannelNumber, 1, temp2, apiKey);
  if(x == 200){
  Serial.println("Channel update successful.");
}
else{
  Serial.println("Problem updating channel. HTTP error code " + String(x));
}

 
  //printing data
  Serial.print("Temperature of 1st Spot: ");
  Serial.println(temp2);
  Serial.print("Temperature of 2nd Spot: ");
  Serial.println(receivedData.temp);

  
  Blynk.virtualWrite(V0, receivedData.slot);



  int spot1 = digitalRead(IRPin);
  int exit = digitalRead(IR_exit);

  Serial.print("Status of 1st spot: ");
  Blynk.virtualWrite(V1, spot1);

  if(spot1){
    Serial.println("Vacant");
  }
  else{
    Serial.println("Occupied");
  }
  
  Serial.print("Status of 2nd spot: ");
  if(receivedData.slot){
    Serial.println("Vacant");
  }
  else{
    Serial.println("Occupied");
  }

  Serial.print("Status of entry gate sensor: ");
  if(entry){
    Serial.println("Vacant");
  }
  else{
    Serial.println("Occupied");
  }


  Serial.print("Status of exit gate sensor: ");
  if(exit){
    Serial.println("Vacant");
  }
  else{
    Serial.println("Occupied");
  }
  

  Serial.print("Free Spaces Available: ");
  Serial.println(slot);
  
  ThingSpeak.writeField(myChannelNumber, 3, slot, apiKey);

  //gate open and close code
  if (digitalRead(IR_enter) == 0 && flag1 == 0) {
   if (slot > 0) {
      flag1 = 1;
      if (flag2 == 0) {
         myservo.write(180);
         slot = slot - 1;
         myData.free_space = slot;
         esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
         delay(800);
      }
   } else {
      delay(800);
   }
  }

  

if (digitalRead(IR_exit) == 0 && flag2 == 0) {
   flag2 = 1;
   if (flag1 == 0) {

      myservo.write(180);
      slot = slot + 1;
      myData.free_space = slot;
      esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
      delay(800);
   }
}

if (flag1 == 1 && flag2 == 1) {
   delay(1000);
   myservo.write(90);
   flag1 = 0, flag2 = 0;
   delay(800);
}

  // Add to structured data object 
  myData.temp = temp2;
  myData.slot = spot1;
  myData.free_space = slot;
  

  //send data
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  delay(2000);
 
 //alarming if there is fire nearby
  if(receivedData.temp > threshold){
    Serial.println("Fire caught in the near parking spot. Run fighting mechanisms.");
  }

}
