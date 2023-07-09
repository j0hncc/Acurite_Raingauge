#include <Ethernet.h>
#include <MemoryFree.h>

void respondToBrowser(String & url, Stream & client);  // jcc 8/16/18

// v 1.13 6/9/2013 pass url string to respondToBrowser()
class WebServer {
    public:
    // Enter a MAC address and IP address for your controller below.
    // The IP address will be dependent on your local network:
    static byte mac[];
    //IPAddress ip(192,168,1, 177);
    
    // Initialize the Ethernet server library
    // with the IP address and port you want to use 
    // (port 80 is default for HTTP):
    EthernetServer server;
    
    static char* br;
    
    WebServer() : server(80) {};
    
    void init(){
           
      // start the Ethernet connection and the server:
      Serial.print("server ip ");
          if ( Ethernet.begin(mac)) {
            server.begin();
            Serial.print(Ethernet.localIP());    
          }
    }


void webserver() {
  // listen for incoming clients
  boolean odd=false;
  Serial << "";
  EthernetClient client = server.available();
  // Serial << "End: " << millis() << endl;
  if (client) {
    //Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String name="";
    String s;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          respondToBrowser(s, client);
          Serial << F("Free: ") << freeMemory() <<  endl;
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
  //else Serial << "No Client " << millis() << endl;
}  // webserver


void receivedLine( String& s, String& name)
{
    if (s.startsWith("GET") || s.startsWith("User"))
        Serial << s << endl;
    //if (name=="") { Serial << "yes" << endl; name = s;  name.replace("GET /HelloThere?myname=","");name.replace(" HTTP/1.1", ""); }
    //if (s.startsWith("GET /HelloThere?myname=")) name=s.substring(23);
}



};

    //byte WebServer::mac[] = {0x90,0xa2,0xda,0x0d,0x97,0x44 };  // set this in your mainline code
    char* WebServer::br="<br>\n";

