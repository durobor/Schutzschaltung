

/*
  Cutterhead Protection 
  for Caruso Prot14 hardware arduino  nanoboard
  ports:


 
  LED red D2         rote Led auf Board
  Relais Out digi4   schaltet Kopf an Verstärker 
  Port1   D5
  Port2   D6
  Port3   D7  TASTER
  
  current measure in  A7    analoge Spannung 
  AC712-05 chip
  typ 185mV/A
  zero ampere out=0.5*Vcc= 0.5* 5V=2.5V
  bei 5A=0.925V + 0ffset 2.5= 3.425V   (in realität 2.4765
  -5A=-0.925 +offset =1.575


  test: 0.5A= wert  530. 0wert= 512=18= 0.087976539589443mV
  1 digit= 5V/1023
524=304mA=524-512=12 digits * 5/1023=317mA berechnet. 
  

  
 */
const int sensorPin = A7;    // select the input pin for the potentiometer
const int buttonPin = 7;     // the number of the pushbutton pin
const int ledPin =  2;        // the number of the LED pin
const int current_val =185 ;     // datasheet says 185mV/A
const int offset_val  =2500;    // datasheet saya 0.5*Vcc Vcc=5V=2.5V
 int sensorValue=0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin 13 as an output.
    Serial.begin(9600);
  
}

// the loop function runs over and over again forever
void loop() {
   sensorValue = analogRead(sensorPin);
 Serial.print("sensor = ");
  Serial.print(sensorValue);
   Serial.println("");

  delay(500);              // wait for a second
}


