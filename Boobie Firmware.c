#include <16F688.h>
// Really wanted to add PWM output mode, but ran out of space!
// Not sure if we would want to sacrifice any features for this.
// C Optimised for the CCS PIC Compiler
#fuses INTRC_IO,NOWDT,PUT, NOPROTECT,NOMCLR,NOFCMEN,BROWNOUT_SW//NOMCLR
#use delay(clock=8000000) //Not using the crystal
#use rs232(baud=57600, parity=N, xmit=PIN_C4, rcv=PIN_C5, bits=8, STOP=1)

#define VERSIONMAJ   1
#define VERSIONMIN   3
#define BUFFER_SIZE 5
#define DATASTART 26      //NNNNNNNNNNNNhhmmDDMMYYYY //Programstring is 24 long minu1 first reserved byte
#define USER_PINS 12
#define high_start 32      //For timer was 32 for about a second
unsigned int16 directions;
unsigned int16 outputon;
unsigned int16 interrupts;
unsigned int16 values;      //Value buffer for interrupt system

unsigned BYTE high_count;            //For timer
unsigned char buffer[BUFFER_SIZE];   //Input Buffer
unsigned char buffercount=0;
unsigned char commandready=0;
unsigned char checkints=0;
unsigned char programmode=0x00;
unsigned char programcounter=0x00;
unsigned int16 inttimestamp=0;

