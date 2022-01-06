#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_NeoPixel.h>

#include "index.html.h"

#define NUMPIXELS 15
#define PIXEL_PIN 6

const char* ssid = "ssid";
const char* password = "password";
const char* hostname = "lights";
typedef struct{
    bool power;
    uint8_t pulseMode;  // 0: continuous, 1: dashes, 2: dots
    uint8_t colourMode;  // 0: single, 1: spread, 2: normal, 3: tight 4: other
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} statusConfig;

statusConfig status = {false, 0, 0, 0, 0, 0};  // initialisation status

ESP8266WebServer server(80);
Adafruit_NeoPixel pixels(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void parseSettingString(String setting){
    // Numbers encode setting in csv row.

    char pulseMode_char, colourMode_char;
    char red_buf[3], green_buf[3], blue_buf[3];  // Two hex characters plus null

    status.power = setting.charAt(0) == '1';
    pulseMode_char = setting.charAt(2); 
    colourMode_char = setting.charAt(4);
    status.pulseMode = atoi(&pulseMode_char);
    status.colourMode = atoi(&colourMode_char);
    setting.substring(7, 9).toCharArray(red_buf, 3);
    setting.substring(9, 11).toCharArray(green_buf, 3);
    setting.substring(11, 3).toCharArray(blue_buf, 3);
    status.red = (unsigned int) strtoul(red_buf, nullptr, 16);
    status.green = (unsigned int) strtoul(red_buf, nullptr, 16);
    status.blue = (unsigned int) strtoul(red_buf, nullptr, 16);

    // Debugging
    Serial.print("\tPower set to: "); Serial.println(status.power);
    Serial.print("\tpulseMode set to: "); Serial.println(status.pulseMode);
    Serial.print("\tcolourMode set to: "); Serial.println(status.colourMode);
    Serial.print("\tRGB: "); Serial.print(status.red, HEX); Serial.print(":"); Serial.print(status.green, HEX); Serial.print(":"); Serial.println(status.blue, HEX);
}

void setLEDs(){
    if(!status.power){
        pixels.clear();
        return;
    }

    if(status.colourMode == 0 && status.pulseMode == 0){
        for(int i = 0; i < pixels.numPixels(); i++){
            pixels.setPixelColor(i, status.red, status.green, status.blue);
        }
        pixels.show();
        return;
    }
    if(status.pulseMode == 0){
        pixels.rainbow(0, status.colourMode);
        pixels.show();
        return;
    }

    // Dots or dashes not yet implemented
    status.pulseMode = 0;
    setLEDs();
}

void getLEDsStatus(char* buffer){
    // Buffer must be size 14
    buffer[0] = status.power ? '1' : '0';
    itoa(status.pulseMode, &buffer[2], 10);
    itoa(status.colourMode, &buffer[4], 10);
    itoa(status.red, &buffer[7], 16);
    itoa(status.green, &buffer[9], 16);
    itoa(status.blue, &buffer[11], 16);
    buffer[1] = ','; buffer[3] = ','; buffer[5] = ','; buffer[6] = '#';
}

void serveStatusGet(){
    Serial.println("Received StatusGet:");
    Serial.println(server.arg("plain"));
    Serial.println("\n");
    char buffer[14];
    getLEDsStatus(buffer);
    server.send(200, "text/plain", buffer);
}

void serveStatusSet(){
    Serial.println("Received StatusSet:");
    Serial.println(server.arg("plain"));
    Serial.println("\n");
    parseSettingString(server.arg("plain"));
}

void serveIndexPage(){
    server.send(200, "text/html", INDEX_HTML);
}

void handleNotFound() {
    String message = "Handler Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}


void setup(){

    Serial.begin(115200);

    // Initialise WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED){
        delay(500);
    }
    Serial.println("WiFi connected, IP Address: " + WiFi.localIP().toString());

    // Initialise DNS
    if(MDNS.begin(hostname)){
        Serial.println("mDNS responder started.");
    }

    server.on("/", serveIndexPage);
    server.on("/index.html", serveIndexPage);

    server.on("/set_state.txt", HTTP_POST, serveStatusSet);
    server.on("/get_state.txt", HTTP_GET, serveStatusGet);

    server.onNotFound(handleNotFound);

    server.begin();
    pixels.begin();
    pixels.clear();
}

void loop(){
    server.handleClient();
    MDNS.update();
    setLEDs();
    delay(25);
}