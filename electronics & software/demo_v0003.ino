/*  June 30th 2018
    Simple bang-bang controller
    Extended to measure ersistance by voltage drop
*/

//Definitions
#define THERMISTORPIN A0        //Analogue pin thermistor is on
#define RESISTANCEPIN A1
#define NUMSAMPLES 5            //Number of samples to read and average, more is slower but smoother
#define THERMISTORNOMINAL 10000 //Resistance at 25c, Thermistor NJ28NA0103F, rs part number 697-4576
#define TEMPERATURENOMINAL 25   //Tempeature for nominal resistance (should pretty much always be 25c)
#define BCOEFFICIENT 4100       //Beta coefficient for Steinhart-hart equation
#define SERIESRESISTOR 9860     //Value of current-limiting resistor, measure with DMM
const float SENSERESISTOR = 1.6; //Sense resistor, as measured. Nitinol will be somewhere between 1.5 and 3 ohms at room temperature

int samples[NUMSAMPLES];        //Initialise array for averaging
int samplesvIn[NUMSAMPLES];
int samplesvOut[NUMSAMPLES];
int holdtemp;               //Timer to hold temperature for 9 seconds

bool driverON = false;                  //Flag for driver circuit being ON or OFF
bool max_achieved = false;
bool min_achieved = false;

#define NIT_PIN_1 8              //Nitinol driver pin
#define NIT_PIN_2 9
#define NIT_PIN_3 10
#define SENSEPIN_1 11             //Nitinol sensor pins
#define SENSEPIN_2 12
#define SENSEPIN_3 13

#define MAX_TEMP 50
#define MIN_TEMP 40

float bend_sensor_rest;
float bend_sensor_max;

void setup() {  
  Serial.begin(9600);
  analogReference(EXTERNAL);    //Cconnect AREF to 3.3V and use that as VCC, less noisy!

  pinMode(NIT_PIN_1, OUTPUT);
  pinMode(NIT_PIN_2, OUTPUT);
  pinMode(NIT_PIN_3, OUTPUT);
  digitalWrite(NIT_PIN_1, LOW);
  digitalWrite(NIT_PIN_2, LOW);
  digitalWrite(NIT_PIN_3, LOW);
  pinMode(SENSEPIN_1, OUTPUT);
  pinMode(SENSEPIN_2, OUTPUT);
  pinMode(SENSEPIN_3, OUTPUT);
  digitalWrite(SENSEPIN_1, LOW);
  digitalWrite(SENSEPIN_2, LOW);
  digitalWrite(SENSEPIN_3, LOW);

  
  bend_sensor_rest = analogRead(A0);  //throw-away reading to stabilise ADC
  delay(1);   
  bend_sensor_rest = analogRead(A0);

  bend_sensor_max = bend_sensor_rest*1.1;    //Will move to +10% of resting position

}

/*-------------------------------------------------------------------------------------------------------------------------------
 Main loop
-------------------------------------------------------------------------------------------------------------------------------*/

void loop() {
  //float temperature;
  float bend_value;
  //temperature = read_temperature();
  //calcResist();
  bend_value = read_bend();
  apply_current(bend_value);
  Serial.println(); //To seperate each trace
  delay(10);
}

/*-------------------------------------------------------------------------------------------------------------------------------
 Read temperature. Read n ADC values, average to reduce noise, convert to resistance, temperature via Steinhart equation
 https://learn.adafruit.com/thermistor/using-a-thermistor
-------------------------------------------------------------------------------------------------------------------------------*/
float read_temperature() {
  uint8_t i;
  float average;

  // take N samples in a row, with a slight delay
  for (i = 0; i < NUMSAMPLES; i++) {
    samples[i] = analogRead(THERMISTORPIN);
    delay(5);
  }

  // average all the samples out
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samples[i];
  }
  average /= NUMSAMPLES;

  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;

  //Convert to celsius
  float steinhart;
  steinhart = average / THERMISTORNOMINAL;          //(R/R0)
  steinhart = log(steinhart);                       //ln(R/R0)
  steinhart /= BCOEFFICIENT;                        //1/B
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); //+1/T0)
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;                              //convert to C

  Serial.print("Thermistor = "); Serial.print(steinhart); Serial.print("          "); //whitespace
  return steinhart; //return steinhart as temperature
}

/*-------------------------------------------------------------------------------------------------------------------------------
  http://robotics.hobbizine.com/flexinolresist.html
  calcResist takes ADC readings at A0 and A1, converts them to voltages and uses the results to
  calculate the resistance of the Flexinol - the constant value of 0.004883 used here is equal to
  the reference voltage of 5 divided by the ADC resolution of 1024
  CHANGES: Using A1 and A2 (A0 for thermistor), using 3.3Vref, so const is 3.3/1024 = 0.003222656
  Float has 7 points of precision, so 0.003223
  Only read single analogue value, six pins can then measure 3 nitinol resistance values and 3 strain gauges
-------------------------------------------------------------------------------------------------------------------------------*/

