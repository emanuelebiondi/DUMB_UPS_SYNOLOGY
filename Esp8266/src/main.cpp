/**
 * WiFiManager advanced - Implements TRIGGEN_PIN button press, press for ondemand configportal, hold for 3 seconds for reset settings.
 */

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <time.h>
#include <Preferences.h>


#define MY_NTP_SERVER "0.pool.ntp.org" 
#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3" // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

time_t now; // this are the seconds since Epoch (1970) - UTC
tm tm; // the structure tm holds time information in a more convenient way

#define TRIGGER_PIN 0
const int led = 13;
uint8_t PIN0 = D2;
uint8_t PIN1 = D5;
uint8_t PIN2 = D6;
uint8_t PIN3 = D7;

bool power_mode;
bool battery_mode;
bool charging_mode;

bool shouldSaveConfig = false;
char PIN0_name[16] = "PIN 0 - Power";
char PIN1_name[16] = "PIN 1 - Battery";
char PIN2_name[16] = "PIN 2 - Unknown";
char PIN3_name[16] = "PIN 3 - Unknown";

char lastTimeLineLoose[50] = "";
char lastTimeLineLooseEpoc[12] = "";
bool lastTimeLineLooseInsert = 0;

// wifimanager can run in a blocking mode or a non blocking mode
// Be sure to know how to process loops with no delay() if using non blocking
bool wm_nonblocking = false; // change to true to use non blocking

WiFiManager wm; // global wm instance
ESP8266WebServer server(80);

String outputState(int output){
  if(digitalRead(output)) return "\"true\"";
  else return "\"false\"";
}

String powerModeStr(){
  if(power_mode) return "<h3 style=\"color: green;\"> LINEA </h3> ";
  else return "<h3 style=\"color: red;\"> BATTERY </h3> ";
}

String chargingModeStr(){
  if(charging_mode && power_mode) return "<h3 style=\"color: orange;\"> CHARGING </h3> ";
  else if ( (charging_mode == 0) && power_mode) return "<h3 style=\"color: green;\"> CHARGE </h3> ";
  else return "<h3 style=\"color: red;\"> DISCHARGING </h3> ";
}

uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000 () {
  randomSeed(A0);
  return random(5000);
}

String Time_str() {
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
    return
    String(tm.tm_year + 1900) + "/" + String(tm.tm_mon + 1) + "/" +  String(tm.tm_mday) + " " +
    String(tm.tm_hour) + ":" + String(tm.tm_min) + ":" + String(tm.tm_sec);
}

void valueUpdate(){
  bool tmp[10];
  for (int i = 0; i < 10; i++){
    tmp[i] = digitalRead(PIN0);
    Serial.printf("%d ", int(tmp[i]));
    delay (90);
  } 
  Serial.println("");
  // TUTTI 1 --> LINE MODE - NO CHARGE [AND]
  if((tmp[0] & tmp[1] & tmp[2] & tmp[3] & tmp[4] & tmp[5] & tmp[6] & tmp[7] & tmp[8] & tmp[9]) == 1){
    charging_mode = 0; power_mode = 1; battery_mode = 0;
      if(lastTimeLineLooseInsert){
      // strcpy(lastTimeLineLoose, "NULL");
      // strcpy(lastTimeLineLooseEpoc, "null");
      lastTimeLineLooseInsert = 0;
    }
  }

  // TUTTI 0 --> BATTERY MODE [NOR]
  if((!(tmp[0] | tmp[1] | tmp[2] | tmp[3] | tmp[4] | tmp[5] | tmp[6] | tmp[7] | tmp[8] | tmp[9])) == 1){
    charging_mode = 0; power_mode = 0; battery_mode = 1;
    if(lastTimeLineLooseInsert == 0){
      strcpy(lastTimeLineLoose, Time_str().c_str());
      strcpy(lastTimeLineLooseEpoc, String(time(&now)).c_str());
      
      lastTimeLineLooseInsert = 1;
    }
  }
  
  // MISTI  --> LiNE MODE - CHARGE [XOR]
  if((tmp[0] ^ tmp[1] ^ tmp[2] ^ tmp[3] ^ tmp[4] ^ tmp[5] ^ tmp[6] ^ tmp[7] ^ tmp[8] ^ tmp[9]) == 1){
    charging_mode = 1; power_mode = 1; battery_mode = 0;
      if(lastTimeLineLooseInsert){
      // strcpy(lastTimeLineLoose, "NULL");
      // strcpy(lastTimeLineLooseEpoc, "null");
      lastTimeLineLooseInsert = 0;
    }
  }

  // Serial.println("power_mode: " + String(power_mode));
  // Serial.println("");
  // Serial.println("charging_mode:" + String(charging_mode));
  // Serial.println("battery_mode:" + String(battery_mode));
}

