#include <TimerOne.h>


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
const int sensorPin = A7;       // select the input pin for the potentiometer
const int buttonPin = 7;        // the number of the pushbutton pinf
//const int ledRelaisPin =  2;    // the number of the LED pin
const int relaisPin = 666;      // don't know yet
const int ledRelaisPin = LED_BUILTIN;  // don't know either yet
const int sensorCurrentScaling = 185;   // datasheet says 185mV/A
const int sensorOffsetVal = 2500;    // datasheet says 0.5*Vcc=2.5V, Vcc=5V
//volatile int relaisON = LOW;
volatile int buttonState = LOW;
int zeroAmpereSensorVal;
float sensorValueToCurrentFactor;
volatile int sensorValues[500];
volatile int sensorValues10min[300];
int sensorValuesSize;
int sensorValuesSize10min;
volatile int sensorValuesIndx;
volatile int sensorValuesIndx10min;
const int readInterval = 4000;  // read sensor value every 4000 micro seconds (250 reads/s)
const int readInterval10min = 2000000;  // read sensor array every 2 seconds
int nmbrOfReadsIn20ms = 5;
int nmbrOfReadsIn300ms = 75;
int nmbrOfReadsIn2000ms = 500;
int nmbrOfReadsIn10min = 300;
volatile float sumSensorValues20ms = 0;
volatile float sumSensorValues300ms = 0;
volatile float sumSensorValues2000ms = 0;
volatile float sumSensorValues10min = 0;
int currentCapacity = 630;      // 630mA
int currentThreshold4ms = 20;
int currentThreshold20ms = 10;
int currentThreshold300ms = 4;
int currentThreshold2000ms = 2.75;
int currentThreshold10min = 2.1;

void initializeSensor() {
  relaisOFF();
  initializeSensorValueToCurrentFactor();
  initializeZeroAmpereSensorVal();
  initializeSensorValues();
  relaisON();
}

void initializeSensorValueToCurrentFactor() {
  sensorValueToCurrentFactor = 5000. / 1024. / sensorCurrentScaling;
}

void initializeZeroAmpereSensorVal() {
  zeroAmpereSensorVal = 512;
}

void initializeSensorValues() {
  sensorValuesSize = ( sizeof ( sensorValues ) / sizeof ( int ) );
  for ( sensorValuesIndx = 0 ; sensorValuesSize > sensorValuesIndx ; sensorValuesIndx++ ) {
    sensorValues [ sensorValuesIndx ] = 0;
  }
  sensorValuesIndx = 0;

  sensorValuesSize10min = ( sizeof ( sensorValues10min ) / sizeof ( int ) );
  for ( sensorValuesIndx10min = 0 ; sensorValuesSize10min > sensorValuesIndx10min ; sensorValuesIndx10min++ ) {
    sensorValues10min [ sensorValuesIndx10min ] = 0;
  }
  sensorValuesIndx10min = 0;
}

void sensorRead() {
  noInterrupts();
    
  sumSensorValues2000ms -= sensorValues [ sensorValuesIndx ];
  sensorValues [ sensorValuesIndx ] = abs ( analogRead ( sensorPin ) - zeroAmpereSensorVal );
  if ( currentCapacity * currentThreshold4ms < sensorValueToCurrent ( sensorValues [ sensorValuesIndx ] ) ) relaisOFF();
  sumSensorValues2000ms += sensorValues [ sensorValuesIndx ];
  
  sumSensorValues20ms -= sensorValues [ ( sensorValuesSize + sensorValuesIndx - nmbrOfReadsIn20ms ) % sensorValuesSize ];
  sumSensorValues20ms += sensorValues [ sensorValuesIndx ];

  sumSensorValues300ms -= sensorValues [ ( sensorValuesSize + sensorValuesIndx - nmbrOfReadsIn300ms ) % sensorValuesSize ];
  sumSensorValues300ms += sensorValues [ sensorValuesIndx ];
  
  sensorValuesIndx = ( sensorValuesIndx + 1 ) % sensorValuesSize;

  if ( sensorValuesSize-1 == sensorValuesIndx ) copyCurrentValue2ms();
  
  interrupts();
}

void copyCurrentValue2ms () {
  sensorValues10min [ sensorValuesIndx10min ] = sumSensorValues2000ms;

  sumSensorValues10min -= sensorValues10min [ ( sensorValuesSize10min + sensorValuesIndx10min - nmbrOfReadsIn10min ) % sensorValuesSize10min ];
  sumSensorValues10min += sensorValues10min [ sensorValuesIndx10min ];
}

void checkCurrent20ms() {
  if ( currentCapacity * currentThreshold20ms < sensorValueToCurrent ( sumSensorValues20ms / nmbrOfReadsIn20ms ) ) relaisOFF();
}

void checkCurrent300ms() {
  if ( currentCapacity * currentThreshold300ms < sensorValueToCurrent ( sumSensorValues300ms / nmbrOfReadsIn300ms ) ) relaisOFF();
}

void checkCurrent2000ms() {
  if ( currentCapacity * currentThreshold2000ms < sensorValueToCurrent ( sumSensorValues2000ms / nmbrOfReadsIn2000ms ) ) relaisOFF();
}

void checkCurrent10min() {
  if (currentCapacity * currentThreshold10min < sensorValueToCurrent ( sumSensorValues10min / nmbrOfReadsIn10min ) ) relaisOFF();
}

float sensorValueToCurrent ( int sensorVal ) {
  return sensorVal * sensorValueToCurrentFactor;
}

void relaisOFF() {
  digitalWrite ( relaisPin, LOW );
  digitalWrite ( ledRelaisPin, LOW );
  buttonState = LOW;
}

void relaisON() {
  digitalWrite ( relaisPin, HIGH );
  digitalWrite ( ledRelaisPin, HIGH );
  buttonState = HIGH;
}

// the setup function runs once when you press reset or power the board
void setup() {
  noInterrupts();
  
  Serial.begin(9600);

  pinMode ( sensorPin, INPUT );
  pinMode ( buttonPin, INPUT_PULLUP );
  pinMode ( relaisPin, OUTPUT );
  pinMode ( ledRelaisPin, OUTPUT );

  initializeSensor();

  Timer1.initialize ( readInterval );
  Timer1.attachInterrupt ( sensorRead );

  interrupts();
}

// the loop function runs over and over again forever
void loop() {
  if ( LOW == digitalRead ( buttonPin ) && LOW == buttonState ) {
    noInterrupts();
    initializeSensor();
    interrupts();
  }
  checkCurrent20ms();
  checkCurrent300ms();
  checkCurrent2000ms();
  checkCurrent10min();
}
