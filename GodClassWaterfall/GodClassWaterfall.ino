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
const int ledPin =  2;          // the number of the LED pin
const int sensorCurrentScaling = 185;   // datasheet says 185mV/A
const int sensorOffsetVal = 2500;    // datasheet says 0.5*Vcc=2.5V, Vcc=5V
int zeroAmpereSensorVal;
float sensorValueToCurrentFactor;
volatile int sensorValues[500];
volatile int sensorValues10min[300];
int sensorValuesSize;
int sensorValuesSize10min;
volatile int sensorValuesIndx;
volatile int sensorValuesIndx10min;
const int readInterval = 4000;  // read sensor value every 4000 micro seconds (250 reads/s)
int nmbrOfReadsIn20ms = 5;
int nmbrOfReadsIn300ms = 75;
int nmbrOfReadsIn2000ms = 500;
int nmbrOfReadsIn10min = 300;
volatile float sumSensorValues20ms = 0;
volatile float sumSensorValues300ms = 0;
volatile float sumSensorValues2000ms = 0;
volatile float sumSensorValues10min = 0;
int currentCapacity = 500;      // 500mA
int currentThreshold20ms = 10;
int currentThreshold300ms = 4;
int currentThreshold2000ms = 2.75;
int currentThreshold10min = 2.1;

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
  
  sensorValuesIndx = ( sensorValuesIndx + 1 ) % sensorValuesSize;
  
  sumSensorValues2000ms -= sensorValues [ sensorValuesIndx ];
  sensorValues [ sensorValuesIndx ] = analogRead ( sensorPin ) - zeroAmpereSensorVal;
  sumSensorValues2000ms += sensorValues [ sensorValuesIndx ];
  
  sumSensorValues20ms += sensorValues [ sensorValuesIndx ];
  sumSensorValues20ms -= sensorValues [ ( sensorValuesSize + sensorValuesIndx - nmbrOfReadsIn20ms ) % sensorValuesSize ];

  sumSensorValues300ms += sensorValues [ sensorValuesIndx ];
  sumSensorValues300ms -= sensorValues [ ( sensorValuesSize + sensorValuesIndx - nmbrOfReadsIn300ms ) % sensorValuesSize ];
  
  interrupts();
}

void copyCurrentValue2ms () {
  noInterrupts();

  sensorValuesIndx10min = ( sensorValuesIndx10min + 1 ) % sensorValuesSize10min;
  sensorValues10min [ sensorValuesIndx10min ] = sumSensorValues2000ms;

  sumSensorValues10min += sensorValues10min [ sensorValuesIndx10min ];
  sumSensorValues10min -= sensorValues10min [ ( sensorValuesSize10min + sensorValuesIndx10min - nmbrOfReadsIn10min ) % sensorValuesSize10min ];

  interrupts();
}

void initializeZeroAmpereSensorVal() {
  zeroAmpereSensorVal = 512;
}

void initializeSensorValueToCurrentFactor() {
  sensorValueToCurrentFactor = 5000. / 1024. / sensorCurrentScaling;
}

float sensorValueToCurrent ( int sensorVal ) {
  return sensorVal * sensorValueToCurrentFactor;
}

void checkCurrent20ms() {
  if ( currentCapacity * currentThreshold20ms < sensorValueToCurrent ( sumSensorValues20ms / nmbrOfReadsIn20ms ) /* * ( 1000000 / readInterval / nmbrOfReadsIn20ms ) */ )
    Serial.println("Cutoff! - 20ms");
}

void checkCurrent300ms() {
  if ( currentCapacity * currentThreshold300ms < sensorValueToCurrent ( sumSensorValues300ms / nmbrOfReadsIn300ms ) )
    Serial.println("Cutoff! - 300ms");
}

void checkCurrent2000ms() {
  if ( currentCapacity * currentThreshold2000ms < sensorValueToCurrent ( sumSensorValues2000ms / nmbrOfReadsIn2000ms ) )
    Serial.println("Cutoff! - 2000ms");
}

void checkCurrent10min() {
  if (currentCapacity * currentThreshold10min < sensorValueToCurrent ( sumSensorValues10min / nmbrOfReadsIn10min ) )
    Serial.println("Cutoff! - 10min");
}

// the setup function runs once when you press reset or power the board
void setup() {
  noInterrupts();
  Serial.begin(9600);
  initializeSensorValueToCurrentFactor();
  initializeZeroAmpereSensorVal();
  initializeSensorValues();
  Timer1.initialize ( readInterval );
  Timer1.attachInterrupt ( sensorRead );
  Timer1.attachInterrupt ( copyCurrentValue2ms, 2000000 );
  interrupts();
}

// the loop function runs over and over again forever
void loop() {
  checkCurrent20ms();
  checkCurrent300ms();
  checkCurrent2000ms();
  checkCurrent10min();
}
