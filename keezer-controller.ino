// This #include statement was automatically added by the Particle IDE.
#include <HX711ADC.h>

//User-inputs to calibrate the keg weight sensors and select temperature units
double Keg1_Empty_Calibration=503991; //Calibrated on 3/4/19. Calibrated on 8/23/18: 496451.
double Keg1_Full_Calibration=887308; //Calibrated on 8/23/18.
double Keg1_Slope=((Keg1_Full_Calibration-Keg1_Empty_Calibration)/100);

double Keg2_Empty_Calibration=-492628; //Calibrated on 6/2/19
double Keg2_Full_Calibration=-106370; //Calibrated on 6/2/19
double Keg2_Slope=((Keg2_Full_Calibration-Keg2_Empty_Calibration)/100);

double Keg3_Empty_Calibration=-198184; //Calibrated on 8/23/18. 8/13/18:-194358
double Keg3_Full_Calibration=204347; //Calibrated on 8/13/18
double Keg3_Slope=((Keg3_Full_Calibration-Keg3_Empty_Calibration)/100);

//Had to replace most of the strain gauges due to corrosion. Values calibrated using different keg outside of keezer, so need to be updated
double Keg4_Empty_Calibration=343800; //Calibrated on
double Keg4_Full_Calibration=744000; //Calibrated on
double Keg4_Slope=((Keg4_Full_Calibration-Keg4_Empty_Calibration)/100);

double Keg5_Empty_Calibration=-431136; //Calibrated on 9/20/18
double Keg5_Full_Calibration=-572007; //Calibrated on 9/20/18
double Keg5_Slope=((Keg5_Full_Calibration-Keg5_Empty_Calibration)/100);

//10lbs Delta = 82693
//Steel, Full = 7437543, calibrated on 8/7/19; Empty = 7354850
//Aluminum, Full = ; Empty = 
double Gas_Empty_Calibration=7354850; 
double Gas_Full_Calibration=7437543;
double Gas_Slope=((Gas_Full_Calibration-Gas_Empty_Calibration)/100);

float TempC; //Sensor is calibrated for Celsius
float TempF;
int Temp;

#define Keg1_SCK A6
#define Keg1_DT A7
#define Keg2_SCK A4
#define Keg2_DT A5
#define Keg3_SCK A2
#define Keg3_DT A3
#define Keg4_SCK D0
#define Keg4_DT D1
#define Keg5_SCK D2
#define Keg5_DT D3
#define Gas_SCK D4
#define Gas_DT D5
#define Temp_Pin A0 
#define PSI_Pin A1
#define Valves D6
#define Lights D7

int Keg1;
int Keg2;
int Keg3;
int Keg4;
int Keg5;
int Gas;
int PSI;

double Keg1_Current_Reading;
double Keg2_Current_Reading;
double Keg3_Current_Reading;
double Keg4_Current_Reading;
double Keg5_Current_Reading;
double Gas_Current_Reading;
int Temp_Current_Reading;
int PSI_Current_Reading;

const int TempnumReadings = 60;     //The number of Temp readings that will be averaged (used to reduce noise)
int Tempreadings[TempnumReadings];  //The array of readings from the analog Temp input
int TempreadIndex = 0;              //The index of the current Temp reading
int Temptotal = 0;                  //The running total of Temp readings
int Tempaverage = 0;                //The average of the Temp readings
int Tempcounter = 0;                //Counter to average readings before full sample

String Status = "Off";

//Timer to only average the temp values every second (in milliseconds)
Timer timer1(1000, Temp_Average);

//Timer to publish values every minute (in milliseconds)
Timer timer2(60000, Publish_Values);
bool Publish_Values_Flag = false;

HX711ADC scale1(Keg1_DT,Keg1_SCK);
HX711ADC scale2(Keg2_DT,Keg2_SCK);
HX711ADC scale3(Keg3_DT,Keg3_SCK);
HX711ADC scale4(Keg4_DT,Keg4_SCK);
HX711ADC scale5(Keg5_DT,Keg5_SCK);
HX711ADC scale6(Gas_DT,Gas_SCK);

void setup() {
    
timer1.start(); 
timer2.start(); 

pinMode(Temp_Pin, INPUT);
pinMode(PSI_Pin, INPUT);
pinMode(Valves, OUTPUT);
pinMode(Lights, OUTPUT);

digitalWrite(Valves, LOW);
digitalWrite(Lights, LOW);

Particle.subscribe("Keezer_On", Keezer_On, MY_DEVICES);
Particle.subscribe("Keezer_Off", Keezer_Off, MY_DEVICES);

Particle.variable("Status", Status);
Particle.variable("Keg1", Keg1);
Particle.variable("Keg2", Keg2);
Particle.variable("Keg3", Keg3);
Particle.variable("Keg4", Keg4);
Particle.variable("Keg5", Keg5);
Particle.variable("Gas", Gas);
Particle.variable("Temp", Temp);
Particle.variable("PSI", PSI);

Time.zone(-5);

// Initialize all the Temp readings to 0. This is used for average values.
for (int TempthisReading = 0; TempthisReading < TempnumReadings; TempthisReading++) {
    Tempreadings[TempthisReading] = 0;
}

scale1.set_scale();
scale2.set_scale();
scale3.set_scale();
scale4.set_scale();
scale5.set_scale();
scale6.set_scale();

scale1.power_up();
scale2.power_up();
scale3.power_up();
scale4.power_up();
scale5.power_up();
scale6.power_up();
}

