/*   
 *    
 *    Realisiert von Stefan Hagel,
 *    
 *    Private-use only! (you need to ask for a commercial-use)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *  Private-use only! (you need to ask for a commercial-use)
 *   
 *   
 *   ---------------------------------------------------------------------------------------------------------------------------------
 *   Remake einer "Wortuhr", realisiert mit einem ESP8266/ESP-01 und einem "Neopixel RGB-Streifen". Die Zeit wird per WLAN von einem Timeserver bezogen.
 *   
 *   Die Datenleitung des Neopixel-Streifens ist auf Pin 2 definiert (Kann weiter unten auch verändert werden.)
 *   
 *   In einem 2-Dimensionalen Array werden die Neopixel definiert die für den Aufruf eines Wortes (oder mehrerer Worte) benötigt werden.
 *   Es werden im weiteren Verlauf des Programmes Worte definiert (ES_IST) und mit der entsprechenden Zeile aus dem Array verknüpft.
 *   ES_IST findet man in der ersten Zeile(0). Es wird von 0 an aufwärts gezählt.
 *   
 *   Es fehlt hier noch:
 *   Die möglichkeit der automatischen Sommer/Winterzeitumstellung
 *   Dimmfunktion (Helle Umgebung = hohe Leuchtkraft...) mit eventuellem Farbwechsel auf rot in der Nacht.
 *   ---------------------------------------------------------------------------------------------------------------------------------
 */


#include <Adafruit_NeoPixel.h>                                                                              // Bibliothek für den Neopixel-Streifen einbinden
#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <WiFiUdp.h>
WiFiUDP Udp;

#define LED_PIN     2                                                                                       // An welchem PWM-Ausgang am Arduino sind die NeoPixel angeschlossen?
#define LED_ANZAHL  114                                                                                     // Wie viele NeoPixel sind angeschlossen?


  //Konstanten
const char *ssid     = "WLAN-SSID";                                                                         // SSID und Passwort vom Wlan
const char *password = "Passwort";
static const char ntpServerName[] = "pool.ntp.org";                                                         // NTP-Server
const int timeZone = 1;                                                                                     // Zeitzone
const int NTP_PACKET_SIZE = 48;                                                                             // Die NTP Zeit befindet sich in den ersten 48 bytes der Nachricht

  //Variablen
unsigned long color;
unsigned long color_background;
unsigned long currentMillis;
unsigned long previousMillis = 0;
unsigned long seconds;
int hours;
int minutes;
byte helligkeit=60;                                                                                         // Helligkeit der LEDs (von 0 bis 255)                                                                                 // I²C Addresse für Uhrmodul
byte packetBuffer[NTP_PACKET_SIZE];                                                                         // Buffer zum speichern der eingehenden und ausgehenden Packete
unsigned int localPort = 8888;                                                                              // Lokaler Port zum Empfang der UDP Packete
time_t prevDisplay = 0;                                                                                     // when the digital clock was displayed


Adafruit_NeoPixel strip(LED_ANZAHL, LED_PIN, NEO_GRB + NEO_KHZ800);                                         // Neopixel-Streifen definieren
/*
 * Argument 1 = Anzahl der NeoPixel
 * Argument 2 = Arduino PWM-Pin an dem der LED-Streifen angeschlossen ist
 * Argument 3 = Welche Art von NeoPixel-Streifen wird verwendet?:
 * NEO_KHZ800  800 KHz bitstream (meistverwendet --> NeoPixel w/WS2812 LEDs)
 * NEO_KHZ400  400 KHz (classic 'v1' (nicht v2) FLORA pixels, WS2811 treiber)
 * NEO_GRB     Pixels sind für GRB bitstream vorgesehen (meistverwendet)
 * NEO_RGB     Pixels sind für RGB bitstream vorgesehen (v1 FLORA pixels, not v2)
 * NEO_RGBW    Pixels sind für RGBW bitstream vorgesehen (NeoPixel RGBW Produkte)
*/


//Array eröffnen. Die LEDs, die zur Anzeige des benötigten Wortes nötig sind, werden hier deklariert.
//Der Eintrag -1 ist ein Abbruchkriterium und weist auf das Ende des Wortes/der Wörter hin. (Wird in der Schleife verwendendet)

