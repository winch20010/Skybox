//SkyBox Receiver  - Sebastien Burfin
//Revision 3.1
//June 1st 2016
//Add Average for the sky light measure send it to intranet
//Revision 3.2
//June 2nd 2016
//Code cleanup
//Revision 3.3
//June 17 2016
//Add revision in serial output
//Revision 3.4
//June 18 2016
//Add motor protection with switches
//Revision 3.5
//August 8th 2016
//Add Serial control for roof
//Revision 3.6
//August 8th 2016
//Bug correction serial
//Revision 3.7
//August 11th 2016
//Full rewrite of Serial + button events
//Revision 3.8
//August 11th 2016
//All working except serial string reception
//Revision 3.9
//August 12th 2016
//Use analog pin instead of digital 0 for end course switches
//Revision 4.0
//August 12th 2016
//Full revision of serial emergency stop procedure

#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile
#include <Ethernet.h>


///////////////ETHERNET INITIALIZATION////////////////////////////
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x92, 0x34 };

// The default Arduino board address:
byte ip[] = { 192, 168, 74, 17 };
 
// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// The DNS server address:
byte dnss[] = { 192, 168, 74, 1 };
// the gateway router address:
byte gateway[] = { 192, 168, 74, 1 };
// the subnet mask:
byte subnet[] = { 255, 255, 255, 0 };

///////////////////////////////////////////////////////

////////////Radio Frequency initialization/////////////

//Radio Frequency call
RH_ASK driver(2000, 8);

///////////////////////////////////////////////////////

//////////////VARIABLES////////////////////////////////

struct dataStruct{
  double tempciel ;
  double temp_ambient; 
  double detectpluie;
  double temp;
  double sqmval;
  unsigned long counter;
}myData;

//RTC_DS1307 RTC;
//PUSHBUTTONS
const int buttonPin1 = 2;
const int buttonPin2 = 7;
const int toitferme = 16;
const int toitouvert = 17;

int buttonClose = 0; 
int buttonOpen = 0; 
int switchOuvert = 0; 
int switchFerme = 0; 
int minutes = 90;


unsigned long time1;
unsigned long time2;
unsigned long timemotor;

#define RELAY1  6                       
#define RELAY2  5

char temperature[6];
char combinedArray;

String data1 = "";
String valSerial;

boolean button = false;
boolean sensouverture = false;
boolean sensfermeture = false;

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

//////////////////////////////////////////////

/////////////BEGIN SETUP//////////////////////
void setup()
{

  //PUSHBUTTONS
  pinMode(buttonPin1, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(toitferme, INPUT_PULLUP);
  pinMode(toitouvert, INPUT_PULLUP);
  pinMode(RELAY1, OUTPUT);       
  pinMode(RELAY2, OUTPUT);
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);

  
  Serial.begin(9600); // Debugging only
  Serial.flush();
  Serial.println("Version 4.0");
  
  Serial.println("setup()");
 
    ///////////////ETHERNET//////////////////
    if (Ethernet.begin(mac) == 0) {
    // if DHCP fails, start with a hard-coded address:
    Serial.println("failed to get an IP address using DHCP, trying manually");
    Ethernet.begin(mac, ip); // function returns void
    //Ethernet.begin(mac, ip, gateway, gateway, subnet); // function returns void
  }
  else
    Serial.println("got an IP address using DHCP");

  /////////////Radio Frequency//////////////
  if (!driver.init())
    Serial.println("init failed");

  ////////////Ethernet Listen/////////////

  server.begin();
  // Say who we think we are.
  Serial.println(Ethernet.localIP());
  delay(1000);

  
}
 

