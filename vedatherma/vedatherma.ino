
//--------------------------------------------IMPORTING LIBRARIES------------------------------------------------------------------------------------------------------//

#include <SPIMemory.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include "Wire.h"

//--------------------------------------------IMPORTING LIBRARIES------------------------------------------------------------------------------------------------------//



//--------------------------------------------INITIALIZING PINS------------------------------------------------------------------------------------------------------//

#define CHIP_SELECT_PIN 5   // Pin connected to the SPI Flash memory chip select
// #define SENSOR1 13  // Pin connected to the temperature sensor
// #define SENSOR2 33          //D4 pin of nodemcu
// #define SENSOR3 27          //D1 pin of nodemcu
// #define SENSOR4 26          //D2 pin of nodemcu
const int ONE_WIRE_BUS[] = {33, 27, 26, 13};
const int SENSOR_COUNT = 4;
//--------------------------------------------INITIALIZING PINS------------------------------------------------------------------------------------------------------//



//--------------------------------------------INITIALIZING OBJECTS------------------------------------------------------------------------------------------------------//

RTC_DS3231 rtc;                    // RTC Object
SPIFlash memory;                   // Initialize SPI Flash memory
// OneWire oneWire(SENSOR1);  // Onewire object
// OneWire oneWire2(SENSOR2);
// OneWire oneWire3(SENSOR3);
// OneWire oneWire4(SENSOR4);

LiquidCrystal_I2C lcd(0x27, 20, 4);
// DallasTemperature sensors(&oneWire);  // Temperature Sensor object
// DallasTemperature sensee2(&oneWire2);
// DallasTemperature sensee3(&oneWire3);
// DallasTemperature sensee4(&oneWire4);

OneWire* oneWire[SENSOR_COUNT];
DallasTemperature* sensors[SENSOR_COUNT];

WiFiClient client;                    // Initialising the WiFi Server Object
IPAddress ip(192, 168, 19, 2);        // IP Address of the server
IPAddress gateway(192, 168, 19, 2);   // Gateway same as the Client Device
IPAddress netmask(255, 255, 255, 0);  // Netmask of the Server
AsyncWebServer server(5210);          // Initialising the WebServer at port 5210

//--------------------------------------------INITIALIZING OBJECTS------------------------------------------------------------------------------------------------------//


 
//-------------------------------------------------DEVICE ID------------------------------------------------------------------------------------------------------//

const uint16_t Device_ID = 2241;  // DEVICE ID

//-------------------------------------------------DEVICE ID------------------------------------------------------------------------------------------------------//



//--------------------------------------------INITIALIZING VARIABLES------------------------------------------------------------------------------------------------------//

float temperatures[4];
uint32_t address = 16;      // Starting address to write data
uint8_t LastLocation = 0;  // Memory location to check if rom is empty

bool logState0 = false;
bool logState30 = false;
bool Boot = true;

unsigned long hotspotTimer = 0;  // Timer to track hotspot intervals
bool hotspotEnabled = true;      // Flag to track hotspot state

const char *ssid = "VEDA THERMA 2241";  // ESP SSID
const char *password = "12345678";      // ESP Password

//--------------------------------------------INITIALIZING VARIABLES------------------------------------------------------------------------------------------------------//



//------------------------------------------------TIME VARIABLES------------------------------------------------------------------------------------------------------//

unsigned long previousTime = 0;                          // variable that holds the previous time in millis
unsigned long currentTime;                               // variable that holds the current time in millis
const unsigned long eventInterval = 1800000;             // 30 mins into milliseconds that determine the interval after the data is stored into flash
//const unsigned long eventInterval = 30000;             // 30 secs into milliseconds that determine the interval after the data is stored into flash
const unsigned long hotspotOnInterval = 900000;          // 15 minutes in milliseconds that determine the time the hotspot is ON
const unsigned long hotspotOffInterval = 300000;         // 5 minutes in milliseconds that determine the time the hotspot is OFF

//------------------------------------------------TIME VARIABLES------------------------------------------------------------------------------------------------------//



//------------------------------------------------SETUP FUNCTION------------------------------------------------------------------------------------------------------//

