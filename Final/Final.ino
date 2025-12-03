//Alec Zheng, Isaiah Marzan

#include <Wire.h>
//LiquidCrystal by Arduino, Adafruit
#include <LiquidCrystal.h>
//Stepper by Arduino
#include <Stepper.h>
//RTClib bu Adafruit
#include <RTClib.h>
//DHT sensor library by Adafruit
#include "DHT.h"

#define RDA 0x80
#define TBE 0x20  

//TEMP SETUP  
DHT dht(53, DHT11);
float temp;
float hum;

//RTC SETUP
RTC_DS1307 rtc;
bool timedisplayed = true;
unsigned long readtime;

//BUTTON STUFF
unsigned char* pin_l = (unsigned char*) 0x109;
unsigned char* ddr_l = (unsigned char*) 0x10A;
unsigned char* port_l = (unsigned char*) 0x10B;
unsigned char* ddr_e = (unsigned char*) 0x2D;
unsigned char* port_e = (unsigned char*) 0x2E;
unsigned char* ddr_h = (unsigned char*) 0x101;
unsigned char* port_h = (unsigned char*) 0x102;
volatile unsigned char* pin_b = (unsigned char*) 0x23;
volatile int state = 0;
//state 0 = disabled, state 1 = idle, state 2 = error, state 3 = running

//UART STUFF
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

//ADC STUFF
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;
int level = 0;

//MOTOR STUFF
unsigned char* pin_c = (unsigned char*) 0x26;
unsigned char* ddr_c = (unsigned char*) 0x27;
unsigned char* port_c = (unsigned char*) 0x28;


//DELAY STUFF
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;
volatile unsigned char *portDDRB = (unsigned char *) 0x24;
volatile unsigned char *portB =    (unsigned char *) 0x25;
int freq = 500;

//STEPPER STUFF
const int stepsPerRevolution = 2038;
Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);  //TEMP PINS CHANGE LATER

//DISPLAY CHARACTERS
const int RS = 11, EN = 12, D4 = 52, D5 = 48, D6 = 50, D7 = 46;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);


void setup() {
  U0Init(9600);
  adc_init();
  //LCD STUFF
  lcd.begin(16, 2); // set up number of columns and rows
  //BUTTON STUFF
  *ddr_e &= 0xEF;   //set PB4 to INPUT
  *port_e |= 0x10;  //INPUT pull up
  attachInterrupt(digitalPinToInterrupt(2), startButton, FALLING);
  //LED STUFF
  *ddr_h |= 0x08;   //blue
  *ddr_h |= 0x40;   //Green
  *ddr_h |= 0x20;   //yellow
  *ddr_h |= 0x10;   //Red
  *port_h &= 0xF7;  //blue off
  *port_h &= 0xBF;  //red off
  *port_h &= 0xDF;  //yellow off
  *port_h &= 0xEF;  //green off
  dht.begin();
  Wire.begin();
  myStepper.setSpeed(5);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  temp = dht.readTemperature(true);
  hum = dht.readHumidity();
  //get when you first read the sensors
  readtime = millis();

  //MOTOR SETUP
  //set up control pins
  // *ddr_c |= 0x04;
  // *ddr_c |= 0x01;
  // *ddr_c |= 0x02;
  pinMode(33, OUTPUT);
  pinMode(35, OUTPUT);
  pinMode(37, OUTPUT);
  analogWrite(37, 255);
}

void loop() {

  if(timedisplayed == false){
    my_delay(1);
    DateTime now = rtc.now();
    printTime(now);
    U0putchar('\n');
    timedisplayed = true;
  }

  //only update temp and hum if 1 min has elapsed since last read.
  if((millis()-readtime) > 60000){
    temp = dht.readTemperature(true);
    hum = dht.readHumidity();
  }
  
  //
  if(state != 0){
    //display temp and hum
    lcd.setCursor(3, 1);
    lcd.print(temp);
    lcd.setCursor(11, 1);
    lcd.print(hum);
    readtime = millis();
  }

  
    
  //state = disabled
  if(state == 0){
    
    *port_h &= 0xF7;  //blue off
    *port_h &= 0xBF;  //green off
    *port_h &= 0xEF;  //red off
    *port_h |= 0x20;  //Yellow light on
    lcd.clear();

    
    
  }
  //State = idle
  else if(state == 1){
    
    *port_h &= 0xF7;  //blue off
    *port_h &= 0xBF;  //red off
    *port_h &= 0xDF;  //yellow off
    *port_h |= 0x10;  //Green light on


    //LCD DISPLAY
    lcd.display();
    lcd.setCursor(0, 0);
    lcd.print("WT:");
    lcd.setCursor(3, 0);
    lcd.print("         ");
    lcd.setCursor(0, 1);
    lcd.print("TP:");
    lcd.setCursor(8, 1);
    lcd.print("HM:");

    //water level
    level = adc_read(0);
    if(level < 20){
      state = 2;
    }
    else{
      lcd.setCursor(3, 0); // move cursor to (0, 0)
      lcd.print(level);
      if(temp > 60.0){
        state = 3;
      }
    }


  }
  //State = Error
  else if(state == 2){
    
    *port_h &= 0xF7;  //blue off
    *port_h &= 0xDF;  //yellow off
    *port_h &= 0xEF;  //green off
    *port_h |= 0x40;  //red on

    lcd.setCursor(3, 0);
    lcd.print("Water Low");

    //reset button check
    if (*pin_l & 0x40) {
      my_delay(1);
      state = 1;
    }
    
    
  }
  //State = Running
  else if(state == 3){
    
    *port_h &= 0xBF;  //green off
    *port_h &= 0xDF;  //yellow off
    *port_h &= 0xEF;  //red off
    *port_h |= 0x08;  //blue on
    
    //35=dir2 ,37=speed, 33=dir1
    // *port_c &= 0xF7;
    // *port_c |= 0x02;
    digitalWrite(33, LOW);
    digitalWrite(35, HIGH);
    if (temp < 60)
      state = 1;
    if(level < 20){
      state = 2;
    }
    
    

  }

  if(state != 3){
    // *port_c &= 0xF7;
    // *port_c &= 0xFD;
    digitalWrite(33, LOW);
    digitalWrite(35, LOW);
  }

}

