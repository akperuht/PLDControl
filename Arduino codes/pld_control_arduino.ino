/*
 * @author Aki Ruhtinas (c) 2019
 * Arduino code for controlling optics and gas supply lines of PLD chamber
 * Communicates with PC via serial ports
 */

// DAC control libraries
#include <Wire.h>
#include <Adafruit_MCP4725.h>

// Temperature sensor control libraries
#include <OneWire.h>
#include <DallasTemperature.h>

// Temperature sensor initialization
#define ONE_WIRE_BUS 2 
OneWire oneWire(ONE_WIRE_BUS);// Setup oneWire instance
DallasTemperature tempSensors(&oneWire);

// Stepper motor control library
#include <Stepper.h>

double chamberT=0; // Temperature of outer surface of the chamber

// SERIAL COMMUNICATION PARAMETERS
const int BAUDRATE=9600;
String msg="null";


// Gas flow setpoints and measured values
// Setpoint 12bit, measured value 10bit
double setPoint1=0;
double setPoint2=0;
double flowVal1=0;
double flowVal2=0;


// Setting up two 12bit DAC modules
Adafruit_MCP4725 dac1;
Adafruit_MCP4725 dac2;

// Stepper motor parameters
const int stepsPerRevolution=200;
const int pwmA=3;
const int pwmB=11;
const int brakeA=9;
const int brakeB=8;
const int dirA=12;
const int dirB=13;
Stepper stepmotor(stepsPerRevolution,dirA,dirB);
bool left=false;
bool right=false;
long stepdelay=2000000;
unsigned long t=0;
unsigned long dt=0;

const int MOTOR_SPEED=100;

int sweepRange=5;
int midPoint=0;
int motorPosition=0;
bool sweep=false;
int sign=1;


/* ==========================================================
 *                          SETUP
 * ==========================================================
 */
void setup() {
  // Setting up serial communication
  Serial.begin(BAUDRATE);
  // Setting up DAC modules for MFC control
  dac1.begin(0x62);
  dac2.begin(0x63);

  // Pins to read gas flow reading
  pinMode(A3,INPUT);
  pinMode(A2,INPUT);

  // Temperature sensor
  tempSensors.begin();

  // Setting up motor control pins
  pinMode(pwmA,OUTPUT);
  pinMode(pwmB,OUTPUT);
  pinMode(brakeA,OUTPUT);
  pinMode(brakeB,OUTPUT);
  digitalWrite(pwmA,HIGH);
  digitalWrite(pwmB,HIGH);
  digitalWrite(brakeA,LOW);
  digitalWrite(brakeB,LOW);
  
  // Setting motor speed
  stepmotor.setSpeed(MOTOR_SPEED);
}
/* ==========================================================
 *                    MAIN LOOP
 * ==========================================================
 */
void loop() {
  // Serial communication part  
  tempSensors.requestTemperatures();
  msg=Serial.readString(); // Message from PC
  handleMessage(msg);

  // Stepper motor control
  dt=micros()-t;
  if(stepdelay>dt){delayMicroseconds(abs(stepdelay-dt));}
  if(left){stepmotor.step(1);motorPosition=motorPosition+1;}
  if(right){stepmotor.step(-1);motorPosition=motorPosition-1;}

  // sweeping
  if(sweep){
    if(abs(motorPosition-midPoint)<sweepRange){
      stepmotor.step(sign);
      motorPosition=motorPosition+sign;
    }
    else{
      sign=sign*(-1);
      stepmotor.step(sign);
      motorPosition=motorPosition+sign;
    }
  }
  t=micros();

  // Measurements
  readFlow();
  chamberT=tempSensors.getTempCByIndex(0);

  // Sending data to computer
  String dataToPC="DATA["+String(flowVal1)+" "+String(flowVal2);
  dataToPC=dataToPC+" "+String(chamberT)+" "+String(motorPosition)+"]";
  Serial.println(dataToPC);
}

/* ==========================================================
 *                   MESSAGE HANDLING
 * ==========================================================
 */

/*
 * Handles messages received from PC
 */
