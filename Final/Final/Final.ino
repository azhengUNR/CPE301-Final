//Alec Zheng, Isaiah Marzan

#define RDA 0x80
#define TBE 0x20  

//BUTTON STUFF
unsigned char* ddr_b = (unsigned char*) 0x24;
unsigned char* port_b = (unsigned char*) 0x25;
unsigned char* ddr_h = (unsigned char*) 0x101;
unsigned char* port_h = (unsigned char*) 0x102;
volatile unsigned char* pin_b = (unsigned char*) 0x23;
int state;
//state 0 = disabled, state 1 = idle, state 2 = running, state 3 = error

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

void setup() {
  U0Init(9600);
  adc_init();
  state = 0;        //start disabled
  *ddr_b &= 0xEF;   //set PB4 to INPUT
  *ddr_h |= 0x08;   //blue CHECK LATER
  *ddr_h |= 0x40;   //red
  *ddr_h |= 0x20;   //yellow
  *ddr_h |= 0x10;   //Green
  *port_b |= 0x10;  //INPUT pull up
}

void loop() {
  switch(state){
    case 0:
      *port_b &= 0xF7;
      *port_h &= 0xBF;
      *port_h &= 0xEF;                //all led off
      *port_h |= 0x20;                //yellow light on
      //button press turns to idle state
      if(*pin_b & 0xEF){ //CHECK LATER
        state = 1;
      }
      

    case 1:
      
      *port_h &= 0xDF;             //yellow off CHECK LATER
      *port_h |= 0x10;             // green light on
      //stop button
      if(*pin_b & 0xEF){
        state = 0;
      }
      //WATER LEVEL MONITORING
      // read the water sensor value by calling adc_read() and check the threshold to display the message accordingly
      // int level = adc_read(0);
      // //if the value is less than the threshold display the value on the Serial monitor
      // if(level >= 10){
      //   state = 3;  //set error state
      //   char error[]= "Water level is too low";
      //   char *output = error[0];
      //   while(*output != '\0'){
      //     U0putchar(*output);
      //     output++;
      //   }
      //   U0putchar('\n');
      // }

    case 2:
      *port_h &= 0xEF;      //green off
      *port_h |= 0x08;      //Blue led on CHECK LATER
      //WATER LEVEL MONITORING
      // read the water sensor value by calling adc_read() and check the threshold to display the message accordingly
      // int level = adc_read(0);
      // //if the value is less than the threshold display the value on the Serial monitor
      // if(level >= 10){
      //   state = 3;  //set error state
      //   char error[]= "Water level is too low";
      //   char *output = error[0];
      //   while(*output != '\0'){
      //     U0putchar(*output);
      //     output++;
      //   }
      //   U0putchar('\n');
      // }

    case 3:
      *port_h &= 0xF7;
      *port_h &= 0xDF;
      *port_h &= 0xEF;                //all led off
      *port_h |= 0x40;                //red light on
      //stop button
      if(*pin_b & 0xEF){
        state = 0;
      }
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