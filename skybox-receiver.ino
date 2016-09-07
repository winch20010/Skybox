//SkyBox Receiver  - Sebastien Burfin
//Revision 5.3
//September 07th 2016
//Rewrite Serial event with new strategy code
//Fix issue with combinedArray size buffer overflow
//Revision 5.4
//September 07th 2016
//Fix memory issue which was causing most of the problem on the ardui ...

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

  double tempciel ;
  double temp_ambient; 
  double detectpluie;
  double temp;
  double sqmval;

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

unsigned long time1 = millis();
unsigned long time2 = millis() + 120000;
unsigned long timemotor;

#define RELAY1  6                       
#define RELAY2  5

char temperature[6];
char post[] = "POST /observatory/add.php HTTP/1.1";
char post2[] = "POST /observatory/safety.php HTTP/1.1";

String data1 = "";
//String valSerial;

boolean button = false;
boolean sensouverture = false;
boolean sensfermeture = false;
//boolean gotdata = false;


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

    /////////////Radio Frequency//////////////
  if (!driver.init())
    Serial.println("init failed");
    
  Serial.println("Version 5.4");
  
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
  ////////////Ethernet Listen/////////////

  server.begin();
  // Say who we think we are.
  Serial.println(Ethernet.localIP());
}

//////////BEGIN LOOP//////////////
void loop()
{
  time1 = millis();
  control();
   ////////////RF433 SECTION/////////////////////
  uint8_t buf[30];
  uint8_t buflen = sizeof(buf);

  if (driver.recv(buf, &buflen)) {
    
Serial.println("RX...");

   tempciel = atof(strtok(buf, ","));
   temp_ambient = atof(strtok(NULL, ","));
   detectpluie = atof(strtok(NULL, ","));
   temp = atof(strtok(NULL, ","));
   sqmval = atof(strtok(NULL, ","));
//   counter = atol(strtok(NULL, ","));
  }

  //////////////////////////////////////////////////////
 
 //////////////SERIAL INPUT /////////////////
   static char bufserial[10];
  if (readline(Serial.read(), bufserial, 10) > 0) {
    String valSerial = bufserial;
    Serial.print(valSerial);
   //Open roof if data received = ouvrir, time less than 50sec and limit switch not activated
   if (valSerial == "OUVRIR" && switchOuvert == HIGH ) { 
      timemotor = millis() + 28000;
      sensouverture = true;
      Serial.println("j ouvre");
      digitalWrite(RELAY1,LOW);
      digitalWrite(RELAY2,HIGH);
      control();
  
  }
  //Close roof if data received = fermer, time less than 50sec and limit switch not activated
   else if (valSerial == "FERMER" && switchFerme == HIGH) {
      timemotor = millis() + 28000;
      sensfermeture = true;
      Serial.println("je ferme");
      digitalWrite(RELAY1,HIGH);
      digitalWrite(RELAY2,LOW);
      control();
 
  }
      else   if (valSerial == "STOP") {
      stop();
    }
    //Give status of the roof
         else  if (valSerial == "ETAT") {
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
  }
//////////////////////////////////////////

  //////////BUTTONS + RELAYS/////////////

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

  /////////////PREPARE DATA TO BE SENT TO INTERNET/////////////////
  //Initialization of variables received by RF433
  char str_temp1[6];
  char str_temp2[6];
  char str_temp3[6];
  char str_temp4[6];
  char str_temp5[6];

 
  //Convert variables in String
  // 4 is mininum width, 2 is precision; float value is copied onto str_temp
  dtostrf(tempciel, 5, 2, str_temp1);
  dtostrf(detectpluie, 5, 2, str_temp2);
  dtostrf(temp_ambient, 5, 2, str_temp3);
  dtostrf(temp, 5, 2, str_temp4);
  dtostrf(sqmval, 5, 2, str_temp5);
 
  //Define attributes to send via Ethernet
  char myData1[] = "temperatureciel=";
  char myData2[] = "&detectionpluie=";
  char myData3[] = "&tempambient=";
  char myData4[] = "&tempsol=";
  char myData5[] = "&sqmaverage=";

  //Concatenate all variables + attributes to be sent via Ethernet
//  Serial.println(sizeof(myData1));
  char combinedArray[sizeof(myData1) + sizeof(str_temp1) +sizeof(myData2) + sizeof(str_temp2) + sizeof(myData3) + sizeof(str_temp3)+ sizeof(myData4) + sizeof(str_temp4) +sizeof(myData5) + sizeof(str_temp5) + 1];
  sprintf(combinedArray, "%s%s%s%s%s%s%s%s%s%s", myData1, str_temp1, myData2,str_temp2,myData3,str_temp3,myData4,str_temp4,myData5,str_temp5);

  ///////////////////////////////////////////////////

////////SAFETY CONTROL IF RECEIVED RF DATA//////////////

    //If Value received  safe, then safety = safe
         if (((sqmval <= 2) && (detectpluie < 1) && (tempciel <= -10)) && ((switchFerme == HIGH)&&(switchOuvert == LOW))) {
   safebool = true;
   char safety[]="safety=safe";
 //  gotdata = false;
         }
         
    //If value receifed not safe, then safety = nosafe
  else  {
   char safety[]="safety=nosafe";
   safebool = false;
 
    }

  if (safestate != safebool) {
  //   Serial.println("different");
      safestate = safebool;
     iptrans(post2, safety);
  }

  //////////////////UPDATE SQL DATABASE WITH SENSORS DATA///////////////////////

  if (time1 >= time2) {

    //Send Every 2 minutes

    time2 += 120000;
    if (sqmval != 0 && detectpluie !=0) {
        Serial.println("ca envoie");
        iptrans(post, combinedArray);
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
////////////////////////////////////////////////////

///////FUNCTION STOP - STOP MOTORS ////////////////
void stop() {
    Serial.println("je stop");
    digitalWrite(RELAY1,HIGH);
    digitalWrite(RELAY2,HIGH);
    button = false;
    sensouverture = false;
    sensfermeture = false;
//    valSerial = "";
}


/////////ETHERNET SEND DATA CONTROL TO NAS////////////
void iptrans(char post[], char combinedArray[]) {
 
    ///////////////ETHERNET init//////////////////////
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
    //Disconnect

    if (client.connected()) { 
      client.stop();  // DISCONNECT FROM THE SERVER
    }
}

///////////INPUT SERIAL RECEIVED////////////////////////
int readline(int readch, char *bufserial, int len)
{
  static int pos = 0;
  int rpos;

  if (readch > 0) {
    switch (readch) {
      case '\n': // Ignore new-lines
        break;
      case '$': // Return on CR
        rpos = pos;
        pos = 0;  // Reset position index ready for next time
        return rpos;
      default:
        if (pos < len-1) {
          bufserial[pos++] = readch;
          bufserial[pos] = 0;
        }
    }
  }
  // No end of line has been found, so return -1.
  return -1;
}
///////////////////////////////////////////////////
