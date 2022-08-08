#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//Initialize user modifiable variables
int hoseOn1 = 4;        //GPIO4 is D2 on the board
int buttonIn1 = 5;      //GPIO5 is D1 on the board
int hoseOnHour1 = 6;
int hoseOnMinute1 = 0;
int hoseOn2 = 12;        //GPIO12 is D6 on the board
int buttonIn2 = 14;      //GPIO14 is D5 on the board
int hoseOnHour2 = 6;
int hoseOnMinute2 = 15;
long dwellMin = 15;
long debounce = 200; 

//Initialize Networking
const char *ssid     = "Skynetwork";
const char *password = "Ri9Is9Lo3";
IPAddress local_IP(192, 168, 4, 220);
IPAddress subnet(255, 255, 252, 0);
IPAddress gateway(192, 168, 4, 1);
WiFiServer server(80);
String header;

// Auxiliar variables to store the current output state
String output1State = "off";
String output2State = "off";

// Assign output variables to GPIO pins
const int output1 = 1;
const int output2 = 2;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

//Initialize non-user modifiable variables
bool test = false;
bool state = false;
bool autoStart1 = false;
bool autoStart2 = false;
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
void hoseOn(int, unsigned long *);
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
  // Initialize the output variables as outputs
  pinMode(output1, OUTPUT);
  pinMode(output2, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output1, LOW);
  digitalWrite(output2, LOW);
  Serial.begin(115200);

  //Set interrupt for debouncing PB    
  attachInterrupt(digitalPinToInterrupt(buttonIn1), debounceButt1, RISING);
  attachInterrupt(digitalPinToInterrupt(buttonIn2), debounceButt2, RISING);

  //Calculate program constants
  dwellMillis = millisPerSec*secPerMin*dwellMin;
  minMillis = millisPerSec*secPerMin;
  hourMillis = millisPerSec*secPerMin*minPerHour;  
  clockUpdateTime = millisPerSec*secPerMin;

  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

  //Initialize wi-fi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED ) {
    delay (500);
    Serial.print (".");
  }

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  //Initialize time values
  timeClient.begin();
  getTime();
}

void loop() {
  
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /5/on") >= 0) {
              Serial.println("GPIO 5 on");
              output1State = "on";
              digitalWrite(output1, HIGH);
            } else if (header.indexOf("GET /5/off") >= 0) {
              Serial.println("GPIO 5 off");
              output1State = "off";
              digitalWrite(output1, LOW);
            } else if (header.indexOf("GET /4/on") >= 0) {
              Serial.println("GPIO 4 on");
              output2State = "on";
              digitalWrite(output2, HIGH);
            } else if (header.indexOf("GET /4/off") >= 0) {
              Serial.println("GPIO 4 off");
              output2State = "off";
              digitalWrite(output2, LOW);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP8266 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 5  
            client.println("<p>GPIO 5 - State " + output1State + "</p>");
            // If the output1State is off, it displays the ON button       
            if (output1State=="off") {
              client.println("<p><a href=\"/5/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 4  
            client.println("<p>GPIO 4 - State " + output2State + "</p>");
            // If the output2State is off, it displays the ON button       
            if (output2State=="off") {
              client.println("<p><a href=\"/4/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/4/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }

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
      Serial.print((millis()- onTime2)/(millisPerSec*secPerMin));
      Serial.print(" minute(s) and ");
      Serial.print((millis()- onTime2)%(millisPerSec*secPerMin)/millisPerSec);
      Serial.println(" second(s)");
    }
  }

  //Set hose on if current time matches requested time
  if (hours == hoseOnHour1 && minutes == hoseOnMinute1 && !autoStart1){
    autoStart1 = true;
    hoseOn(hoseOn1, &onTime1);
  }

  //Turn hose off after 15 minutes regardless of requested start source
  if (millis() - onTime1 >= dwellMillis && digitalRead(hoseOn1)){
    autoStart1 = false;
    hoseOff(hoseOn1);
  }

  //Set hose on if current time matches requested time
  if (hours == hoseOnHour2 && minutes == hoseOnMinute2 && !autoStart2){
    autoStart2 = true;
    hoseOn(hoseOn2, &onTime2);
  }

  //Turn hose off after 15 minutes regardless of requested start source
  if (millis() - onTime2 >= dwellMillis && digitalRead(hoseOn2)){
    autoStart2 = false;
    hoseOff(hoseOn2);
  }

}

void getTime(){ //Function to perform time maintenance
  
  timeClient.update();
  clockTimer = millis();
  day = daysOfTheWeek[timeClient.getDay()];
  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  
  return;
}

void hoseOn(int relay, unsigned long *timeT){ //Function to turn hose ON, initialize start time, and write out ON! message

  digitalWrite(relay, HIGH);
  Serial.write("ON!\n");
  *timeT = millis();
  
  return;
    
}

void hoseOff(int relay){ //Function to turn hose OFF and write out OFF! message

  digitalWrite(relay, LOW);
  Serial.write("OFF!\n");
  
  return;
    
}

void hoseToggle(int relay, unsigned long *timeT){ //Function called by button debounce to toggle current state of hose

  if (!digitalRead(relay))
    hoseOn(relay, timeT);
  
  else 
    hoseOff(relay);
    
  return;
    
}

ICACHE_RAM_ATTR void debounceButt1(){ //Debounce function called as ISR by interrupt attached to PB

  if((long)(millis() - last_millis) >= debounce) {
    hoseToggle(hoseOn1, &onTime1);
    last_millis = millis();
  }
  
}

ICACHE_RAM_ATTR void debounceButt2(){ //Debounce function called as ISR by interrupt attached to PB

  if((long)(millis() - last_millis) >= debounce) {
    hoseToggle(hoseOn2, &onTime2);
    last_millis = millis();
  }
  
}
