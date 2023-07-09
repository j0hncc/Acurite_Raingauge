
#include <stdio.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <Streaming.h>

#include "secrets.h" :

int freeMemory(){ return ESP.getFreeHeap(); }

void respondToBrowser(String & url, Stream & client);  // jcc 8/16/18

WiFiServer server(80);
class WebServerWifi
{
    public:
    // Initialize the Ethernet server library
    // with the IP address and port you want to use 
    // (port 80 is default for HTTP):
    
    void init(){
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, password);

        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
        }    

        Serial << "Connected, IP address: " << WiFi.localIP() << ", mac: " <<  WiFi.macAddress() <<  endl;
        server.begin();
    }

    // called every loop()
    void webserver()
    {

        // listen for incoming clients
       
        WiFiClient client=server.accept();

        // Serial << "End: " << millis() << endl;
        if (client) {
            Serial.println("new client");
            // an http request ends with a blank line
            boolean currentLineIsBlank = true;
            String name="";
            String s;
            while (client.connected()) {
            if (client.available()) {
                // Serial.println("Client Available");
                char c = client.read();
                // if you've gotten to the end of the line (received a newline
                // character) and the line is blank, the http request has ended,
                // so you can send a reply
                if (c == '\n' && currentLineIsBlank) {
                respondToBrowser(s, client);
                Serial << F("Free: 1337 ") <<  endl;
                break;
                }
                if (c == '\n') {
                // you're starting a new line
                receivedLine(s, name);
                currentLineIsBlank = true; s="";
                } 
                else if (c != '\r') {
                // you've gotten a character on the current line
                currentLineIsBlank = false;s+=c;
                }
            }
            }
            // give the web browser time to receive the data
            delay(5);
            // close the connection:
            client.stop();
            Serial.println("Web done");
        }
        // else Serial << "No Client " << millis() << endl;

    }

    void receivedLine( String& s, String& name)
    {
        if (s.startsWith("GET") || s.startsWith("User"))
            Serial << s << endl;
        //if (name=="") { Serial << "yes" << endl; name = s;  name.replace("GET /HelloThere?myname=","");name.replace(" HTTP/1.1", ""); }
        //if (s.startsWith("GET /HelloThere?myname=")) name=s.substring(23);
    }


} wifiserver;
