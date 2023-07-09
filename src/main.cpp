/*
   Acurite 00899

3/20/2015    1.83    Took this as the latest (Mom's computer is gone).  Id change from 157E to 1730
3/24/2015    1.84    Eliminate id check, use id received if matches
3/25/2015    1.85    mac addr set here (2nd receiver built)
8/18/2018    1.90    Change title to PV.  Original HW!!
7/08/2023    1.94    Change title to Owl Ridge
*/
const char* ver = "v1.94";  // == 1.83
#include "Arduino.h"   // replace this by "WProgram.h" when your IDE is older then 1.0
 
#define DEBUG true // flag to turn on/off Serial debugging.  Need FALSE for webserver memory!
#define Serial if(DEBUG)Serial 

#include <stdio.h>
#include <TimeLib.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Streaming.h>
//#include <MemoryFree.h>
const char* br="<br/>\n";
#include "Rolling.h"
#include "Stats.h"
#include "WebServer.h"

byte saveId[3];

WebServer server;
byte WebServer::mac[] = {0x90,0xa2,0xda,0x0d,0x97,0x44 }; // uno  .44, d1 mini .45 
const int PIN=8;
const int RSSIPIN=14;
char tempstr40[40];

// Prototypes
void logTime(int hundr);
void printFree();
void receivedData(byte bdat[][8]);

// Arduino Setup
void setup() 
{          
  // initialize the digital pin as an output.
  // Pin 13 has an LED connected on most Arduino boards:
  pinMode(PIN, INPUT);
  //pinMode(RSSIPIN, INPUT);
  //digitalWrite(RSSIPIN, HIGH);  // enable pullup
  Serial.begin(9600);
  Serial << F("Accurite 00899, ") << ver << endl;

  // reduce no-connection timeout to 15000 in C:\Program Files\Arduino\arduino-1.0.3\libraries\Ethernet\dhcp.h
  server.init();
  Serial.println();
  printFree();
  //plugSomeData();
}

void printFree(){  Serial << "Free: " << freeMemory() <<  endl; };

const int STARTW=600, STARTTOL=150;
const int LONGW=390,  LONGTOL=110;
const int SHORTW=185, SHORTTOL=95;
int dur=0;
const int DATLEN=64;
const int PKTLEN=(4+DATLEN)*3;  // 3 msgs of hdr + 64bits each
int durs[PKTLEN];


/************ recordRain with rollover , v 1.2 ***********/
static uint32_t lastMilli=0;
static uint32_t lastMins=0;
unsigned long minSinceBoot()  // corresponds to millis()
{
	uint32_t now=millis();
	uint32_t elapsedMins = ( now - lastMilli )/ 60000L;
	if (elapsedMins > 0)
	{
		lastMins += elapsedMins;
		//lastMilli=now;
		lastMilli+= elapsedMins * 60000L;
	}
	return lastMins;
}

float daysSinceBoot()
{
	return float( minSinceBoot()) / 60/24;
}

// days:hours:minutes
char * uptimeString()
{
	unsigned long m=minSinceBoot();
	static char s[14];
	//sprintf(s, "%4lu:%02lu:%02lu", m/60/24, (m % (60*24)) / 60, m % 60);
	sprintf(s, "%4lu:%02lu:%02lu:%02lu", m/60/24/7, (m % (60*24*7))/60/24, (m % (60*24)) / 60, m%60);  // weeks, days, hours
	return s;
}

static long lastraincount=-1;
static long lastreadtime=-1; 
unsigned long passBits, failID, failCheckSum, failMatch8, failMatchRain, failParity , failNeg, failSanity;

unsigned long ageMin()
{
  return lastreadtime <=0 ? 99999 : (minSinceBoot() - lastreadtime) ; 
}

void recordRain(long raincount)
{
    unsigned long ageMinSave=ageMin();  // might be needed below
    lastreadtime=minSinceBoot();
    /*
    if (lastraincount < 0 || raincount < lastraincount)
    { lastraincount=raincount; return; } // this, or the rain sender, has been reset
    */
    if (lastraincount < 0)    // first read
    { lastraincount=raincount; return; } 
    
    if ( raincount < lastraincount )
    {
       if (raincount < 5 )  // insane, unless the rain sender has just been reset
          lastraincount=raincount; 
       else
          failNeg++ ;
       return; 
     } 
    
    int hundr = raincount-lastraincount;
    lastraincount=raincount;
    if ( hundr/( ageMinSave + 1) > 100 )  // greater than 1 inch per minute is insane
    {
       failSanity++;
       return;
    }
    Stats::record( lastreadtime ,hundr);
    logTime( hundr);
}