float calcResist(){
  //Really needs some noise filtering
  //Needs to be called on interrupt routine maybe every 500ms - 1000ms to avoid heating nitinol
  uint8_t i;
  float average;
  float vIn;
  float vOut;
  float rFlexinol_1;
  float rFlexinol_2;
  float rFlexinol_3;
  
  digitalWrite(SENSEPIN_1, HIGH); //Turn mosfet on so we have a voltage to read
  digitalWrite(SENSEPIN_2, HIGH);
  digitalWrite(SENSEPIN_3, HIGH);
  
  if (driverON == false){        //turning driver circuit on drops current to sensing device, V=IR, sensitivity increased
    digitalWrite(NIT_PIN_1, HIGH);
    digitalWrite(NIT_PIN_2, HIGH);
    digitalWrite(NIT_PIN_3, HIGH);
  }
  /*
   * //Filtered analogue reading
 
  
  for(i=0; i < NUMSAMPLES; i++){
    vIn = analogRead (A1);   //take a throw-away reading to reset the ADC after switching pins
    delay (10);
    samplesvIn[i] = analogRead (A1);

    vOut = analogRead (A2);   //take a throw-away reading to reset the ADC after switching pins
    delay (10);
    samplesvOut[i] = analogRead (A2);
  }
  */

  //Unfiltered analogue reading
  rFlexinol_1 = analogRead (A3);   //take a throw-away reading to reset the ADC after switching pins
  delay (10);
  rFlexinol_1 = analogRead (A3);

  rFlexinol_2 = analogRead (A4);
  delay (10);
  rFlexinol_2 = analogRead (A4);

  rFlexinol_3 = analogRead (A5);  
  delay (10);
  rFlexinol_3 = analogRead (A5);
  
  digitalWrite(SENSEPIN_1, LOW);  //Disable ASAP so as not to heat nitinol
  digitalWrite(SENSEPIN_2, LOW);
  digitalWrite(SENSEPIN_3, LOW);
  if (driverON == true){        //Turn driver back on if it was on
    digitalWrite(NIT_PIN_1, HIGH);
    digitalWrite(NIT_PIN_2, HIGH);
    digitalWrite(NIT_PIN_3, HIGH);
  }

  /*
  // average all the vIn samples
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samplesvIn[i];
  }
  average /= NUMSAMPLES;
  vIn = average;

  // average all the vOut samples
  average = 0;
  for (i = 0; i < NUMSAMPLES; i++) {
    average += samplesvOut[i];
  }
  average /= NUMSAMPLES;
  vOut = average;
  */
  
  /*
  //Convert to voltage
  vIn = vIn * 0.003223;
  vOut = vOut * 0.003223;
  
  //rFlexinol = (SENSERESISTOR * vOut) / (vIn - vOut);
  rFlexinol = (vIn / vOut) - 1;
  rFlexinol = rFlexinol * SENSERESISTOR;
  Serial.print("Resistance = "); Serial.println(rFlexinol);
  return rFlexinol;
  */

  Serial.print(rFlexinol_1); Serial.print(" "); Serial.print(rFlexinol_2); Serial.print(" "); Serial.print(rFlexinol_3);  Serial.print(" ");
}

/*-------------------------------------------------------------------------------------------------------------------------------
Read bend sensors
Use raw ADC values, actual resistance has no bearing on position, just wastes machine cycles

-------------------------------------------------------------------------------------------------------------------------------*/
float read_bend(){
  //Read bend sensor resistance
  float bend_sensor_1;
  float bend_sensor_2;
  float bend_sensor_3;
  
  bend_sensor_1 = analogRead(A0);  //throw-away reading to stabilise ADC
  delay(1);   
  bend_sensor_1 = analogRead(A0);

  bend_sensor_2 = analogRead(A1);  //throw-away reading to stabilise ADC
  delay(1);   
  bend_sensor_2 = analogRead(A1);
  
  bend_sensor_3 = analogRead(A2);  //throw-away reading to stabilise ADC
  delay(1);   
  bend_sensor_3 = analogRead(A2);
  
  Serial.print(bend_sensor_1); Serial.print(" ");  Serial.print(bend_sensor_2); Serial.print(" "); Serial.print(bend_sensor_3); Serial.print(" ");
  return bend_sensor_3;
}

/*-------------------------------------------------------------------------------------------------------------------------------
 Bang-bang controller, incorporate Teh's work later
 At very high ferquencies, especially with PID, rise and fall times may become crucial. At the moment, MOSFET will turn and
 off faster than arduino
 May be good idea to implement flag variable, so calcResist function knows if MOSFET needs to be turned on
 
 Need to actuate nitinol 2 and 3 (as wired to stripboard)
-------------------------------------------------------------------------------------------------------------------------------*/
void apply_current(float bend_value) {  

  if ((bend_value<bend_sensor_max) && (!max_achieved)){
    digitalWrite(NIT_PIN_2, HIGH);
    digitalWrite(NIT_PIN_3, HIGH);
    driverON = true;
    Serial.println("Power on");
  }

  if (bend_value>=bend_sensor_max){
    max_achieved = true;
    min_achieved = false;
  } 
  
  if ((bend_value>bend_sensor_rest) && (max_achieved)){
    digitalWrite(NIT_PIN_2, LOW);
    digitalWrite(NIT_PIN_3, LOW);
    driverON = false;
    Serial.println("Power off");
  }

  if (bend_value<bend_sensor_rest){
    min_achieved = true;
    max_achieved = false;
  } 
}
