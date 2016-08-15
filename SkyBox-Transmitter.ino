//SkyBox Transmitter  - Sebastien Burfin
//Revision 3.1
//June 2016


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


//Define sensors
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
#define DS18B20 0x28     // Adresse 1-Wire du DS18B20
#define BROCHE_ONEWIRE 10 // Broche utilisée pour le bus 1-Wire
OneWire ds(BROCHE_ONEWIRE); // Création de l'objet OneWire ds

//Define Radio Frequency Sensor
RH_ASK driver;

//PUSHBUTTONS
const int buttonPin1 = 6;
int buttonState1 = 0; 
int LCDPIN = A2;
int increment;
//Temperatures variables
String  SerialDegC, infos;
char SerialDegree[3];
char valuetoprint[20];
//Define time for backlight to switchoff
unsigned long time1 = 0;
unsigned long time2 = 60000 ;

//Define Values to send via RF433
struct dataStruct{
  float temp_ciel ; 
  float temp_ambient;
  float humid;
  float temp;
  unsigned long counter;
}myData;
byte tx_buf[sizeof(myData)] = {0};


//Define interrupt to measure light sensitivity
volatile unsigned long cnt = 0;
unsigned long oldcnt = 0;
unsigned long t = 0;
unsigned long last;
unisgned long timefreq;



const byte interruptPin = 2;
unsigned long pulse_cnt = 0;

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
	lcd.begin(16, 2);
	// Print a message to the LCD.
	lcd.setCursor(0, 0);
	lcd.print("WINCH INTERFACE");
	lcd.setCursor(0, 1);
	lcd.print("V1.0 2016");          
	delay(3000);  
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
		Serial.print("FREQ: "); 
		Serial.print(hz);
		Serial.print("\t = "); 
		Serial.println("MSQM : ");
		Serial.println(Msqm);
		
	}


	//DS18B20 Temperature Sensor reading
 	float temp;
  	Lit la température ambiante �  ~1Hz
	if(getTemperature(&temp)) {
    
    		// Affiche la température
		Serial.print("Temperature : ");
		Serial.print(temp);
    
    		Serial.write(176); // caractère °
		Serial.write('C');
		Serial.println();

   
    		myData.temp = temp;
	}


  	//MLX 6014 Sky temperature reading
	Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC()); 
	Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC());  Serial.write(176);  Serial.write('C');Serial.println();
  
	myData.temp_ciel=mlx.readObjectTempC();
	myData.temp_ambient = mlx.readAmbientTempC();


	//Rain sensor reading
	float humivalue = analogRead(A0); 
	myData.humid = map(humivalue, 0, 1024, 100, 0); 
	Serial.println(myData.humid);


	//LCD Screen reading and actions
	buttonState1 = digitalRead(buttonPin1);

	char convert[11];
	//When Pushbutton is pressed :
	if (buttonState1 == LOW) {
		increment++;
		time2 = millis() + 59900;
		delay(100);
		digitalWrite(LCDPIN, HIGH);
  		lcd.clear();
		//What to print on LCD Screen :
		switch (increment) {
			//Print DS18b20 Temp sensor value
			case 1:
			lcd.setCursor(0, 0);
			lcd.print("Temp Sonde");
			lcd.setCursor(0, 1);
			// print the number of seconds since reset:
			// lcd.print(myData.temp);
			dtostrf(myData.temp, 5, 2, convert);
			sprintf(valuetoprint, "%s%s", convert, SerialDegree);
			lcd.print(valuetoprint);
			break;

			//Print Ambiant Temperature from Sky sensor
			case 2:
			lcd.setCursor(0, 0);
			lcd.print("Temp Ambiente");
			lcd.setCursor(0, 1);
			dtostrf(myData.temp_ambient, 5, 2, convert);
			sprintf(valuetoprint, "%s%s", convert, SerialDegree);
			lcd.print(valuetoprint);
			break;

			//Print Sky Temperature
			case 3:
			lcd.setCursor(0, 0);
			lcd.print("Temp Ciel");
			lcd.setCursor(0, 1);
			dtostrf(myData.temp_ciel, 5, 2, convert);
			sprintf(valuetoprint, "%s%s", convert, SerialDegree);
			lcd.print(valuetoprint);
			break;

			//Print SQM Value from Light sensor
			case 4:
			lcd.setCursor(0, 0);
			lcd.print("SQM");
			lcd.setCursor(0, 1);
			dtostrf(Msqm, 5, 2, convert);
			sprintf(valuetoprint, "%s", convert);
			lcd.print(valuetoprint);
			break;

			//Default Menu
			default:
			increment = 0;
			lcd.setCursor(0, 0);
			lcd.print("Cliquez pour");
			lcd.setCursor(0, 1);
			infos = "plus d infos";
			infos.toCharArray(valuetoprint,20);
			lcd.print(valuetoprint);
			break;
		}
 	}


	//Send data out of RF433

	memcpy(tx_buf, &myData, sizeof(myData) );
	byte zize=sizeof(myData);

	driver.send((uint8_t *)tx_buf, zize);
	driver.waitPacketSent(200);
 
	myData.counter++;
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
// Fonction récupérant la température depuis le DS18B20
// Retourne true si tout va bien, ou false en cas d'erreur
//////////////////////////////////////////////////////
boolean getTemperature(float *temp){
	byte data[9], addr[8];
	// data : Données lues depuis le scratchpad
	// addr : adresse du module 1-Wire détecté

	if (!ds.search(addr)) { // Recherche un module 1-Wire
	    ds.reset_search();    // Réinitialise la recherche de module
	    return false;         // Retourne une erreur
	}
  
	if (OneWire::crc8(addr, 7) != addr[7]) // Vérifie que l'adresse a été correctement reçue
	    return false;                        // Si le message est corrompu on retourne une erreur

	if (addr[0] != DS18B20) // Vérifie qu'il s'agit bien d'un DS18B20
	    return false;         // Si ce n'est pas le cas on retourne une erreur

	ds.reset();             // On reset le bus 1-Wire
	ds.select(addr);        // On sélectionne le DS18B20

	ds.write(0x44, 1);      // On lance une prise de mesure de température
	delay(800);             // Et on attend la fin de la mesure
  
	ds.reset();             // On reset le bus 1-Wire
	ds.select(addr);        // On sélectionne le DS18B20
	ds.write(0xBE);         // On envoie une demande de lecture du scratchpad

	  for (byte i = 0; i < 9; i++) // On lit le scratchpad
	    data[i] = ds.read();       // Et on stock les octets reçus
  
  	// Calcul de la température en degré Celsius
	  *temp = ((data[1] << 8) | data[0]) * 0.0625; 
  
  	// Pas d'erreur
	  return true;
}


////////////////////END OF FUNCTIONS///////////////////////