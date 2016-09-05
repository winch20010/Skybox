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
//Revision 4.1
//August 15th 2016
//Add STATUS variables
//Revision 4.2
//August 26th 2016
//Add FaileStatus checks
//Revision 4.3
//August 27th 2016
//Review safety code and bug fixes
//Revision 5.0.1
//September 04th 2016
//Safety issue correction (no serial print in the safety loop)
//Revision 5.1
//September 05th 2016
//Remove Serial event and check serial input in the loop function

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
unsigned long time2 = 120000;
unsigned long timemotor;

#define RELAY1  6                       
#define RELAY2  5

char temperature[6];
char combinedArray;
char post[] = "POST /observatory/add.php HTTP/1.1";
char post2[] = "POST /observatory/safety.php HTTP/1.1";

String data1 = "";
String valSerial;

boolean button = false;
boolean sensouverture = false;
boolean sensfermeture = false;
boolean gotdata = false;


char safety[] = "safety=nosafe";
boolean safestate = true;
 boolean safebool = false;

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
  Serial.println("Version 5.1");
  
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
//  serialev();
  //////////BUTTONS + RELAYS/////////////


  //Serial Roof command


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



  ////////////RF433 SECTION/////////////////////
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

  if (driver.recv(buf, &buflen)) {
    
Serial.println("jerecois");
    // Message with a good checksum received, dump it.
    driver.printBuffer("Got:", buf, buflen);
    if (buflen == 24) {
            gotdata = true;
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
  
  if  (gotdata) {
    
         if (((mysqm <= 2) && (detecpluid < 1) && (tempcield <= -10)) && ((switchFerme == HIGH)&&(switchOuvert == LOW))) {
   safebool = true;
   sprintf(safety, "%s%s", "safety=", "safe");
   gotdata = false;
         }
  else  {
   sprintf(safety, "%s%s", "safety=", "nosafe");
   safebool = false;
    gotdata = false;
    }
    //  Serial.println(safety);
  
  }

  if (safestate != safebool) {
  //   Serial.println("different");
      safestate = safebool;
      iptrans(post2, safety);
  }
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
    iptrans(post, combinedArray);

  }

//valSerial = "";
while (Serial.available() > 0) {
  delay(10);
        valSerial = Serial.readStringUntil('\n');
Serial.println(valSerial);
        if (valSerial == "ETAT$") {
             
        if (switchFerme == LOW) {
          Serial.println("FERME$");
        }
        else if (switchOuvert == LOW) {
          Serial.println("OUVERT$");
        }
        else if (sensouverture) {
          Serial.println("OUVERTURE$");
        }
        else if (sensfermeture) {
          Serial.println("FERMETURE$");
        }
        else {
          Serial.println("UNKNOWN$");
        }

        }

       else   if (valSerial == "OUVRIR$" && switchOuvert == HIGH ) { 
      timemotor = millis() + 28000;
      sensouverture = true;
  //    Serial.println("j ouvre");
      digitalWrite(RELAY1,LOW);
      digitalWrite(RELAY2,HIGH);


 
  }
        else  if (valSerial == "STOP$") {
         // vals = "";
      stop();
        }

          else if (valSerial == "FERMER$" && switchFerme == HIGH) {
      timemotor = millis() + 28000;
      sensfermeture = true;
  //    Serial.println("je ferme");
      digitalWrite(RELAY1,HIGH);
      digitalWrite(RELAY2,LOW);

    
  
  }
}
valSerial = "";
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
 
else if ((timemotor < millis()) && ((sensfermeture) || (sensouverture))) {
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
    valSerial = "";
 

}

void iptrans(char post[], char combinedArray[]) {
    ///////////////ETHERNET//////////////////////
Serial.println("j envoie ca : ");
Serial.println(post);
Serial.println(combinedArray);

  EthernetClient client = server.available();
 
  /////////////////////////////////////////////
     if (client.connect("192.168.74.5",83)) { // REPLACE WITH YOUR SERVER ADDRESS
 
      client.println(post); 
      client.println("Host: 192.168.74.5"); // SERVER ADDRESS HERE TOO
      client.println("Content-Type: application/x-www-form-urlencoded"); 
      client.print("Content-Length: "); 
      client.println(strlen(combinedArray)); 
      client.println(); 
      client.print(combinedArray); 
      client.println();
   
    } 
delay(200);
    //Disconnect

    if (client.connected()) { 
      client.stop();  // DISCONNECT FROM THE SERVER
    }
}

void serialev(String vals) {
  Serial.print("val : " );
  Serial.println(vals);

  //Open roof if data received = ouvrir, time less than 50sec and limit switch not activated
  if (vals == "OUVRIR$" && switchOuvert == HIGH ) { 
      timemotor = millis() + 28000;
      sensouverture = true;
  //    Serial.println("j ouvre");
      digitalWrite(RELAY1,LOW);
      digitalWrite(RELAY2,HIGH);
     //   vals = "";
    //  control();
  
  
  }
  //Close roof if data received = fermer, time less than 50sec and limit switch not activated
  else if (vals == "FERMER$" && switchFerme == HIGH) {
      timemotor = millis() + 28000;
      sensfermeture = true;
  //    Serial.println("je ferme");
      digitalWrite(RELAY1,HIGH);
      digitalWrite(RELAY2,LOW);
    //            vals = "";
    //  control();
    
  
  }
      else  if (vals == "STOP$") {
          vals = "";
      stop();
       

    }
 
      else if (vals == "ETAT$") {
             
        if (switchFerme == LOW) {
          Serial.println("FERME$");
        }
        else if (switchOuvert == LOW) {
          Serial.println("OUVERT$");
        }
        else if (sensouverture) {
          Serial.println("OUVERTURE$");
        }
        else if (sensfermeture) {
          Serial.println("FERMETURE$");
        }
        else {
          Serial.println("UNKNOWN$");
        }

      }

else {
  Serial.println("Value not accepted");
 
  }
//return;
  //vals = "";

}