#define NUM_PIXELS 11                                                                                       // Maximale Anzahl der Pixel für ein/e Zahl/Wort
signed short leds[26][NUM_PIXELS] = {                                                                       // WORT
  {   0,   1,   3,   4,   5,  -1 },                                                                         // ES IST
  {   7,   8,   9,  10,  -1 },                                                                              // FUENF
  {  21,  20,  19,  18,  -1 },                                                                              // ZEHN
  {  26,  27,  28,  29,  30,  31,  32,  -1 },                                                               // VIERTEL
  {  17,  16,  15,  14,  13,  12,  11,  -1 },                                                               // ZWANZIG
  {  44,  45,  46,  47,  -1 },                                                                              // HALB
  {  35,  34,  33,  -1 },                                                                                   // VOR
  {  43,  42,  41,  40,  -1 },                                                                              // NACH
  {  65,  64,  63,  62,  -1 },                                                                              // EINS
  {  58,  57,  56,  55,  -1 },                                                                              // ZWEI
  {  66,  67,  68,  69,  -1 },                                                                              // DREI
  {  73,  74,  75,  76,  -1 },                                                                              // VIER
  {  51,  52,  53,  54,  -1 },                                                                              // FUENF
  {  87,  86,  85,  84,  83,  -1 },                                                                         // SECHS
  {  88,  89,  90,  91,  92,  93,  -1 },                                                                    // SIEBEN
  {  80,  79,  78,  77,  -1 },                                                                              // ACHT
  { 106, 105, 104, 103,  -1 },                                                                              // NEUN
  { 109, 108, 107, 106,  -1 },                                                                              // ZEHN
  {  70,  71,  72,  -1 },                                                                                   // ELF
  {  94,  95,  96,  97,  98,  -1 },                                                                         // ZWOELF
  { 101, 100,  99,  -1 },                                                                                   // UHR
  { 110,  -1 },                                                                                             // 1
  { 110, 111,  -1 },                                                                                        // 2
  { 110, 111, 112,  -1 },                                                                                   // 3
  { 110, 111, 112, 113,  -1 },                                                                              // 4
  {  65,  64,  63,  -1 }                                                                                    // EIN (UHR) - Volle Stunde
};

// Hier wird den Worten eine Zeile aus dem Array zugewiesen. (0 ist die erste Zeile!)
#define ES_IST      0
#define M_FUENF     1
#define M_ZEHN      2
#define VIERTEL     3
#define ZWANZIG     4
#define HALB        5
#define VOR         6
#define NACH        7
#define H_EINS      8
#define H_ZWEI      9
#define H_DREI      10
#define H_VIER      11
#define H_FUENF     12
#define H_SECHS     13
#define H_SIEBEN    14
#define H_ACHT      15
#define H_NEUN      16
#define H_ZEHN      17
#define H_ELF       18
#define H_ZWOELF    19
#define UHR         20
#define M_EINS      21
#define M_ZWEI      22
#define M_DREI      23
#define M_VIER      24
#define H_EIN       25




// Setup wird einmal beim Programmstart ausgeführt!
void setup()  {
  delay(5000);
  Serial.begin(115200);                                                                                     // Serielle Schnittstelle starten
  Serial.print("Verbindung zum Netzwerk ");
  Serial.print(ssid);
  Serial.println(" wird aufgebaut.");
  WiFi.begin(ssid, password);                                                                               // Mit dem Internet verbinden
  Serial.print("Verbinde mit Internet");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("IP Adresse wurde vom DHCP-Server bezogen: ");
  Serial.println(WiFi.localIP());
  Serial.println("Starte UDP");
  Udp.begin(localPort);
  Serial.print("Lokaler Port: ");
  Serial.println(Udp.localPort());
  Serial.println("Warte auf Syncronisation...");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  Serial.println("Zeit wurde abgerufen:");
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
  
  strip.begin();                                                                                            // Initialisieren des NeoPixel Objectes (Treiber installation und ähnliches...)
  strip.show();                                                                                             // Alle Pixel ausschalten
  strip.setBrightness(helligkeit);                                                                          // Wert der Helligkeit wird weiter oben eingestellt.

  color = strip.Color(95,200,95);                                                                           // RGB-Farbe der Uhrzeit
  color_background = strip.Color(0,0,0);                                                                    // RGB-Farbe des Hintergrundes (0,0,0) ist aus.  
}




// Hier folgen jetzt die Funktionen


void setLeds(int i) {                                                                                       //Die Funktion setLeds liest aus der entsprechenden Zeile des Arrays die einzelnen Werte und übergibt sie dem Neopixelstreifen. Pixel für Pixel.
  for (int k = 0; k < NUM_PIXELS; k++) {
    if (leds[i][k] > -1) {
      strip.setPixelColor(leds[i][k], color);
    }
    else {
      break;
    }
  }
}