/************ recordRain  ***********/

void logTime(int hundr)
{
//    console << F("  recording: millis: ") << millis() << " minutes: " << lastreadtime <<" hundr: "<< hundr
//    << " days: " << daysSinceBoot() <<  endl;
}

void advanceStats()
{
    Stats::record( minSinceBoot() ,0);
    logTime( 0);
}

inline bool within( int value , int target, int tolerance)
{	return ( (target-tolerance) < value && value < (target+tolerance)); }

void printDur( int* durr, int startw)
{
	for (int i=startw; i<startw+4+DATLEN; i++)
	{
		//Serial.print( durr[i]); Serial.print( ",");
		Serial << durr[i] << ",";
		if (((i + 1) % 4)==0) Serial.print("  ");
	}
	Serial.println();
}

/* Measures the length (in microseconds) of a pulse on the pin; state is HIGH
 * or LOW, the type of pulse to measure.  Works on pulses from 2-3 microseconds
 * to 3 minutes in length, but must be called at least a few dozen microseconds
 * before the start of the pulse. */
unsigned long readPulse(uint8_t pin, uint8_t state, unsigned long timeout=5000L)
{
	// cache the port and bit of the pin in order to speed up the
	// pulse width measuring loop and achieve finer resolution.  calling
	// digitalRead() instead yields much coarser resolution.
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	uint8_t stateMask = (state ? bit : 0);
	unsigned long width = 0; // keep initialization out of time critical area
	
	// convert the timeout from microseconds to a number of times through
	// the initial loop; it takes 16 clock cycles per iteration.
	unsigned long numloops = 0;
	unsigned long maxloops = microsecondsToClockCycles(timeout) / 16;
	
	// wait for any previous pulse to end
        // jcc excised	
	
	// wait for the pulse to start
	while ((*portInputRegister(port) & bit) != stateMask)
		if (numloops++ == maxloops)
			return 0;
	
	// wait for the pulse to stop
	while ((*portInputRegister(port) & bit) == stateMask) {
		if (numloops++ == maxloops)
			return 0;
		width++;
	}

	// convert the reading to microseconds. The loop has been determined
	// to be 20 clock cycles long and have about 16 clocks between the edge
	// and the start of the loop. There will be some error introduced by
	// the interrupt handlers.
	return clockCyclesToMicroseconds(width * 21 + 16); 
}


bool isOn(int pulse){
	bool ans=false;
	if (within( pulse, SHORTW, SHORTTOL)) ans= false;
	else if (within( pulse, LONGW, LONGTOL)) ans = true;
	else if (within( pulse, STARTW, STARTTOL)) ans = true;
	// cout << "Pulse width unhandled  " << pulse << endl;
	return ans;
}

void convert( int *durrp, unsigned char odat[][8])
{

	for (int rec=0; rec<3; rec++){
		durrp+=4; // skip 4 pulse header
		for (int ipulse=0; ipulse < 64; ipulse++, durrp++){
			int obytno= ipulse / 8;
			int obitno= 7-(ipulse % 8);
			if (isOn(*durrp))
				odat[rec][obytno] |= ( 1<<obitno);
			else
				odat[rec][obytno] &= ~( 1<<obitno);

		}
	}

}

bool startBitsB()
{
	for (int i=0; i<4; i++)
		if (! within( pulseIn(PIN, HIGH), STARTW, STARTTOL))
			return false;
	return true;
}

bool evenParity( byte *b)
{
   byte b0p=0, b1p=0;
   for (byte i=0; i<8; i++)
   {
     b0p += (b[0] >> i) & 1;
     b1p += (b[1] >> i) & 1;
   } 
  return  !  ( b0p & 1 + b1p & 1);
}

int x;
int usHi, usLo;
void loop() 
{
	int i=4;
        server.webserver();
        //x=analogRead( RSSIPIN);
        //if ( 190 > x) return;
 
	if  (!startBitsB()) return;  // wait for 4 start bits
        //Serial << "bits" << endl;
	for ( ; i < PKTLEN; i++)
        {
           usHi = readPulse( PIN, HIGH);
           usLo = readPulse( PIN, LOW);
           durs[i] = (usHi > usLo) ? LONGW : SHORTW;
        }

        passBits++; 
	byte bdat[3][8] ;
	convert( durs, bdat);
        //Serial << " x " << x << endl;
	//if (bdat[0][0]==0x15 && bdat[0][1]==0x7e && bdat[0][2]==0xf0) {
        if (   !strncmp((char *)bdat[0], (char *)bdat[1], 2) )
        {
                Serial << _HEX( bdat[0][0]) << ':' << _HEX( bdat[0][1]) << endl;
                receivedData( bdat);
	}
        else failID++;

} // loop

