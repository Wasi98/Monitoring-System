#include <SoftwareSerial.h>
#include <AltSoftSerial.h>
#include <TinyGPS++.h>


SoftwareSerial gsmSerial(2, 3); // RX, TX pins for GSM module
AltSoftSerial ngps;
TinyGPSPlus gps;

String message;
String response;
float latitude, longitude;
float distance;
void getGps(float& latitude, float& longitude);

//geofence
boolean send_alert_once = true;
const float maxDistance = 30;
float initialLatitude = 23.812357;
float initialLongitude = 90.291245;

unsigned long previousTime = 0;
unsigned long interval = 5000;


void setup() {
  Serial.begin(9600); // Serial monitor for debugging
  ngps.begin(9600);
  gsmSerial.begin(9600); // GSM module baud rate
  delay(1000);
  Serial.println("Initializing GSM module...");
  gsmSerial.println("AT");
  waitForResponse();

  gsmSerial.println("ATE1");
  waitForResponse();

  gsmSerial.println("AT+CNMI=1,2,0,0,0");
  waitForResponse();

  gsmSerial.println("AT+CMGF=1"); // Set SMS text mode
  waitForResponse();
  Serial.println("GSM module ready!");
}

void loop() {

   //get location command part
    while(gsmSerial.available()) {
     //Serial.println("Get command found"); 
    message = gsmSerial.readString(); //reads the incoming messages to GSM
    if (message.indexOf("Get Location") != -1) { // Replace with your own keyword
      //getGps(latitude, longitude);

      if(distance > maxDistance){
         response = "Outside the fence & location is:.\r";
      response += "http://maps.google.com/maps?q=loc:";
      response += String(latitude,6) + "," + String(longitude,6);
      sendResponse(response);
      delay(3000);
      }
      else{
        response = "Inside the fence & location is:\r";
      response += "http://maps.google.com/maps?q=loc:";
      response += String(latitude,6) + "," + String(longitude,6);
      sendResponse(response);
      //Serial.println("Get command found");
      delay(3000);
      }
    }
  }
   //fence checking part
   if (millis() - previousTime >= interval) {
    previousTime = millis();
    runFence();
   }
}


void sendResponse(String msg){
      gsmSerial.print("AT+CMGS=\"+8801682593520\"\r"); // Replace with your own phone number
      delay(1000);
      gsmSerial.print(msg);
      delay(1000);
      gsmSerial.write(0x1A); // Send Ctrl+Z to end message
      delay(1000);
      Serial.println("SMS received and responded to.");
      clearSMS();
      //delay(3000);
}

void waitForResponse(){
  delay(1000);
  while(gsmSerial.available()){
    Serial.println(gsmSerial.readString());
  }
  gsmSerial.read();
}

void getGps(float& latitude, float& longitude)
{
    // Can take up to 60 seconds
  bool newData = false;
  //less than 2seconds delay
    for(unsigned long start = millis(); millis() - start < 2000;){ //less than 2seconds delay
    while (ngps.available()){ //for gps serial data check
      if (gps.encode(ngps.read())){ //for gps data read
        newData = true;
        break;
      }
    }
  }
  
  if (newData) //If newData is true
  {
    latitude = gps.location.lat(),6;
    longitude = gps.location.lng(),6;
    newData = false;
  }
  else {
    Serial.println("No GPS data is available");
    latitude = 0;
    longitude = 0;
  }
}


// Calculate distance between two points
float getDistance(float flat1, float flon1, float flat2, float flon2) {

  // Variables
  float dist_calc=0;
  float dist_calc2=0;
  float diflat=0;
  float diflon=0;

  // Calculations
  diflat  = radians(flat2-flat1);
  flat1 = radians(flat1);
  flat2 = radians(flat2);
  diflon = radians((flon2)-(flon1));

  dist_calc = (sin(diflat/2.0)*sin(diflat/2.0));
  dist_calc2 = cos(flat1);
  dist_calc2*=cos(flat2);

  dist_calc2*=sin(diflon/2.0);
  dist_calc2*=sin(diflon/2.0);
  dist_calc +=dist_calc2;

  dist_calc=(2*atan2(sqrt(dist_calc),sqrt(1.0-dist_calc)));
  
  dist_calc*=6371000.0; //Converting to meters

  return dist_calc;
}

void runFence(){
    getGps(latitude, longitude);
    distance = getDistance(latitude, longitude, initialLatitude, initialLongitude); //checking function of geofencing

   if(distance > maxDistance) {
    if(send_alert_once == true){

      response = "Alert! The object is outside of the fence.\r";
      response += "http://maps.google.com/maps?q=loc:";
      response += String(latitude,6) + "," + String(longitude,6);
      sendResponse(response);
      Serial.println(response);
      send_alert_once = false;
    }
  }
  else{
    send_alert_once = true;
    Serial.println("Inside the fence");
  }
}

void clearSMS() {
  // Send AT command to delete all SMS messages
  gsmSerial.println("AT+CMGD=1,4");
  delay(1000);
  /*while (gsmSerial.available()) { 
    Serial.write(gsmSerial.read()); 
  }*/
}

