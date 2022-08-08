#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//Initialize user modifiable variables
int hoseOn1 = 4;        //GPIO4 is D2 on the board
int buttonIn1 = 5;      //GPIO5 is D1 on the board
int hoseOnHour1 = 6;
int hoseOnMinute1 = 0;
int hoseOn2 = 12;        //GPIO12 is D6 on the board
int buttonIn2 = 14;      //GPIO14 is D4 on the board
int hoseOnHour2 = 6;
int hoseOnMinute2 = 15;
long dwellMin = 15;
long debounce = 200; 
const char *ssid     = "Skynetwork";
const char *password = "Ri9Is9Lo3";

//Initialize non-user modifiable variables
bool test = false;
bool state = false;
bool autoStart = false;
long dwellMillis;
long clockUpdateTime;
long minMillis;
long hourMillis;
int hours;
int minutes;
String day;
unsigned long onTime1;
unsigned long onTime2;
unsigned long myTime;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Initialize constants
long millisPerSec = 1000;
long secPerMin = 60;
long minPerHour = 60;
unsigned long clockTimer = 0;
volatile unsigned long last_millis = 0;
const long utcOffsetInSeconds = -14400;

//Initialize objects
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//Declare functions
void getTime();
void hoseOn(int);
void hoseOff(int);
void hoseToggle();

void setup(){

  //Set hardware
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(hoseOn1, OUTPUT);
  digitalWrite(hoseOn1, LOW);
  pinMode(buttonIn1, INPUT);
  pinMode(hoseOn2, OUTPUT);
  digitalWrite(hoseOn2, LOW);
  pinMode(buttonIn2, INPUT);
  Serial.begin(115200);

  //Set interrupt for debouncing PB    
  attachInterrupt(digitalPinToInterrupt(buttonIn1), debounceButt, RISING);
  attachInterrupt(digitalPinToInterrupt(buttonIn2), debounceButt, RISING);

  //Calculate program constants
  dwellMillis = millisPerSec*secPerMin*dwellMin;
  minMillis = millisPerSec*secPerMin;
  hourMillis = millisPerSec*secPerMin*minPerHour;  
  clockUpdateTime = millisPerSec*secPerMin;

  //Initialize wi-fi
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  //Initialize time values
  timeClient.begin();
  getTime();
}

void loop() {

  //Update time as per specified period
  if (millis()- clockTimer >= clockUpdateTime)
    getTime();

  //Get 1 second value
  myTime = millis()%1000;

  //Set variable false if not exactly 1 second
  if (myTime != 0 ){
    test = false;
  }

  //If exactly 1 second has passed, toggle onboard LED and display runtime
  if (myTime == 0 && !test){
    test = true;
    state=!state;
    digitalWrite(LED_BUILTIN, state);
    if (digitalRead(hoseOn1)){
      
      Serial.print("Sprinkler1 on time is ");
      Serial.print((millis()- onTime1)/(millisPerSec*secPerMin));
      Serial.print(" minute(s) and ");
      Serial.print((millis()- onTime1)%(millisPerSec*secPerMin)/millisPerSec);
      Serial.println(" second(s)");
    }
    
    if (digitalRead(hoseOn2)){
      
      Serial.print("Sprinkler2 on time is ");
      Serial.print((millis()- onTime1)/(millisPerSec*secPerMin));
      Serial.print(" minute(s) and ");
      Serial.print((millis()- onTime1)%(millisPerSec*secPerMin)/millisPerSec);
      Serial.println(" second(s)");
    }
  }

  //Set hose on if current time matches requested time
  if (hours == hoseOnHour1 && minutes == hoseOnMinute1 && !autoStart){
    autoStart = true;
    hoseOn(hoseOn1, onTime1);
  }

  //Turn hose off after 15 minutes regardless of requested start source
  if (millis() - onTime1 >= dwellMillis && digitalRead(hoseOn1)){
    autoStart = false;
    hoseOff(hoseOn1);
  }

  //Set hose on if current time matches requested time
  if (hours == hoseOnHour2 && minutes == hoseOnMinute2 && !autoStart){
    autoStart = true;
    hoseOn(hoseOn2, onTime2);
  }

  //Turn hose off after 15 minutes regardless of requested start source
  if (millis() - onTime2 >= dwellMillis && digitalRead(hoseOn2)){
    autoStart = false;
    hoseOff(hoseOn2);
  }

}

void getTime(){ //Function to perform time maintenance
  
  timeClient.update();
  clockTimer = millis();
  day = daysOfTheWeek[timeClient.getDay()];
  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();

  Serial.print(day);
  Serial.print(", ");
  Serial.print(hours);
  Serial.print(":");
  Serial.println(minutes);
  
  return;
}

void hoseOn(int relay, unsigned long timeT){ //Function to turn hose ON, initialize start time, and write out ON! message

  digitalWrite(relay, HIGH);
  Serial.write("ON!\n");
  timeT = millis();
  
  return;
    
}

void hoseOff(int relay){ //Function to turn hose OFF and write out OFF! message

  digitalWrite(relay, LOW);
  Serial.write("OFF!\n");
  
  return;
    
}

void hoseToggle(int relay){ //Function called by button debounce to toggle current state of hose

  if (!digitalRead(relay))
    hoseOn(relay);
  
  else 
    hoseOff(relay);
    
  return;
    
}

ICACHE_RAM_ATTR void debounceButt(){ //Debounce function called as ISR by interrupt attached to PB

  if((long)(millis() - last_millis) >= debounce) {
    hoseToggle();
    last_millis = millis();
  }
  
}