struct RSSI_Led 
{  // RAII
	int pin;
	RSSI_Led(int p): pin(p) { pinMode(pin,OUTPUT); digitalWrite(pin, HIGH);	}
	~RSSI_Led() { digitalWrite(pin, LOW); pinMode(pin,INPUT); }
};

bool checkSum( byte *b)
{
   int sum=0;
   for (int i=0; i<7; i++)
     sum+=b[i];
   return ( sum & 0xff ) == b[7];
}

void receivedData(byte bdat[][8])
{
    int i;
    if ( ! checkSum( bdat[0] ) || ! checkSum( bdat[1]))
    {
       failCheckSum++;
       Serial << F(" failCheckSum") << endl;
       return;
    }
    int mismatch8= memcmp( (char *)bdat[0], (char *)bdat[1], sizeof(bdat[0]));
    // if the two data packets don't match, there is some pulse interpretation difficulty
    if (  mismatch8 ) 
    {
        failMatch8++;
        for (int ii=0;ii<3;ii++) {
            sprintf(tempstr40, "%02x%02x %02x%02x %02x%02x %02x%02x", bdat[ii][0], bdat[ii][1], bdat[ii][2], bdat[ii][3], bdat[ii][4], bdat[ii][5], bdat[ii][6], bdat[ii][7]);
            Serial << F("----> ") << tempstr40 << endl;
        }
        for ( i=0; i<3; i++) printDur( durs, i*(4+DATLEN));
    }

    int rain[3];
    //if (!compareBytes( bdat, 0, 4+DATLEN)) return;  // make sure copies zero and 1 are the same
    RSSI_Led lightIt(RSSIPIN);
    for (i=0;i<3;i++) {
        rain[i] = (bdat[i][ 6] & 0x7f) + ((bdat[i][ 5] & 0x7f) << 7);
        //printBin( durs, i*(4+DATLEN));
        Serial << F("  raincount: ") << rain[i] << endl;
    }

    failMatchRain += (rain[0]!=rain[1]);
    if (rain[0]==rain[1] && ! mismatch8 ) 
    {
        if ( !evenParity( bdat[0]+5) || !evenParity( bdat[1]+5 ))
        {
          Serial << F(" failParity") << endl;
          failParity++;
        }
        else
        { 
          recordRain(rain[0]);
          memcpy( saveId, bdat[0], 3);
        }
    }
    else 
       Serial << F("*** mismatch ***") << endl;
    printFree();
    //Serial.println(String("----> ") + str  + " rainfall hundredths: " + rainf + " <<<<<<<<-");
    delay(1000);   // time to flush serial
    //Serial.flush();  // wait til complete  http://code.google.com/p/arduino/issues/detail?id=871
}
/*
boolean compareBytes( byte dat[], int one, int theother){
    for (int i=0;i<(4+DATLEN); i++)
        if (dat[one++] != dat[theother++]) return false;
    else return true;
}
*/


void respondToBrowser(String & url, Stream & client){
          client.print( F("HTTP/1.0 200 OK\nContent-Type: text/html\nConnnection: close\n\n<!DOCTYPE HTML>\n<html>\n<meta http-equiv=\"refresh\" content=\"" ));
          client.print( 60);  // (odd=!odd)? 4 : 7);
          client.println("\">");
          client <<  F("<title>Owl Ridge2 Rainfall</title><h2>Owl Ridge2 Rain in Hundredths of an Inch</h2>");
	  advanceStats();   // 1.27
          Stats::upminutes = minSinceBoot();
          Stats::report(Stats::L1, client);
          Stats::report(Stats::L24, client);
          Stats::report(Stats::L7, client);
          Stats::report(Stats::L4, client);
          Stats::report(Stats::L12, client);
          char d=' ';
          client << br 
              << F("Acurite 00899 ") << ver << F(", id ")
              << _HEX( saveId[0]) << F(".") << _HEX( saveId[1]) << br
              << F("uptime weeks: " ) << uptimeString() << br 
              << F("diagnostic: ") << ageMin() <<d<< lastraincount <<d<< minSinceBoot()/60 <<d<< freeMemory() <<d<< 
                 _HEX( saveId[2] ) <<d<< passBits <<d<< failID <<d<< failCheckSum <<d<< failMatch8 <<d<< 
                 failMatchRain <<d<< failParity <<d<< failNeg <<d<< failSanity <<d<< br
              << F("      ageMin count upHr freeMem battery passB failID checkSum match8 matchRain parity neg sanity") << br
              << F("</html>") << endl;

}