void loop() {

//Read current values
Keg1_Current_Reading = scale1.get_units();
Keg2_Current_Reading = scale2.get_units();
Keg3_Current_Reading = scale3.get_units();
Keg4_Current_Reading = scale4.get_units();
Keg5_Current_Reading = scale5.get_units();
Gas_Current_Reading = scale6.get_units();
Temp_Current_Reading = analogRead(Temp_Pin);
PSI_Current_Reading = analogRead(PSI_Pin);

//Calculate PSI
//Equation based on Honeywell TruStability® Board Mount Pressure Sensor Model#: SSC D AN N 060PG A A 5
//Sensor is a 5V sensor being supplied with 5V but Photon can only read max of 3.3 V. Reading will max out at 40PSI, which is regulator max.
PSI=((0.01209*(float)PSI_Current_Reading)-7.5);

//Convert Keg Readings into % Full values
Keg1=((1/Keg1_Slope)*Keg1_Current_Reading)-(Keg1_Empty_Calibration/Keg1_Slope);
Keg2=((1/Keg2_Slope)*Keg2_Current_Reading)-(Keg2_Empty_Calibration/Keg2_Slope);
Keg3=((1/Keg3_Slope)*Keg3_Current_Reading)-(Keg3_Empty_Calibration/Keg3_Slope);
Keg4=((1/Keg4_Slope)*Keg4_Current_Reading)-(Keg4_Empty_Calibration/Keg4_Slope);
Keg5=((1/Keg5_Slope)*Keg5_Current_Reading)-(Keg5_Empty_Calibration/Keg5_Slope);
Gas=((1/Gas_Slope)*Gas_Current_Reading)-(Gas_Empty_Calibration/Gas_Slope);

//If test to see if Publish_Values timer has triggered
if (Publish_Values_Flag==true) {
//Limited to 4 publishes per second. If more are needed, create a second timer offset by a few seconds

//Publish Temp values every minute
//String Temp_Publish = String(Temp);
//Particle.publish("TempF", Temp_Publish, PRIVATE);

//Publish Keg1 values every minute, used for calibration purposes
//String Keg1_Publish = String(Keg1_Current_Reading);
//Particle.publish("Keg1", Keg1_Publish, PRIVATE);

//Publish Keg2 values every minute, used for calibration purposes
//String Keg2_Publish = String(Keg2_Current_Reading);
//Particle.publish("Keg2", Keg2_Publish, PRIVATE);

//Publish Keg3 values every minute, used for calibration purposes
//String Keg3_Publish = String(Keg3_Current_Reading);
//Particle.publish("Keg3", Keg3_Publish, PRIVATE);

//Publish Keg4 values every minute, used for calibration purposes
//String Keg4_Publish = String(Keg4_Current_Reading);
//Particle.publish("Keg4", Keg4_Publish, PRIVATE);

//Publish Keg5 values every minute, used for calibration purposes
//String Keg5_Publish = String(Keg5_Current_Reading);
//Particle.publish("Keg5", Keg5_Publish, PRIVATE);

//Publish Gas values every minute, used for calibration purposes
//String Gas_Publish = String(Gas_Current_Reading);
//Particle.publish("Gas", Gas_Publish, PRIVATE);

Publish_Values_Flag=false;
}
//If statement to cycle the valves every morning to help keep them from sticking
if (Time.hour()==3 and Time.minute()==0) {
    Status = "Valves Cycling";
    digitalWrite(Valves,HIGH);
    delay(15000);
    
    digitalWrite(Valves,LOW);
    delay(15000);
    
    digitalWrite(Valves,HIGH);
    delay(15000);  
    
    digitalWrite(Valves,LOW);
    digitalWrite(Lights,LOW);
    delay(20000);
    Status = "Off";
}
}

void Temp_Average()
{
//Average the Temp readings to help reduce noise
//Subtract the last Temp reading:
Temptotal = Temptotal - Tempreadings[TempreadIndex];
//Read from the Temp sensor:
Tempreadings[TempreadIndex] = Temp_Current_Reading;
//Add the Temp reading to the total:
Temptotal = Temptotal + Tempreadings[TempreadIndex];
//Advance to the next position in the array:
TempreadIndex = TempreadIndex + 1;
//If we're at the end of the array...
if (TempreadIndex >= TempnumReadings) {
//...wrap around to the beginning:
TempreadIndex = 0;
}

if (Tempcounter < TempnumReadings) {
    Tempcounter = Tempcounter + 1;
    Tempaverage = Temptotal / Tempcounter;
}
    else {
    //Calculate the average Temp reading:
    Tempaverage = Temptotal / TempnumReadings;
}
//Calculate temperature using the average read value
TempC=((100*((float)Tempaverage/4095)*3.3)-50); //Pin1=Power, Pin2=Vout, Pin3=Ground
TempF=((TempC*9/5)+32-7); //Added -7 as calibration factor; compared readings to 2 seperate thermometers to verify this value
Temp=TempF; //Select TempF or TempC
}

void Publish_Values()
{
Publish_Values_Flag = true;
}

void Keezer_On(const char *event, const char *data)
{
    digitalWrite(Valves,HIGH);
    digitalWrite(Lights,HIGH);
    Status = "Open for Business";
}

void Keezer_Off(const char *event, const char *data)
{
    digitalWrite(Valves,LOW);
    digitalWrite(Lights,LOW);
    Status = "Off";
}