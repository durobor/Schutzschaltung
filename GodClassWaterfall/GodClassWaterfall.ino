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
const int sensorPin = A7;       
const int potentiometerPin = A1;        // currently not in use
const int buttonPin = 7;
const int relaisPin = 4;
const int ledRelaisPin = 6;
const int ledTwo = 5;                   // currently not in use  // could be used as PWM or LED
const int ncPin = 3;                    // currently not in use
const int ledMaxCurrentReached = 2;

bool relaisState = false;
const int debounce = 100;
const int sensorCurrentScaling = 185;   // datasheet says 185mV/A
const int sensorOffsetVal = 2500;    // datasheet says 0.5*Vcc=2.5V, Vcc=5V
volatile int buttonState = LOW;
volatile float potentiometerValue = 1.0;
int zeroAmpereSensorVal = 0;
float sensorValueToCurrentFactor;
volatile int sensorValues[500];
volatile int readIn;
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
volatile long sumSensorValues20ms = 0;
volatile long sumSensorValues300ms = 0;
volatile long sumSensorValues2000ms = 0;
volatile long sumSensorValues10min = 0;
int currentCapacity = 400;      // 630mA
int currentThreshold4ms = 8;   // used to be 20
int currentThreshold20ms = 6;  // used to be 10
int currentThreshold300ms = 3; // 4
int currentThreshold2000ms = 2; // 2.75
int currentThreshold10min = 1.5; //2.1

void initializeSensor() {
  relaisOFF();
  initializeSensorValueToCurrentFactor();
  initializeSensorValuesToZero();
  initializeZeroAmpereSensorVal();
}

void initializeSensorValueToCurrentFactor() {
  sensorValueToCurrentFactor = 5000. / 1024. / sensorCurrentScaling;
}

void initializeZeroAmpereSensorVal() {
  interrupts();
  
  delay ( ( readInterval * nmbrOfReadsIn2000ms / 1000 ) + 10 );
  zeroAmpereSensorVal = sumSensorValues2000ms / nmbrOfReadsIn2000ms;
  
  noInterrupts();
}

void initializeSensorValuesToZero() {
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

  zeroAmpereSensorVal = 0;
  sumSensorValues20ms = 0;
  sumSensorValues300ms = 0;
  sumSensorValues2000ms = 0;
  sumSensorValues10min = 0;
}

void sensorRead() {
  noInterrupts();
    
  sumSensorValues2000ms -= sensorValues [ sensorValuesIndx ];
  readIn = analogRead ( sensorPin );
  readIn = abs ( readIn );
  readIn -= zeroAmpereSensorVal;
  readIn = abs ( readIn );
  //sensorValues [ sensorValuesIndx ] = abs ( analogRead ( sensorPin )  - zeroAmpereSensorVal );
  sensorValues [ sensorValuesIndx ] = readIn;
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
  sumSensorValues10min -= sensorValues10min [ ( sensorValuesSize10min + sensorValuesIndx10min - nmbrOfReadsIn10min ) % sensorValuesSize10min ];
  sensorValues10min [ sensorValuesIndx10min ] = sumSensorValues2000ms;
  sumSensorValues10min += sensorValues10min [ sensorValuesIndx10min ];
 
  sensorValuesIndx10min = ( sensorValuesIndx10min + 1 ) % sensorValuesSize10min;
}

void readPotentiometer() {
  potentiometerValue = analogRead ( potentiometerPin ) / 1023.;
}

void checkCurrent20ms() {
  if ( potentiometerValue * currentCapacity * currentThreshold20ms < sensorValueToCurrent ( sumSensorValues20ms / nmbrOfReadsIn20ms ) )
    tresholdReached();
}

void checkCurrent300ms() {
  if ( potentiometerValue * currentCapacity * currentThreshold300ms < sensorValueToCurrent ( sumSensorValues300ms / nmbrOfReadsIn300ms ) )
    tresholdReached();
}

void checkCurrent2000ms() {
  if ( potentiometerValue * currentCapacity * currentThreshold2000ms < sensorValueToCurrent ( sumSensorValues2000ms / nmbrOfReadsIn2000ms ) )
    tresholdReached();
}

void checkCurrent10min() {
  if ( potentiometerValue * currentCapacity * currentThreshold10min < sensorValueToCurrent ( sumSensorValues10min / nmbrOfReadsIn10min ) )
    tresholdReached();
}

void tresholdReached() {
  relaisOFF();
  digitalWrite ( ledMaxCurrentReached, HIGH );
  delay ( 5000 );
}

float sensorValueToCurrent ( int sensorVal ) {
  return sensorVal * sensorValueToCurrentFactor * 1000;
}

void relaisOFF() {
  digitalWrite ( relaisPin, LOW );
  digitalWrite ( ledRelaisPin, LOW );
  buttonState = LOW;
  relaisState = false;
}

void relaisON() {
  digitalWrite ( relaisPin, HIGH );
  digitalWrite ( ledRelaisPin, HIGH );
  digitalWrite ( ledMaxCurrentReached, LOW );
  buttonState = HIGH;
  relaisState = true;
}

// the setup function runs once when you press reset or power the board
void setup() {
  noInterrupts();
  
  Serial.begin(9600);

  pinMode ( sensorPin, INPUT );
  pinMode ( buttonPin, INPUT_PULLUP );
  pinMode ( relaisPin, OUTPUT );
  pinMode ( ledRelaisPin, OUTPUT );
  pinMode ( ledMaxCurrentReached, OUTPUT );
//  pinMode ( potentiometerPin, INPUT );        // currently not in use
//  pinMode ( ledTwo, OUTPUT );                 // currently not in use
//  pinMode ( ncPin, OUTPUT );                  // currently not in use

  Timer1.initialize ( readInterval );
  Timer1.attachInterrupt ( sensorRead );

  initializeSensor();

  interrupts();
}

// the loop function runs over and over again forever
void loop() {
  if ( LOW == digitalRead ( buttonPin ) /*&& LOW == buttonState*/ ) {
    delay ( debounce );
    if ( LOW == digitalRead ( buttonPin ) ) {
      relaisState = !relaisState;
      delay ( 300 ); 
    }
  }
  if ( relaisState == true ) relaisON();
  else relaisOFF();

//  readPotentiometer();                      // currently not in use
  checkCurrent20ms();
  checkCurrent300ms();
  checkCurrent2000ms();
  checkCurrent10min();
  Serial.println ( sensorValues [ sensorValuesIndx ], DEC );
}