void handleMessage(String msg){
  // Form of gas flow command: GAS[setPoint1 setPoint2]
  if(msg.startsWith("GAS",0) > 0){setGasParam(midString(msg,"[","]"));};

  // Motor control commands
  if(msg.startsWith("GOTO",0) > 0){right=false;left=false;sweep=false;motorGoTo(midString(msg,"[","]"));Serial.println("Motor moved");};
  if(msg.startsWith("SWEEP",0) > 0){right=false;left=false;setSweepParam(midString(msg,"[","]"));};
  if(msg.startsWith("LEFT")){right=false;left=true;sweep=false;Serial.println("Motor stepping to left");};
  if(msg.startsWith("RIGHT")){right=true;left=false;sweep=false;Serial.println("Motor stepping to right");};
  if(msg.startsWith("STOP")){right=false;left=false;sweep=false;Serial.println("Motor stopped");};
}


/* ==========================================================
 *              GAS FLOW CONTROL & MEASUREMENT
 * ==========================================================
 */
 
/*
 * Setting up the gas flow parameters
 */
void setGasParam(String param){
  param.replace(",", "."); // Replacing commas from PLDControl software
  int N=3; // Size of param array
  float values[N]; // param array that 
  extractValues(values,param,N);
  setPoint1=values[0];
  setPoint2=values[1];
  setFlow(setPoint1,setPoint2);
}

/* 
*  Sets MFC remote setpoints to desired gas flows
*  via DAC module. Remote setpoint is voltage 0-5V, 
*  set by 12 bit Adafruit DAC module. Thus setpoint needs to be scaled
*  to integer between 0 and 4096.
*/
void setFlow(double setPoint1, double setPoint2){
  
  bool valueToEEPROM=false; // determines if setpoint is stored in EEPROM memory
  // If used, EEPROM could wear out due limited number of writes
  
  // 12bit integer value of setpoint
  int V1=(setPoint1/10.0)*4096;
  int V2=(setPoint2/10.0)*4096;
  
  // 12bit integer value of setpoint
  dac1.setVoltage(V1, valueToEEPROM);
  dac2.setVoltage(V2, valueToEEPROM);
  Serial.print("GAS FLOW SET: ");
  Serial.print(V1);
  Serial.print(" ");
  Serial.println(V2);
}

/*
 * Reads measured gas flow value
 */
void readFlow(){
  // N2 flow value
  flowVal1=(analogRead(A3)/1023.0)*10; // Scaling value to 10 sccm

  // Argon flow value
  flowVal2=(analogRead(A2)/1023.0)*10;
  // Using K value to convert to real flow
  flowVal2*1.4573;
}

/* ==========================================================
 *                  MOTOR CONTROL
 * ==========================================================
 */

/*
 * Setting up the sweep
 */
void setSweepParam(String param){
  int N=3; // Size of param array
  float values[N]; // param array that 
  extractValues(values,param,N);
  sweepRange=values[0];
  midPoint=values[1];
  motorGoTo(String(midPoint));
  sweep=true;
  Serial.println("Motor started sweeping");
}

/*
 * Driving stepper motor to a certain point
 */
void motorGoTo(String point){
  stepmotor.step(point.toFloat()-motorPosition);
  motorPosition=point.toFloat();
}

/* ==========================================================
 *                   HELPERS
 * ==========================================================
 */

 /*
  * Gets substring between two strings
  */
String midString(String str, String start, String finish){
  int locStart = str.indexOf(start);
  if (locStart==-1) return "ERROR";
  locStart += start.length();
  int locFinish = str.indexOf(finish, locStart);
  if (locFinish==-1) return "ERROR";
  return str.substring(locStart, locFinish);
}
 /*
  *  Gets values from string separated space
  *  
  *  Reference of the values float array is given to
  *  this function, so values of the array are changed
  */
void extractValues(float *values, String str, int N){
  String separator=" "; // Separator of values in str
  int i=0; 
  // Initialization of values array
  while(i<N){
    values[i]=0;
    i++;
  }
  // Extract floating point numbers from str to values
  // Takes N first numbers 
  i=0;
  int first=-1;
  while(str.indexOf(separator)>-1&&i<N){
    first = str.indexOf(separator); // index of first separator in string
    values[i]=str.substring(0,first).toFloat(); // Conversion to float
    str=str.substring(first+1); //remove value from string
    i++;
  }
  values[i]=str.toFloat(); // Takes last number when there is less than N elements
}