void setup() {

  Serial.begin(9600);                        // Baud Rate 9600

  // sensors.begin();                           // Initialise sensors
  // sensee2.begin();
  // sensee3.begin();
  // sensee4.begin();

  // Initialize OneWire and DallasTemperature objects 
  for (int i = 0; i < SENSOR_COUNT; i++) {
    oneWire[i] = new OneWire(ONE_WIRE_BUS[i]);
    sensors[i] = new DallasTemperature(oneWire[i]);
    sensors[i]->begin();
  }
  
  lcd.init();                                // initialize lcd screen
  lcd.backlight();                           // turn on the backlight

  lcd.setCursor(3, 0);
  lcd.print("VEDANTRIK");
  lcd.setCursor(2, 1);
  lcd.print("TECHNOLOGIES");
  delay(2000);
  lcd.clear();

  if (!rtc.begin()) {                        // Check if RTC is available
    Serial.println("Couldn't find RTC");
    lcd.setCursor(0, 0);
    lcd.print("Couldn't find RTC");
    delay(2000);
  }

  memory.begin();                            // Initialize SPI Flash memory with chip select pin

  WiFi.mode(WIFI_AP);                  // Initialize ESP32 in AP Mode
  WiFi.softAPConfig(ip, ip, netmask);  // Passing the IP Address, Gateway Address (same as IP Address), and Netmask
  WiFi.softAP(ssid, password);         // Setting up the NodeMCU in AP Mode with Credentials defined above

  //--------------------------------------------SERVER ROUTE FUNCTIONS------------------------------------------------------------------------------------------------------//

  server.on("/Temp", HTTP_GET, [](AsyncWebServerRequest *request) {                         // Function handling the "/Temp" route
    DateTime now = rtc.now();  // Re-initialize the RTC module

    readTemperatures();
    // sensors.requestTemperatures();                    // Obtain the temperature data from the sensors object
    // float temperatureC = sensors.getTempCByIndex(0);  // Obtain the Temperature value at index "0" and store the temperature value into the variable

    String Str = "<!DOCTYPE html>\n";
    Str += "<html>\n";
    Str += "<body>\n";
    Str += "<style>\n";
    Str += ".interface canvas {\n";
    Str += "display: block;\n";
    Str += "margin: 0 auto;\n";
    Str += "}\n";

    Str += ".interface p{\n";
    Str += "background-color: #59D2FE;\n";
    Str += "text-align: left;\n";
    Str += "padding: 10px;\n";
    Str += "border-radius: 10px;\n";
    Str += "}\n";

    Str += ".interface h3{\n";
    Str += "background-color: #5C7AFF;\n";
    Str += "padding: 5px ;\n";
    Str += "text-align: center;\n";
    Str += "border-radius: 10px;\n";
    Str += "}\n";
    Str += "</style>\n";
    Str += "<div class=\"interface\">";
    Str += "<h3>Realtime Values</h3>\n";
    Str += "<p>Sensor 1: " + String(temperatures[0]) + " °C</p>\n";
    Str += "<p>Sensor 2: " + String(temperatures[1]) + " °C</p>\n";
    Str += "<p>Sensor 3: " + String(temperatures[2]) + " °C</p>\n";
    Str += "<p>Sensor 4: " + String(temperatures[3]) + " °C</p>\n";
    Str += "</div>\n";

    // JavaScript Auto Reload Function
    Str += "<script>\n";
    Str += "setTimeout(function(){\n";
    Str += "window.location.reload();\n";
    Str += "}, 1000);\n";
    Str += "</script>\n";
    Str += "</body>\n";
    Str += "</html>\n";
    request->send(200, "text/html", Str);
  });

  // Serve the HTML form
  server.on("/Date", HTTP_GET, [](AsyncWebServerRequest *request) {                         // Function handling the "/Date" route
    String html = "<!DOCTYPE html>\n"
                  "<html>\n"
                  "<body>\n"
                  "<script>\n"
                  "document.addEventListener('DOMContentLoaded', function() {\n"
                  "var today = new Date();\n"
                  "var dd = String(today.getDate()).padStart(2, '0');\n"
                  "var mm = String(today.getMonth() + 1).padStart(2, '0');\n"
                  "var yyyy = today.getFullYear();\n"
                  "var date = yyyy + '-' + mm + '-' + dd;\n"
                  "document.getElementById('dateInput').value = date;\n"
                  "var hours = String(today.getHours()).padStart(2, '0');\n"
                  "var minutes = String(today.getMinutes()).padStart(2, '0');\n"
                  "var time = hours + ':' + minutes;\n"
                  "document.getElementById('timeInput').value = time;\n"
                  "});\n"
                  "</script>\n"
                  "<form action=\"/get\" method=\"POST\">\n"
                  "<label for=\"dateInput\">Date:</label><br>\n"
                  "<input type=\"date\" id=\"dateInput\" name=\"inputDate\"><br><br>\n"
                  "<label for=\"timeInput\">Time:</label><br>\n"
                  "<input type=\"time\" id=\"timeInput\" name=\"inputTime\"><br><br>\n"
                  "<input type=\"submit\" value=\"Submit\">\n"
                  "</form>\n"
                  "</body>\n"
                  "</html>\n";
    request->send(200, "text/html", html);
  });

  // Handle form submission
  server.on("/get", HTTP_POST, [](AsyncWebServerRequest *request) {                         // Function handling the "/get" route
    if (request->hasParam("inputDate", true) && request->hasParam("inputTime", true)) {
      String inputDate = request->getParam("inputDate", true)->value();
      String inputTime = request->getParam("inputTime", true)->value();
      Serial.println("Received input date: " + inputDate);
      Serial.println("Received input time: " + inputTime);
      // Parse the date
      int inputDateLength = inputDate.length();
      char inputDateChar[inputDateLength + 1];  // +1 for the null terminator
      inputDate.toCharArray(inputDateChar, inputDateLength + 1);

      // Variables to store the parsed date values
      int yyyy, mm, dd;
      sscanf(inputDateChar, "%d-%d-%d", &yyyy, &mm, &dd);

      // Parse the time
      int inputTimeLength = inputTime.length();
      char inputTimeChar[inputTimeLength + 1];  // +1 for the null terminator
      inputTime.toCharArray(inputTimeChar, inputTimeLength + 1);

      // Variables to store the parsed time values
      int hours, minutes;
      sscanf(inputTimeChar, "%d:%d", &hours, &minutes);

      // Print the extracted values
      Serial.print("Year: ");
      Serial.println(yyyy);
      Serial.print("Month: ");
      Serial.println(mm);
      Serial.print("Day: ");
      Serial.println(dd);
      Serial.print("Hours: ");
      Serial.println(hours);
      Serial.print("Minutes: ");
      Serial.println(minutes);
      rtc.adjust(DateTime(yyyy, mm, dd, hours, minutes, 0));
    }
    request->send(200, "text/html", "Date & Time set!! <br><a href=\"/Date\">Go back</a>");
  });

  // Handle form submission
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {                         // Function handling the "/data" route
    String jsonData = "{\"Device_ID\":\"" + String(Device_ID) + "\",\"data\":\"";  // Variable that holds JSON String
    int count = 0;                                                                 // counter that holds number of readings
    String year_ = String(memory.readFloat(sizeof(float)));                        // Getting the year saved in the memory location

    for (uint32_t addr = 16; addr < memory.getCapacity(); addr += 4) {  // For loop to iterate over the data to convert it to JSON
      if (String(memory.readFloat(addr)) != "nan") {                    // If statement to check if memory loction is empty
        jsonData += String(memory.readFloat(addr)) + ",";               // Add commma "," after every data byte
        count += 1;                                                     // Increment counter
        if ((count % 9 == 0)) {                                         // If statement to check if the count variable is equal to the variable that tracks the
                                                                        // configAt variable to check if the data appended to the string is containing time
          jsonData += "\\n";                                            // Add newline character
        }
      } else {
        break;
      }
    }
    jsonData += "\"}";
    request->send(200, "text/html", jsonData);
  });

  server.on("/erase", HTTP_GET, [](AsyncWebServerRequest *request) {                         // Function handling the "/data" route
    memory.eraseChip();
    delay(50);
    request->send(200, "text/html", "erased");
  });

    //--------------------------------------------SERVER ROUTE FUNCTIONS------------------------------------------------------------------------------------------------------//
  server.begin();

}