unsigned BYTE getbit(int16 position, int16 sourcedata)
{
   if ((sourcedata & 1<<position) != 0)
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

unsigned int16 setbit(int16 position, int16 sourcedata, BYTE value)
{
   if (value != 1)
   {
       return (sourcedata &= ~(1 << position));
   }
   else
   {
      return (sourcedata | (1 << position));
   }
}

void setpin(BYTE pin, BYTE value)
{
   switch (pin)
   {
      case 0: if (value != 0) {output_high(45);}else{output_low(45);} break;
      case 1: if (value != 0) {output_high(44);}else{output_low(44);} break;
      //case 2: if (value != 0) {output_high(43);}else{output_low(43);} break;
      //case 3: if (value != 0) {output_high(61);}else{output_low(61);} break;
      //case 4: if (value != 0) {output_high(60);}else{output_low(60);} break;
      case 5: if (value != 0) {output_high(59);}else{output_low(59);} break;
      case 6: if (value != 0) {output_high(40);}else{output_low(40);} break;
      case 7: if (value != 0) {output_high(41);}else{output_low(41);} break;
      case 8: if (value != 0) {output_high(42);}else{output_low(42);} break;
      case 9: if (value != 0) {output_high(56);}else{output_low(56);} break;
      case 10: if (value != 0) {output_high(57);}else{output_low(57);} break;
      case 11: if (value != 0) {output_high(58);}else{output_low(58);} break;
   }
}

BYTE readpin(BYTE pin)
{
   switch (pin)
   {
      case 0: return(input(45)); break;
      case 1: return(input(44)); break;
      case 2: return(input(43)); break;
      //case 3: return(input(61)); break;
      //case 4: return(input(60)); break;
      case 5: return(input(59)); break;
      case 6: return(input(40)); break;
      case 7: return(input(41)); break;
      case 8: return(input(42)); break;
      case 9: return(input(56)); break;
      case 10: return(input(57)); break;
      case 11: return(input(58)); break;
   }
}

// Stores the state of any input pins, these are the reference for level change detector
void initinterrupts()
{
   BYTE i;
   for (i=0;i<USER_PINS;i++)
   {
      if (getbit(i, directions) == 0x00)
      {
         values = setbit(i,values, readpin(i));
      }
   }   
}

void checkpinterrupts()
{
   BYTE i;
   BYTE tempval;
   for (i=0;i<USER_PINS;i++)
   {
      //if (directions[i] == 0x00 && interrupts[i] == 0x01) //Needs to be an input and needs to have interrupt enabled
      if (getbit(i,directions) == 0x00 && getbit(i, interrupts) != 0x00) //Needs to be an input and needs to have interrupt enabled
      {
         tempval = readpin(i);                  //Check current value for comparison
         if (getbit(i,values) != tempval)
         {
            values = setbit(i,values, tempval);
            if (tempval==0x00)
            {
               printf("\r\nInt: %d low\t%05Lu\r\n>", i+4,inttimestamp);
            }   
            else
            {
               printf("\r\nInt: %d high\t%05Lu\r\n>", i+4,inttimestamp);
            }
         }
      }
   }
}

void setoutputs(void)
{
   BYTE i;
   for (i=0;i<USER_PINS;i++)
   {
      if (getbit(i,directions) != 0x00)
      {
         if (getbit(i,outputon) == 0x00)
         {
            setpin(i,0);
         }   
         else
         {
            setpin(i,1);
         }
      }
   }
}

signed int atoi(char *s)
{
   signed int result;
   int sign, base, index;
   char c;

   index = 0;
   sign = 0;
   base = 10;
   result = 0;

   if (!s)
      return 0;
   // Omit all preceeding alpha characters
   c = s[index++];

   // increase index if either positive or negative sign is detected
   if (c == '-')
   {
      sign = 1;         // Set the sign to negative
      c = s[index++];
   }
   else if (c == '+')
   {
      c = s[index++];
   }

   if (c >= '0' && c <= '9')
   {

      // Check for hexa number
      if (c == '0' && (s[index] == 'x' || s[index] == 'X'))
      {
         base = 16;
         index++;
         c = s[index++];
      }

      // The number is a decimal number
      if (base == 10)
      {
         while (c >= '0' && c <= '9')
         {
            result = 10*result + (c - '0');
            c = s[index++];
         }
      }
      else if (base == 16)    // The number is a hexa number
      {
         c = toupper(c);
         while ( (c >= '0' && c <= '9') || (c >= 'A' && c<='F'))
         {
            if (c >= '0' && c <= '9')
               result = (result << 4) + (c - '0');
            else
               result = (result << 4) + (c - 'A' + 10);

            c = s[index++];
            c = toupper(c);
         }
      }
   }

   if (sign == 1 && base == 10)
       result = -result;

   return(result);
}

void help()
{
   printf("v\tversion\r\nd\tpin config\r\n");
   printf("l\tload config\r\ns\tsave config\r\n");
   printf("f\tformat to defaults\r\n");
   printf("x\treboot\r\n");
   printf("cPPD\tconfig (P)in (D)irection*\r\n");
   printf("rPP\tread (P)in value\r\n");
   printf("wPPV\twrite (P)in (V)alue (0 or 1)\r\n");
   printf("iPPE\t(P)in interrupts (E)nable\r\n");
   printf("\r\n* Direction 0=Input 1=Output\r\n");
   printf("\tPin (PP) 04 to 15\r\n");
}

void printval(BYTE value)
{
   if (value == 0)
   {
      printf("Off");
   }
   else if (value == 1)
   {
      printf("On ");
   }
   else
   {
      printf("NA ");
   }   
}

void listpins()
{
   BYTE i;

   printf("Pin\tDirection\tValue\tInterrupts\r\n");
   printf("01\tVin\r\n");
   printf("02\tGnd\r\n");
   printf("03\t5v\r\n");

   for(i=0;i < USER_PINS;i++)
   {
      printf("%02d\t", i+4);
      if (i==3)
      {
         printf("RX");
      }else if (i==4)
      {
         printf("TX");
      }
      else
      {
         switch (getbit(i,directions))
         {
            case 0:   printf("Input\t\t");
                  printval(readpin(i));
                  break;
            case 1:   printf("Output\t\t");
                  printval(getbit(i,outputon));
                  break;
            default:printf("Error");
                  break;               
         }
         printf("\t");
         switch (getbit(i,directions))
         {
            case 0:   printval(getbit(i, interrupts));
                  break;
            case 1:   printval(2);
                  break;
         }
      }
      printf("\r\n");
   }   

   printf("16\t5v\r\n");
   printf("17\tGnd\r\n");
   printf("18\tVin\r\n");
}

void clearbuffer(char* data)
{
   byte i;
   for (i=0; i<BUFFER_SIZE-1; i++)
   {
      data[i] = '\0';//0x0d;
   }
   buffercount=0;
}


BYTE checkpin(BYTE pinval)
{
   if (pinval > 3 && pinval < 16)
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

BYTE checkstring(char* data, BYTE params)
{
   byte i;
   byte error=0;

   for (i=0; i<=params; i++)
   {
      if (data[i] == '\0')
      {
         error = 1;
         break;
      }
   }

   if (error != 1)
   {
      if (data[params+1] != '\0')
      {
         error=1;
      }
   }

   return error;
}

void loadpins()
{
   BYTE counter=DATASTART;

   *((int8*)&directions + 0) = read_eeprom(counter++);
   *((int8*)&directions + 1) = read_eeprom(counter++);
   *((int8*)&outputon + 0) = read_eeprom(counter++);
   *((int8*)&outputon + 1) = read_eeprom(counter++);
   *((int8*)&interrupts + 0) = read_eeprom(counter++);
   *((int8*)&interrupts + 1) = read_eeprom(counter++);
}

void savepins()
{
   BYTE counter=DATASTART;
   write_eeprom(counter++, *((int8*)&directions + 0));
   write_eeprom(counter++, *((int8*)&directions + 1));
   write_eeprom(counter++, *((int8*)&outputon + 0));
   write_eeprom(counter++, *((int8*)&outputon + 1));
   write_eeprom(counter++, *((int8*)&interrupts + 0));
   write_eeprom(counter++, *((int8*)&interrupts + 1));
   write_eeprom(0x00, 0x55);
}

void formateeprom()
{
   directions=0x00; //Only bits in here so easy to clear.
   outputon=0x00; //Only bits in here so easy to clear.
   interrupts=0x00; //Only bits in here so easy to clear.
   savepins();
}

//Serial Interrupt
#INT_RDA
void SerialInt()
{
   unsigned char incharacter;   
   incharacter=getchar();

   if (programmode == 1)
   {
      printf("*");
      write_eeprom(programcounter,incharacter);
      if (programcounter<24)
      {
         programcounter ++;
      }
      else
      {
         programmode = 0;
      }
   }
   else
   {
      if (incharacter==0x08 || incharacter==0x7F)
      {
         buffer[buffercount] = '\0';
         if (buffercount > 0)
         {
            buffercount--;
         }
      }
      else
      {
         if (incharacter==0x0d)   //CR Pressed - process buffer
         {
            commandready=1;
         }

         if (buffercount < BUFFER_SIZE-1) 
         {
            if (commandready==1)
            {
               buffer[buffercount+1] = '\0';
            }
            else
            {
               buffer[buffercount++] = incharacter;
            }
         }   
      }
   }

   printf("%c",incharacter);
}

#int_rtcc
clock_isr()
{    high_count -= 1;                        //flips from X to 0   
   if(high_count==0)
   {      
      high_count=high_start;              //Inc SECONDS counter every   
   }
   inttimestamp++;
   checkints=1;
   return 0;
}

void main()
{
   BYTE mycount;
   BYTE ourval;
   char pin[2];
   int pinval;
   int parval;

    //setup_oscillator(4000000);
   //setup_adc(adc_clock_internal); //The ADC uses a clock to work
   //setup_adc_ports(ALL_ANALOG); //RC2 as analogue

   set_rtcc(0);
   setup_counters(RTCC_INTERNAL, RTCC_DIV_64);
   enable_interrupts(INT_RDA);
   enable_interrupts(INT_RTCC);
   enable_interrupts(GLOBAL);

   if (read_eeprom(0x00) != 0x55)
   {
      formateeprom();
   }

   loadpins();
   setoutputs();
   initinterrupts();      
   clearbuffer(buffer);
   buffer[0]='v';      //Display ver on boot
   commandready=1;

    while (TRUE)
   {
      if (checkints==1)
      {
         checkpinterrupts();
         checkints=0;
      }
      if (commandready==1)
      {
         commandready=0;
         printf("\r\n");
         switch (buffer[0])
         {
            case '?':   help();
                     break;
            case 'x':   reset_cpu();
                     break;
            case 'f':   printf("Formatting\r\n");
                     formateeprom();
                     loadpins();
                     setoutputs();
                     initinterrupts();
                     break;
            case 'd':   if (buffer[1] == 'c')
                     {
                        for (mycount=0; mycount<=USER_PINS-1; mycount++)         //BYTE zero is reserved!
                        {
                           printf("%d,", mycount+4);
                           printf("%d,", getbit(mycount,directions));
                           printf("%d,", getbit(mycount,outputon));
                           printf("%d,", readpin(mycount));
                           printf("%d\r\n", getbit(mycount, interrupts));
                        }
                        printf("\r\n");
                     }
                     else
                     {
                        listpins();
                     }
                     break;
            case 'l':   loadpins();
                     printf("Loaded\r\n");
                     setoutputs();
                     break;
            case 's':   savepins();
                     printf("Saved\r\n");
                     break;
            case 'c':   if (checkstring(buffer,3) == 0)
                     {
                        if (buffer[3] == '0' || buffer[3] == '1')
                        {
                           pin[0]=buffer[1];
                           pin[1]=buffer[2];
                           pinval = atoi(pin);
                           pin[0]=buffer[3];
                           pin[1]='\0';
                           parval = atoi(pin);
                           if (checkpin(pinval) == 1)
                           {
                              if ((pinval >=6  && pinval <= 8) && parval == 1)
                              {
                                 printf("Pin %d Input only\r\n", pinval);
                              }
                              else
                              {
                                 if (getbit(pinval-4,directions)==1 && parval == 0)
                                 {
                                    setpin(pinval-4,0);
                                 }
                                 printf("Pin %d now ", pinval);
                                 if (parval == 0)
                                 {
                                    printf("Input\r\n");
                                 }
                                 else
                                 {
                                    printf("Output\r\n");
                                 }   
                                 directions = setbit(pinval-4,directions,parval);
                                 //directions[pinval-4]=parval;
                                 setoutputs();
                                 initinterrupts();
                              }
                           }
                           else
                           {
                              printf("\r\nInvalid Pin Val\r\n");
                           }
                        }
                        else
                        {
                           printf("Invalid Dir Val\r\n");
                        }
                     }
                     break;
            case 'w':   if (checkstring(buffer,3) == 0)
                     {
                        if (buffer[3] == '0' || buffer[3] == '1')
                        {
                           pin[0]=buffer[1];
                           pin[1]=buffer[2];
                           pinval = atoi(pin);
                           pin[0]=buffer[3];
                           pin[1]='\0';
                           parval = atoi(pin);
                           if (checkpin(pinval) == 1)
                           {
                              printf("Pin %d output set to %d\r\n", pinval, parval);
                              //outputon[pinval-4]=parval;
                              outputon = setbit(pinval-4,outputon,parval);
                              setoutputs();
                           }
                           else
                           {
                              printf("\r\nInvalid Pin Val\r\n");
                           }
                        }
                        else
                        {
                           printf("Invalid Output Val\r\n");
                        }
                     }
                     break;
            case 'r':   if (checkstring(buffer,2) == 0)
                     {
                        pin[0]=buffer[1];
                        pin[1]=buffer[2];
                        pinval = atoi(pin);
                        if (checkpin(pinval) == 1)
                        {
                           if (getbit(pinval-4,directions)==1)
                           {
                              printf("Pin %d val is %d (Output)\r\n", pinval, getbit(pinval-4,outputon));   
                           }
                           else
                           {
                              printf("Pin %d val is %d (Input)\r\n", pinval, readpin(pinval-4));   
                           }
                        }
                        else
                        {
                           printf("\r\nInvalid Pin Val\r\n");
                        }
                     }
                     break;
            case 'i':   if (checkstring(buffer,3) == 0)
                     {
                        if (buffer[3] == '0' || buffer[3] == '1')
                        {
                           pin[0]=buffer[1];
                           pin[1]=buffer[2];
                           pinval = atoi(pin);
                           pin[0]=buffer[3];
                           pin[1]='\0';
                           parval = atoi(pin);
                           if (checkpin(pinval) == 1)
                           {
                              if (parval == 0)
                              {
                                 printf("Pin %d int disabled\r\n", pinval);
                              }
                              else
                              {
                                 printf("Pin %d int enabled\r\n", pinval);
                              }
                               interrupts = setbit(pinval-4, interrupts,parval);
                              //interrupts[pinval-4]=parval;
                              initinterrupts();
                           }
                           else
                           {
                              printf("\r\nInvalid Pin Val\r\n");
                           }
                        }
                        else
                        {
                           printf("Invalid Int Val\r\n");
                        }
                     }
                     break;
            case 'v':   if (buffer[1] == '~' && buffer[2] == 'C' && buffer[3] == '%')
                     {   
                        programmode=1;
                        programcounter=1;
                     }
                     printf("Archonix Boobie V%d.%d\r\n",VERSIONMAJ,VERSIONMIN);
                     //NNNNNNNNNNNNhhmmDDMMYYYY //Programstring is 24 long minu1 first reserved byte
                     for (mycount=1; mycount<=12; mycount++)         //BYTE zero is reserved!
                     {   
                        ourval = read_eeprom(mycount);
                        if (ourval == '^')
                        {      
                           break;
                        }
                        printf("%c",ourval);
                     }
                     printf(" Boobie was born at ");
                     for (mycount=13; mycount<=16; mycount++)      
                     {   
                        printf("%c",read_eeprom(mycount));
                        if (mycount == 14)
                        {      
                           printf(":");
                        }
                     }
                     printf(" on ");
                     for (mycount=17; mycount<=24; mycount++)         
                     {   
                        printf("%c",read_eeprom(mycount));
                        if (mycount == 18 || mycount == 20)
                        {      
                           printf("/");
                        }
                     }
                     printf("\r\n");
                     break;
            case 0x0d:   
            case 0x00:    break;
            default:    printf("Bad Command\r\n");                              
         }
         clearbuffer(buffer);
         printf(">");
      }
   }   
} 