void displayTime() {                                                                                        //Die Funktion displayTime errechnet werte aus der ausgelesenen Zeit mit denen dann die passenden Worte der Uhrzeit ausgewählt werden.
  minutes = minute();
  
  int mins = minutes - (minutes % 5);                                                                       //Erklärung für die Rechnung mit dem Modulo (%): Beispiel mins=28(minutes)%5 = 3 (Ergebnis ist 5 rest 3. Es wird nur der Rest ausgegeben. Somit ist mins=3)
  int hrs;
  int vor = 0;

  strip.clear();                                                                                            //Strip löschen 

  for (int a = 0; a < LED_ANZAHL; a++) {                                                                    //Allen Uhrzeit-Pixeln wird ein Farbwert zugewiesen. Alle anderen Pixel in einer anderen Farbe (color_background).
    strip.setPixelColor(a, color_background);
  }
  
  setLeds(ES_IST);
  
  switch (mins / 5) {                                                                                       //Hier werden die Minutenworte geschaltet
    case 0:                                                                                                 // Volle Stunde
      setLeds(UHR);
      break;
    case 1:
      setLeds(M_FUENF);
      setLeds(NACH);
      break;
    case 2:
      setLeds(M_ZEHN);
      setLeds(NACH);
      break;
    case 3:
      setLeds(VIERTEL);
      setLeds(NACH);
      break;
    case 4:
      setLeds(ZWANZIG);
      setLeds(NACH);
      break;
    case 5:
      setLeds(M_FUENF);
      setLeds(VOR);
      setLeds(HALB);
      vor = 1;
      break;
    case 6:
      setLeds(HALB);
      vor = 1;
      break;
    case 7:
      setLeds(M_FUENF);
      setLeds(NACH);
      setLeds(HALB);
      vor = 1;
    break;
    case 8:
      setLeds(ZWANZIG);
      setLeds(VOR);
      vor = 1;
      break;
    case 9:
      setLeds(VIERTEL);
      setLeds(VOR);
      vor = 1;
      break;
    case 10:
      setLeds(M_ZEHN);
      setLeds(VOR);
      vor = 1;
      break;
    case 11:
      setLeds(M_FUENF);
      setLeds(VOR);
      vor = 1;
      break;
  }

  switch (minutes % 5) {                                                                                    // Hier werden die LEDs für die Minutenanzeige (1-4) geschaltet.
    case 0:
      break;
    case 1:
      setLeds(M_EINS);
      break;
    case 2:
      setLeds(M_ZWEI);
      break;
    case 3:
      setLeds(M_DREI);
      break;
    case 4:
      setLeds(M_VIER);
      break;
  }

  hrs = hour() + vor;                                                                                       // hrs ist die Angezeigte Stunde. Bei zehn vor vier ist es 15:50Uhr. Daher wird eine Stunde hinzugezählt um von 15 auf 16 zu kommen (zehn VOR 4)
  if (hrs > 12) {                                                                                           // Stunde 13 ist nicht in den Worten enthalten. Daher werden Stundenzahlen größer als 12 auf 0 zurückgesetzt.
    hrs = hrs - 12;
  }
  if (hrs == 0) {                                                                                           // Da wir keine null Uhr haben wird die Stunde null zur stunde 12 gemacht.
    hrs = 12;
  }

                                                                                                            // STUNDEN
  switch (hrs) {
    case 1:
      if(mins/5==0)  {
        setLeds(H_EIN);
        break;
      }
    else  {
      setLeds(H_EINS);
      break;
    }
    case 2:
      setLeds(H_ZWEI);
      break;
    case 3:
      setLeds(H_DREI);
      break;
    case 4:
      setLeds(H_VIER);
      break;
    case 5:
      setLeds(H_FUENF);
      break;
    case 6:
      setLeds(H_SECHS);
      break;
    case 7:
      setLeds(H_SIEBEN);
      break;
    case 8:
      setLeds(H_ACHT);
      break;
    case 9:
      setLeds(H_NEUN);
      break;
    case 10:
      setLeds(H_ZEHN);
      break;
    case 11:
      setLeds(H_ELF);
      break;
    case 12:
      setLeds(H_ZWOELF);
      break;
  }
  strip.show();                                                                                             // Zeit anzeigen
}


void WiFiinit() {
  while(WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi ist nicht verbunden. Verbindung wird neu aufgebaut.");
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      delay(100);
  }

}




/*-------- NTP code ----------*/

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Übertrage die NTP-Anfrage");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Empfange Daten vom NTP-Server");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("Keine Antwort vom NTP-Server :-(");
  return 0; // return 0 if unable to get the time
}





     // send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}




void loop()  {                                                                                              // Diese Funktion wird in einer nie endenden Schleife ausgeführt! Zeit vom Timeserver lesen, Zeit anzeigen, 250ms warten und wieder von vorne.
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      displayTime();
    }
  }
}