//-------------------------------------------------SETUP FUNCTION------------------------------------------------------------------------------------------------------//



//---------------------------------------------ADDITIONAL FUNCTIONS------------------------------------------------------------------------------------------------------//

void LogValue() {                                                                     //-----> Function that saves to the Flash Memory
  readTemperatures();
  // sensors.requestTemperatures();                    // Obtain the temperature data from the sensors object
  // temperatureC = sensors.getTempCByIndex(0);  // Obtain the Temperature value at index "0" and store the temperature value into the variable
  // sensee2.requestTemperatures();
  // sensorr2 = sensee2.getTempCByIndex(0);
  // sensee3.requestTemperatures();
  // sensorr3 = sensee3.getTempCByIndex(0);
  // sensee4.requestTemperatures();
  // sensorr4 = sensee4.getTempCByIndex(0);

  DateTime now = rtc.now();  // Re-initialise the RTC module
  float dd = (float)now.day();     // Stoing the date as a float
  float mm = (float)now.month();   // Stoing the month as a float
  float yy = (float)now.year();    // Stoing the year as a float
  float HH = (float)now.hour();    // Stoing the hours as a float
  float MM = (float)now.minute();  // Stoing the minutes as a float

  if (String(memory.readFloat(LastLocation)) == "nan") {
    memory.writeFloat(LastLocation + 1 * sizeof(float), (float)Device_ID);  // Write the Device ID into the 4th index in the Flash Memory
    Boot = false;
  }

  if (Boot) {
    for (uint32_t addr = 16; addr < memory.getCapacity(); addr += 4) {  // For loop to iterate over the data
      if (String(memory.readFloat(addr)) == "nan") {
        address = addr;
        break;
      }
    }
    Boot = false;
  }

  memory.writeFloat(address, temperatures[0]);                  // Writing the Temperature into the memory location
  memory.writeFloat(address + 1 * sizeof(float), temperatures[1]);  // Writing the Temperature into the memory location
  memory.writeFloat(address + 2 * sizeof(float), temperatures[2]);  // Writing the Temperature into the memory location
  memory.writeFloat(address + 3 * sizeof(float), temperatures[3]);  // Writing the Temperature into the memory location
  memory.writeFloat(address + 4 * sizeof(float), HH);        // Writing the Hours into the memory location
  memory.writeFloat(address + 5 * sizeof(float), MM);        // Writing the Minutes into the memory location
  memory.writeFloat(address + 6 * sizeof(float), dd);
  memory.writeFloat(address + 7 * sizeof(float), mm);
  memory.writeFloat(address + 8 * sizeof(float), yy);

  memory.writeFloat(LastLocation, address);  // Writing adress at the 0th index to just indicate that configuraton process is complete
  address += 9 * sizeof(float);  // Incrementing the adress variable by 12 locations since we wrote Temp, Hours and Minutes
  Serial.println(address);
}