//////////BEGIN LOOP//////////////
void loop()
{
  time1 = millis();
  control();

  //////////BUTTONS + RELAYS/////////////


  //Serial Roof command

  if (stringComplete) {

 
 //   Serial.println("info recu");


  //Open roof if data received = ouvrir, time less than 50sec and limit switch not activated
  if (inputString == "OUVRIR$" && switchOuvert == HIGH ) { 
     
      sensouverture = true;
      //Serial.println("j ouvre");
      digitalWrite(RELAY1,LOW);
      digitalWrite(RELAY2,HIGH);
      control();
  
  
  }
  //Close roof if data received = fermer, time less than 50sec and limit switch not activated
  else if (inputString == "FERMER$" && switchFerme == HIGH) {
     
      sensfermeture = true;
      //Serial.println("je ferme");
      digitalWrite(RELAY1,HIGH);
      digitalWrite(RELAY2,LOW);
      control();
    
  
  }
      else  if (inputString == "STOP$") {
      stop();

    }
  }
  // check which pushbutton is pressed.

  if (buttonClose == HIGH && buttonOpen == LOW && switchOuvert == HIGH) {
    button = true;
    sensouverture = true;
    //Hard button open roof
    Serial.println("j ouvre avec bouton");
    digitalWrite(RELAY1,LOW);
    digitalWrite(RELAY2,HIGH);
 
    
  } else if (buttonClose == LOW && buttonOpen == HIGH && switchFerme == HIGH) {
    button = true;
    sensfermeture = true;
    //Hard button close roof
    Serial.println("je ferme avec bouton");
    digitalWrite(RELAY1,HIGH);
    digitalWrite(RELAY2,LOW);
  } 

  ///////////////ETHERNET//////////////////////

  EthernetClient client = server.available();
 
  /////////////////////////////////////////////

  ////////////RF433 SECTION/////////////////////
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

  if (driver.recv(buf, &buflen)) {
    int i;

    // Message with a good checksum received, dump it.
    driver.printBuffer("Got:", buf, buflen);
    if (buflen == 24) {
      memcpy(&myData, buf, sizeof(myData));
    }
  }

  //////////////////////////////////////////////////////

  /////////////PREPARE DATA TO BE SENT TO INTERNET/////////////////
  //Initialization of variables received by RF433
  char str_temp[6];
  char str_temp2[6];
  char str_temp3[6];
  char str_temp4[6];
  char str_temp5[6];

  //Size of data received
  char combinedArray[84];

  //Affect variables to values received by the RF433
  double tempcield = myData.tempciel;
  double detecpluid = myData.detectpluie;
  double tempambient = myData.temp_ambient;
  double tempsol = myData.temp;
  double mysqm = myData.sqmval;


  //Convert variables in String
  // 4 is mininum width, 2 is precision; float value is copied onto str_temp
  dtostrf(tempcield, 5, 2, str_temp);
  dtostrf(detecpluid, 5, 2, str_temp2);
  dtostrf(tempambient, 5, 2, str_temp3);
  dtostrf(tempsol, 5, 2, str_temp4);
  dtostrf(mysqm, 5, 2, str_temp5);
 
  //Define attributes to send via Ethernet
  char myData1[] = "temperatureciel=";
  char myData2[] = "&detectionpluie=";
  char myData3[] = "&tempambient=";
  char myData4[] = "&tempsol=";
  char myData5[] = "&sqmaverage=";

  //Concatenate all variables + attributes to be sent via Ethernet

  sprintf(combinedArray, "%s%s%s%s%s%s%s%s%s%s", myData1, str_temp, myData2,str_temp2,myData3,str_temp3,myData4,str_temp4,myData5,str_temp5);

  ///////////////////////////////////////////////////

  //////////////////SEND ETHERNET///////////////////////

  if (time1 >= time2) {

    //Send Every 2 minutes

    time2 += 120000;
    Serial.println("ca envoie");

    if (client.connect("192.168.74.5",83)) { // REPLACE WITH YOUR SERVER ADDRESS
 
      client.println("POST /observatory/add.php HTTP/1.1"); 
      client.println("Host: 192.168.74.5"); // SERVER ADDRESS HERE TOO
      client.println("Content-Type: application/x-www-form-urlencoded"); 
      client.print("Content-Length: "); 
      client.println(strlen(combinedArray)); 
      client.println(); 
      client.print(combinedArray); 
      client.println();
   
    } 

    //Disconnect

    if (client.connected()) { 
      client.stop();  // DISCONNECT FROM THE SERVER
    }
  }

}

////////////END LOOP/////////////////////

////////FUNCTION CONTROL/////////////////
//This function controls events and button states
//And stop motors if any issue occurs

void control(){

  buttonClose = digitalRead(buttonPin1);
  buttonOpen = digitalRead(buttonPin2);
  switchOuvert = digitalRead(toitouvert);
  switchFerme = digitalRead(toitferme);

  


 // if (switchOuvert == 0) {
 //   Serial.println("c est ouvert ...");
 // }

  if (buttonClose == LOW && buttonOpen == LOW ) {
   
    Serial.println("APPUI 2 BOUTONS");
    stop();
  }
  else if (buttonClose == HIGH && buttonOpen == HIGH && (button)) {
    stop();
    Serial.println("je coupe toutv2");
  }

  else if (((sensouverture) && switchOuvert == LOW) || ((sensfermeture) && switchFerme == LOW)) {
    Serial.println("FIN COURSE");
    stop();
  }
 
else if ((timemotor < millis()) && inputString != "" ) {
    Serial.println("TEMPS PASSE");
    stop();
  }
  
}

void stop() {
    Serial.println("je stop");
    digitalWrite(RELAY1,HIGH);
    digitalWrite(RELAY2,HIGH);
    button = false;
    sensouverture = false;
    sensfermeture = false;
    inputString = "";
    stringComplete = false;
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    valSerial += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '$' ) {
      if (valSerial == "STOP$" || valSerial == "OUVRIR$" || valSerial == "FERMER$" ) {
        stringComplete = true;
        timemotor = millis() + 28000;
        inputString = valSerial;
        valSerial = "";
      }
      else if (valSerial == "ETAT$") {
             
	      if (switchFerme == LOW) {
    	    Serial.println("FERME");
        }
      	else if (switchOuvert == LOW) {
        	Serial.println("OUVERT");
      	}
      	else {
	        Serial.println("UNKNOWN");
    	  }
      }
      
      valSerial = "";
    }
  }
}
