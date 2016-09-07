//SkyBox Transmitter  - Sebastien Burfin
//Revision 3.1
//May 31st 2016
//Revision 3.2
//June 1st 2016
//Add Average for the sky light measure and change the LCD prints


//Library definitions

#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile
#include <OneWire.h> // Inclusion de la librairie OneWire
#include <LiquidCrystal.h>
#include <Adafruit_MLX90614.h>

//Lcd pinout definition
LiquidCrystal lcd(9,7, 11, 8, 3, 4, 5);

//Sky Quality meter variables
const float A = 22.0;
double Msqm;
int incaver = 0 ;
double msqmaver[10];
double MsqmMoy;
double sum;

//Define sensors
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
#define DS18B20 0x28     // Adresse 1-Wire du DS18B20
#define BROCHE_ONEWIRE 10 // Broche utilisÃ©e pour le bus 1-Wire
OneWire ds(BROCHE_ONEWIRE); // CrÃ©ation de l'objet OneWire ds

//Define Radio Frequency Sensor
RH_ASK driver;

//PUSHBUTTONS
const int buttonPin1 = 6;
int buttonState1 = 0; 
int LCDPIN = A2;
int increment;
int buttonState;
int lastButtonState = LOW;   // the previous reading from the input pin
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

//Temperatures variables
String  SerialDegC, infos;
char SerialDegree[3];
char valuetoprint[20];
char valuetoprint2[20];
//Define time for backlight to switchoff
unsigned long time1 = 0;
unsigned long time2 = 60000 ;

//Define Values to send via RF433
//struct dataStruct{
  float temp_ciel ; 
  float temp_ambient;
  float humid;
  float tempsol;
 // float sqmval;
  unsigned long counter;
//}myData;
//byte tx_buf[sizeof(myData)] = {0};



//Define interrupt to measure light sensitivity
volatile unsigned long cnt = 0;
unsigned long oldcnt = 0;
unsigned long t = 0;
unsigned long last;
unsigned long timefreq;



const byte interruptPin = 2;
unsigned long pulse_cnt = 0;

 char str_temp_ciel[11];
 char str_tempsol[11];
 char str_temp_ambient[11];
 char str_Msqm[11];
 char str_MsqmMoy[11];
 char str_counter[32];
 char str_humid[11];
 
///////////START OF SETUP//////////////////////

void setup() {
  //Light to frequency Interrup
  attachInterrupt(digitalPinToInterrupt(interruptPin), add_pulse, RISING);
  
  //LCD PushButton
  pinMode(buttonPin1,INPUT_PULLUP );
  pinMode(LCDPIN, OUTPUT);
  digitalWrite(LCDPIN, HIGH);
  increment = 0;


  //LCD Degree character definition
  SerialDegC += char(223);           // Setup a Degrees C Serial symbol
  SerialDegC += "C";
  
  SerialDegC.toCharArray(SerialDegree,3);

  
  Serial.begin(9600);

  //Sky temperature sensor initialization
  mlx.begin(); 
  if (!driver.init())
       Serial.println("init failed");
         
  
  //LCD screen initialization
        defaultlcd();

}

////////////END OF SETUP///////////////////
////////////START OF LOOP/////////////////////