void readTemperatures() {
    for (int i = 0; i < SENSOR_COUNT; i++) {
        sensors[i]->requestTemperatures();
        temperatures[i] = sensors[i]->getTempCByIndex(0);
    }
} 

void loop() {

  // sensors.requestTemperatures();                    // Obtain the temperature data from the sensors object
  // float temperatureC = sensors.getTempCByIndex(0);  // Obtain the Temperature value at index "0" and store the temperature value into the variable

  // sensee2.requestTemperatures();
  // sensee3.requestTemperatures();
  // sensee4.requestTemperatures();

  // sensorr2 = sensee2.getTempCByIndex(0);
  // sensorr3 = sensee3.getTempCByIndex(0);
  // sensorr4 = sensee4.getTempCByIndex(0);

  // Updates frequently
  currentTime = millis();  // Get the current instance in milliseconds

  DateTime now = rtc.now();        // Re-initialise the RTC module
  float MM = (float)now.minute();  // Stoing the minutes as a float

  if(((int)MM % 15 == 0) && (logState0 == 0)){
    logState0 = true;
    logState30 = false;

    LogValue();  // Calling the LogValue function to save the readings into the Flash memory
  } else if (((int)MM % 15 != 0)){
    logState0 = false;
    logState30 = true;

    // LogValue();  // Calling the LogValue function to save the readings into the Flash memory
  } 
  // server.handleClient();                                                               // Handling the server requests until the set time interval is due
  // for(int i=0; i<memory.getCapacity(); i+=4){
  //   Serial.println(memory.readFloat(i));
  // }

  if (currentTime - hotspotTimer >= (hotspotEnabled ? hotspotOffInterval : hotspotOnInterval)) {  // Check if a new interval is due or not to toggle the state of the Hotspot
    // Toggle the hotspot state
    hotspotEnabled = !hotspotEnabled;  // Check the hotspot state and toggle the state of the Hotspot

    // If hotspot is being turned off, disable it
    if (!hotspotEnabled) {          // Check if the hotspot is ON
      WiFi.softAPdisconnect(true);  // Disconnect existing clients and disable hotspot
      Serial.println("Hotspot turned off");
    } else {
      WiFi.softAP(ssid, password);  // Re-enable hotspot
      Serial.println("Hotspot turned on");
    }

    hotspotTimer = currentTime;  // Reset the timer for the next interval
  }

  readTemperatures();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1:" + String(temperatures[0]));
  lcd.setCursor(0, 1);
  lcd.print("2:" + String(temperatures[1]));
  lcd.setCursor(9, 0);
  lcd.print("3:" + String(temperatures[2]));
  lcd.setCursor(9, 1);
  lcd.print("4:" + String(temperatures[3]));
  delay(1000); // Wait 1 second before next loop

  // clearMemory();
  float dd = (float)now.day();    // Stoing the date as a float
  float mm = (float)now.month();  // Stoing the month as a float
  float yy = (float)now.year();   // Stoing the year as a float
  float HH = (float)now.hour();   // Stoing the hours as a float
  // float MM = (float)now.minute();                                                        // Stoing the minutes as a float

  // Serial.println(String(dd) + "/" + String(mm) + "/" + String(yy) + " " + String(HH) + ":" + String(MM));
}

//---------------------------------------------ADDITIONAL FUNCTIONS------------------------------------------------------------------------------------------------------//