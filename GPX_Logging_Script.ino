#include <Adafruit_GPS.h>
#include <SD.h>
#include <SPI.h>
#include <Audio.h>
#include <Wire.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioSynthWaveform       waveform1;      //xy=398,259
AudioSynthWaveform       waveform2;      //xy=401,297
AudioEffectDigitalCombine combine1;       //xy=651,273
AudioOutputI2S2          i2s2_1;         //xy=960,257
AudioConnection          patchCord1(waveform1, 0, combine1, 0);
AudioConnection          patchCord2(waveform2, 0, combine1, 1);
AudioConnection          patchCord3(combine1, 0, i2s2_1, 0);
AudioConnection          patchCord4(combine1, 0, i2s2_1, 1);
// GUItool: end automatically generated code

#define GPSSerial Serial2
#define captureControl 28
#define max_SD 6


File GPX_000;
Adafruit_GPS GPS(&GPSSerial);

const int chipSelectSD = BUILTIN_SDCARD;
char fileName[13] = "test.gpx";
bool LogActive = 0;

#define GPSECHO true

bool btnState = 1;
bool previousBtnState = 1;
uint32_t timer = 0;

bool startGPXFile(File &f, char *fileName){
  if (!GPS.fix){
    Serial.println("Failed to start log, no fix");
    return;
  }
  if (f) {
  
  //Write Header
    f.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    f.println("<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" version=\"1.1\" creator=\"Dillon Mims\" xmlns=\"http://www.topografix.com/GPX/1/1\">");
 
  //Start Route
    f.println("<rte>");
    f.print("<name>");
    f.print(fileName);
    f.println("</name>");
    f.print("<desc>");
    f.print("GPX Tracking");
    f.println("</desc>");
    f.print("<number>");
    f.print("1");
    f.println("</number>");
    Serial.println("Log Started");

    waveform1.amplitude(0.5);
    waveform2.amplitude(0.5);
    delay(50);
    waveform1.amplitude(0);
    waveform2.amplitude(0);

    return 1;
  }
  else{
    Serial.println("Bruh");
    return 0;
  }
}

void endGPXFile(File &f){

  if (!f) return;
  f.println("  </rte>");
  f.println("</gpx>");

  // close the file:
  f.close();
  waveform1.amplitude(0.5);
  waveform2.amplitude(0.5);
  delay(50);
  waveform1.amplitude(0);
  waveform2.amplitude(0);
  Serial.println("Log Ended");
}

void writeRoutePoint(File &f, Adafruit_GPS &gps){

  if (!gps.fix) return;

  float lon = (gps.longitudeDegrees);
  float lat = (gps.latitudeDegrees);

  f.print("  <rtept lat=\""); f.print(lat, 6); f.print("\" lon=\""); f.print(lon, 6); f.println("\">");

  f.print("<ele>"); f.print(GPS.altitude); f.println("</ele>");

 f.print("    <time>20");
  if (gps.year < 10) f.print('0');
  f.print(gps.year);
  f.print('-');
  if (gps.month < 10) f.print('0');
  f.print(gps.month);
  f.print('-');
  if (gps.day < 10) f.print('0');
  f.print(gps.day);
  f.print('T');
  if (gps.hour < 10) f.print('0');
  f.print(gps.hour);
  f.print(':');
  if (gps.minute < 10) f.print('0');
  f.print(gps.minute);
  f.print(':');
  if (gps.seconds < 10) f.print('0');
  f.print(gps.seconds);
  f.println("Z</time>");

  f.println("<name>GPS Waypoint</name>");
  
  f.println("</rtept>");

  Serial.println("Logged");
  delay(250);
}



void setup()
{
 // Open serial communications and wait for port to open:
  Serial.begin(9600);

  pinMode(max_SD, OUTPUT);
  digitalWrite(max_SD, HIGH);

  pinMode(captureControl, INPUT_PULLUP);

  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);

  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  delay(1000);

  // Ask for firmware version
  GPSSerial.println(PMTK_Q_RELEASE);


  Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelectSD)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  AudioMemory(20);
  waveform1.frequency(500);
  waveform1.begin(WAVEFORM_SINE);
  waveform2.frequency(5000);
  combine1.setCombineMode(3);

}

void loop()
{

    char c = GPS.read();                 // grab incoming bytes
    if (GPS.newNMEAreceived()) {         // got a full NMEA sentence?

      if (!GPS.parse(GPS.lastNMEA())) {  // parse it
      // parse failed; wait for next sentence
      }
    }

  bool btnState = digitalRead(captureControl);
  //Serial.println(btnState);

  //much thanks to user AndyA on PJRC Forum for helping me clean up my debounce as well as state handling :)
  if(previousBtnState == HIGH && btnState == LOW){
    uint32_t now = millis();
    if((now - timer) > 100){
      timer = now;
      if(LogActive == 0){
        GPX_000 = SD.open(fileName, FILE_WRITE);
        LogActive = startGPXFile(GPX_000, fileName);
      }
      else{
        endGPXFile(GPX_000);
        LogActive = 0;
      }
    }
  }
  previousBtnState = btnState;

  	if(LogActive == 1){

    if (GPS.fix) {
      writeRoutePoint(GPX_000, GPS);
    }
    else if(!GPS.fix){
      Serial.println(GPS.fix);
    }

  }

}