void handleRoot() {
  StreamString temp;
  temp.reserve(1500);  // Preallocate a large chunk to avoid memory fragmentation
  temp.printf("\
                <html>\
                  <head>\
                    <meta http-equiv='refresh' content='30' name='viewport' content='width=device-width, initial-scale=1'/>\
                    <title>DUMB UPS</title>\
                    <style>\
                      html {font-family: Arial; display: inline-block; text-align: center;}\
                      h2 {font-size: 3.0rem;}\
                      body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}\
                    </style>\
                  </head>\
                  <body>\
                    <br>\
                    <h1>DUMB UPS BOARD</h1>\
                    <img style=\"width: 80;\" src=\"https://cdn-icons-png.flaticon.com/512/11948/11948074.png\">\
                    <br>\
                    <h4> MODE: </h4> %s\
                    <h4> Battery State:</h4> %s\
                    <br>\
                    <h4> Last update at: <b> %s </b></h4>\
                    <h4> Line loose at: <b>%s <b></h4>\
                    <br>\
                    <br>\
                    <br>\
                    <h3> PIN READING - DEBUG MODE </h4>\
                    <h4> %s: %s</h4>\
                    <h4> %s: %s</h4>\
                    <h4> %s: %s</h4>\
                    <h4> %s: %s</h4>\
                  </body>\
                </html>",
              powerModeStr().c_str(),
              chargingModeStr().c_str(),
              Time_str().c_str(),
              lastTimeLineLoose,
              PIN0_name ,outputState(PIN0).c_str(), 
              PIN1_name, outputState(PIN1).c_str(), 
              PIN2_name, outputState(PIN2).c_str(), 
              PIN3_name, outputState(PIN3).c_str());
  server.send(200, "text/html", temp.c_str());
}

void handleGetValueJSON() {
  char tmp_battery_state [14];
  if(charging_mode && power_mode) strcpy(tmp_battery_state, "charging");
  else if ( (charging_mode == 0) && power_mode ) strcpy(tmp_battery_state, "charge");
  else strcpy(tmp_battery_state, "discharging");

  String message = "{ ";
  message += "\"mode\" : ";
  message += (power_mode ? "\"linea\"" : "\"battery\"");
  message += ", ";
  message += "\"battery_state\" : ";
  message += "\"";
  message += tmp_battery_state;
  message += "\"";
  message += ", ";
  message += "\"Time\" : ";
  message += "\"";
  message += Time_str().c_str();
  message += "\"";
  message += ", ";
  message += "\"TimeNowEpoc\" : ";
  message += "\"";
  message += String(time(&now)).c_str();
  message += "\"";
  message += ", ";
  message += "\"Timelineloose\" : ";
  message += "\"";
  message += lastTimeLineLoose;
  message += "\"" ;
    message += ", ";
  message += "\"TimelinelooseEpoc\" : ";
  message += "\"";
  message += lastTimeLineLooseEpoc;
  message += "\"" ;
  message += " }";
  server.send(200, "application/json", message);
}

String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveConfigCallback() {
  shouldSaveConfig = true;
}

void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP  
  Serial.begin(115200);
  Serial.setDebugOutput(false);  
  delay(3000);
  Serial.println("\n Starting");

  // wm.resetSettings(); // wipe settings

  if(wm_nonblocking) wm.setConfigPortalBlocking(false);
  
  std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit"};
  wm.setMenu(menu);

  wm.setClass("invert"); // set dark theme

  wm.setShowStaticFields(true); // force show static ip fields
  wm.setConfigPortalTimeout(180); // auto close configportal after n seconds

  wm.setSTAStaticIPConfig(IPAddress(192,168,1,43), IPAddress(192,168,1,1), IPAddress(255,255,255,0), IPAddress(192,168,1,1)); // optional DNS 4th argument

  wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails
  wm.setSaveConfigCallback(saveConfigCallback);  

  bool res;
  res = wm.autoConnect("DUMB-UPS AP", "password"); // password protected ap

  if(!res) {
    Serial.println("Failed to connect or hit timeout");
    ESP.restart();
  } 
  else {
    //if you get here you have connected to the WiFi    
    Serial.println("connected...yeey :)");
    configTime(MY_TZ, MY_NTP_SERVER);
    
    if (MDNS.begin("esp8266")) { Serial.println("MDNS responder started"); }

    server.on("/", handleRoot);

    server.on("/api/states", handleGetValueJSON);
    
    server.on(F("/api/liveordead"), []() {
      server.send(200, "text/plain", (battery_mode ? "dead" : "live"));
    });

    server.begin();
    Serial.println("HTTP server started");
    
    pinMode(PIN0, INPUT);
    pinMode(PIN1, INPUT);
    pinMode(PIN2, INPUT);
    pinMode(PIN3, INPUT);
    pinMode(TRIGGER_PIN, INPUT);
  }
}

void checkButton(){
  // check for button press
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if( digitalRead(TRIGGER_PIN) == LOW ){
      Serial.println("Button Pressed");
      server.stop();
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000); // reset delay hold
      if( digitalRead(TRIGGER_PIN) == LOW ){
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }
      
      // start portal w delay
      Serial.println("Starting config portal");
      wm.setConfigPortalTimeout(180);
      
      if (!wm.startConfigPortal("DUMB-UPS AP","password")) {
        Serial.println("failed to connect or hit timeout");
        delay(3000);
        ESP.restart();
      } else {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
        configTime(MY_TZ, MY_NTP_SERVER);
      }
    }
  }
}

void loop() {
  if(wm_nonblocking) wm.process(); // avoid delays() in loop when non-blocking and other long running code  
  checkButton();
  // put your main code here, to run repeatedly:
  server.handleClient();
  
  valueUpdate();
}