// INPUT OUTPUT
void U0Init(int U0baud)
{
  //write this method. Check previous lab codes
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);
  // Same as (FCPU / (16 * U0baud)) - 1;
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0 = tbaud;
}

unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}
unsigned char U0getchar()
{
  return *myUDR0;
}
void U0putchar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}


// ANALOG TO DIGITAL
void adc_init()
{
  // setup the A register
 // set bit 7 to 1 to enable the ADC 
 *my_ADCSRA |= 0b10000000;

 // clear bit 5 to 0 to disable the ADC trigger mode
 *my_ADCSRA &= 0b11011111;

 // clear bit 3 to 0 to disable the ADC interrupt 
 *my_ADCSRA &= 0b11110111;

 // clear bit 0-2 to 0 to set prescaler selection to slow reading
 *my_ADCSRA &= 0b11111000;

  // setup the B register
  // clear bit 3 to 0 to reset the channel and gain bits
 *my_ADCSRB &= 0b11110111;

 // clear bit 2-0 to 0 to set free running mode
 *my_ADCSRB &= 0b11111000;

  // setup the MUX Register
 // clear bit 7 to 0 for AVCC analog reference
 *my_ADMUX &= 0b01111111;

// set bit 6 to 1 for AVCC analog reference
 *my_ADMUX |= 0b01000000;

  // clear bit 5 to 0 for right adjust result
 *my_ADMUX &= 0b11011111;

 // clear bit 4-0 to 0 to reset the channel and gain bits
 *my_ADMUX &= 0b11100000;

}

unsigned int adc_read(unsigned char adc_channel_num) //work with channel 0
{
  // clear the channel selection bits (MUX 4:0)
 *my_ADMUX &= 0b11100000;

  // clear the channel selection bits (MUX 5) hint: it's not in the ADMUX register
 *my_ADCSRB &= 0b11110111;
 
  // set the channel selection bits for channel 0
 *my_ADMUX &= 0b11100000;

  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0b01000000;

  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register and format the data based on right justification (check the lecture slide)
  
  unsigned int val = (*my_ADC_DATA & 0x03FF);
  return val;
}

void my_delay(unsigned int freq)
{
  
  // calc period
  double period = 1.0/double(freq);
  // 50% duty cycle
  double half_period = period/2;
  // clock period def
  double clk_period = 0.0000000625;
  // calc ticks
  unsigned int ticks = half_period/clk_period;
  

  
  *myTCCR1A &= 0xF8;
  // stop the timer
  *myTCCR1B = 0x0;
  // set the counts
  *myTCNT1 = (unsigned int) (65536 - ticks);
  // start the timer 
  *myTCCR1B |= 0x01;
  // wait for overflow
  while((*myTIFR1 & 0x01)==0); 
  // stop the timer
  *myTCCR1B &= 0xF8;   
  // reset TOV           
  *myTIFR1 |= 0x01;
}

void U0putString(const char *s) {
  while (*s) {
    U0putchar(*s);
    s++;
  }
}

void printTime(const DateTime& now) {
    char buf[32];  
    sprintf(buf, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    U0putString(buf);
}

void startButton()
{
  if(state == 0){ //disabled
    my_delay(1);
    state = 1;
    U0putString("Disabled to Idle: ");
  }
  else if(state == 1){ //idle
    my_delay(1);
    state = 0;
    U0putString("Idle to Disabled: ");
  }
  else if(state == 2){ //error
    my_delay(1);
    state = 0;
    U0putString("Error to Disabled: ");
  }
  else if(state == 3){ //running
    my_delay(1);
    state = 0;
    U0putString("Running to Disabled: ");
    
  }

  timedisplayed = false;
  

}