void loop() {

  //LCD Backlight switchoff time calculation
  time1 = millis();
  if (time1 > time2) {
    digitalWrite(LCDPIN, LOW);
  }

  //Light To Frequency Measure
  if (millis() - last >= 2000) {
    Serial.print ("delai : ");
    Serial.println (millis() - last); 
    timefreq = millis() - last;
    last = millis();
    t = cnt;
    unsigned long hz = (t - oldcnt)/ (timefreq / 1000);
    oldcnt = t;
    Msqm = A - 2.5*log10(hz); //Frequency to magnitudes/arcSecond2 formula
      msqmaver[incaver] = Msqm;
       incaver++;
    //Serial.print("FREQ: "); 
    //Serial.print(hz); 
    //Serial.print("MSQM : ");
    //Serial.println(Msqm);

    
  }
 
   if (incaver == 10) {
      for (int i = 0 ; i < 10 ; i++){
    sum += msqmaver[i] ;
    MsqmMoy = sum / 10;
    //sqmval = MsqmMoy;
    }
    sum = 0;
      incaver = 0 ;
    }

        for (int i = 0 ; i < 10 ; i++){
   // Serial.print(msqmaver[i]);
   // Serial.print(";");
    }
   // Serial.print ("Moy : ");
   // Serial.println(MsqmMoy);

  //DS18B20 Temperature Sensor reading
  float temp;
  //  Lit la tempÃ©rature ambiante Ã  ~1Hz
  if(getTemperature(&temp)) {
    
        // Affiche la tempÃ©rature
  //  Serial.print("Temperature : ");
   // Serial.print(temp);
    
        Serial.write(176); // caractÃ¨re Â°
   // Serial.write('C');
  //  Serial.println();

   
        tempsol = temp;
  }


    //MLX 6014 Sky temperature reading
 // Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC()); 
 // Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC());  Serial.write(176);  Serial.write('C');Serial.println();
  
  temp_ciel=mlx.readObjectTempC();
  temp_ambient = mlx.readAmbientTempC();
  



  
  //Rain sensor reading
  float humivalue = analogRead(A0); 
  humid = map(humivalue, 0, 1024, 100, 0); 
 //// Serial.println(myData.humid);
 
  dtostrf(temp_ciel, 5, 2, str_temp_ciel);
  dtostrf(temp_ambient, 5, 2, str_temp_ambient);
dtostrf(tempsol, 5, 2, str_tempsol);
dtostrf(Msqm, 5, 2, str_Msqm);
dtostrf(MsqmMoy, 5, 2, str_MsqmMoy);
dtostrf(humid, 5, 2, str_humid);

  //LCD Screen reading and actions
  buttonState1 = digitalRead(buttonPin1);
    char convert[11];
    char convert2[11];

  
  //When Pushbutton is pressed :
  if (buttonState1 == HIGH) {
    lastButtonState =1;
  }
  
  if (buttonState1 != lastButtonState) {
    lastButtonState = 0;

  if (buttonState1 == LOW) {
 
    increment++;
    time2 = millis() + 60000;
    digitalWrite(LCDPIN, HIGH);
      lcd.clear();

  }
  }
  
    //What to print on LCD Screen :
    switch (increment) {
      //Print DS18b20 Temp sensor value
      case 1:
      lcd.setCursor(0, 0);
      lcd.print("Temp Sonde");
      lcd.setCursor(0, 1);
      // print the number of seconds since reset:
      // lcd.print(myData.temp);
   //   dtostrf(tempsol, 5, 2, convert);
      sprintf(valuetoprint, "%s%s", str_tempsol, SerialDegree);
      lcd.print(valuetoprint);
      break;

      //Print Ambiant Temperature from Sky sensor
      case 2:
      lcd.setCursor(0, 0);
      lcd.print("Temp Ambiente");
      lcd.setCursor(0, 1);
 //     dtostrf(temp_ambient, 5, 2, convert);
      sprintf(valuetoprint, "%s%s", str_temp_ambient, SerialDegree);
      lcd.print(valuetoprint);
      break;

      //Print Sky Temperature
      case 3:
      lcd.setCursor(0, 0);
      lcd.print("Temp Ciel");
      lcd.setCursor(0, 1);
 //     dtostrf(temp_ciel, 5, 2, convert);
      sprintf(valuetoprint, "%s%s", str_temp_ciel, SerialDegree);
      lcd.print(valuetoprint);
      break;

      //Print SQM Value from Light sensor
      case 4:
      lcd.setCursor(0, 0);
      lcd.print("SQM    Average");
      lcd.setCursor(0, 1);
   //   dtostrf(Msqm, 5, 2, convert);
      sprintf(valuetoprint, "%s", str_Msqm );
      lcd.print(valuetoprint);
      lcd.setCursor(8, 1);
   //   dtostrf(MsqmMoy, 5, 2, convert);      
      sprintf(valuetoprint2, "%s", str_MsqmMoy );
      lcd.print(valuetoprint2);
      break;

      //Default Menu
      default:
      increment = 0;
      defaultlcd();
      break;
    }

     
 

  //Send data out of RF433

 char DatatoSend[60];
 sprintf(DatatoSend, "%s,%s,%s,%s,%s,%s.",str_temp_ciel,str_temp_ambient,str_humid,str_tempsol,str_MsqmMoy,str_counter);
 // memcpy(tx_buf, DatatoSend, sizeof(DatatoSend) );
  byte zize=sizeof(DatatoSend);

  driver.send((uint8_t *)DatatoSend, zize);
  driver.waitPacketSent(300);
 
  counter++;
  ltoa(counter, str_counter, 10);
  //   delay(600);

}

//////////END OF LOOP //////////////



//////////START OF FUNCTIONS//////////

//////////////////////////////////////
/// Light to Frequency interrup count
//////////////////////////////////////
void add_pulse() {

  // increase pulse count
  cnt++;
  return;
}
//////////////////////////////////////



//////////////////////////////////////////////////////
// Fonction rÃ©cupÃ©rant la tempÃ©rature depuis le DS18B20
// Retourne true si tout va bien, ou false en cas d'erreur
//////////////////////////////////////////////////////
boolean getTemperature(float *temp){
  byte data[9], addr[8];
  // data : DonnÃ©es lues depuis le scratchpad
  // addr : adresse du module 1-Wire dÃ©tectÃ©

  if (!ds.search(addr)) { // Recherche un module 1-Wire
      ds.reset_search();    // RÃ©initialise la recherche de module
      return false;         // Retourne une erreur
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) // VÃ©rifie que l'adresse a Ã©tÃ© correctement reÃ§ue
      return false;                        // Si le message est corrompu on retourne une erreur

  if (addr[0] != DS18B20) // VÃ©rifie qu'il s'agit bien d'un DS18B20
      return false;         // Si ce n'est pas le cas on retourne une erreur

  ds.reset();             // On reset le bus 1-Wire
  ds.select(addr);        // On sÃ©lectionne le DS18B20

  ds.write(0x44, 1);      // On lance une prise de mesure de tempÃ©rature
  delay(200);             // Et on attend la fin de la mesure
  
  ds.reset();             // On reset le bus 1-Wire
  ds.select(addr);        // On sÃ©lectionne le DS18B20
  ds.write(0xBE);         // On envoie une demande de lecture du scratchpad

    for (byte i = 0; i < 9; i++) // On lit le scratchpad
      data[i] = ds.read();       // Et on stock les octets reÃ§us
  
    // Calcul de la tempÃ©rature en degrÃ© Celsius
    *temp = ((data[1] << 8) | data[0]) * 0.0625; 
  
    // Pas d'erreur
    return true;
}
///////////////////////////////////

///////DEFAULT LCD printout///////////
void defaultlcd() {
    lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(0, 0);
  lcd.print("WINCH SKYBOX");
  lcd.setCursor(0, 1);
  lcd.print("V3.2 JUIN 2016");          
}
//////////////////////////////////////

////////////////////END OF FUNCTIONS///////////////////////
