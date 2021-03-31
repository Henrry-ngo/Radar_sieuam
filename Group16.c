#include<16f887.h>
#include<string.h>
#include<stdlib.h>
#fuses HS
#fuses NOWDT
#use delay(clock=20M)
#use rs232(baud=9600, xmit=pin_c6, rcv=pin_c7)

#bit tmr1if = 0x0C.0
#bit tmr1on = 0x10.0
#BYTE trisc=0x87
#BYTE portc=0x07
#bit RC2=0x7.2
// LCD pinout setting
#define LCD_ENABLE_PIN  PIN_d2                                    ////
#define LCD_RS_PIN      PIN_d0                                    ////
#define LCD_RW_PIN      PIN_d1                                    ////
#define LCD_DATA4       PIN_D4                                    ////
#define LCD_DATA5       PIN_D5                                    ////
#define LCD_DATA6       PIN_D6                                    ////
#define LCD_DATA7       PIN_D7

// HC-SR04 pinout

#define trigPin         PIN_c1
#define echoPin         PIN_c2     

#include<lcd.c>
// Khai bao bien toan cuc
// Can tao ra tan so 50Hz = 20.000us
// Don vi delay nho nhat = 4u ---> dem 5000 lan
// T_on = [530,2140](us) ---> 2,65% -> 12,05%
// Vay gia tri T_on = [265,1205]

char b;
unsigned int16 T_on, C;
double distance, getVar;
int1 hasEcho;
unsigned char Angle[20];
unsigned char Dist[20];

#int_RDA
void interrupt_uart()
{
   if(kbhit())
   {
      b = getch();
   }
}
#int_CCP1                              // Chuong trinh ngat CCP1 o chan RC2
void interrupt_ccp1()
{
   if(input(pin_c2))                    // Neu a xung canh len
   {
      set_timer1(0);                  // Reset timer1 ve 0
      setup_ccp1(ccp_capture_fe);     // Chuyen CCP1 capture canh xuong
   }
   if(!input(pin_c2))                   // Neu la xung canh duoi
   {
      getVar = CCP_1;                  // CCP_1 = tra tri TMR1
      // 
      distance = (getVar*0.8)/58;
      setup_ccp1(ccp_capture_re);     // Chuyen CCP1 capture canh len
      hasEcho = 1;                     // Co xung echo tra ve
      disable_interrupts(int_timer1);   // Tat ngat Timer1
   }
}

#int_timer1                                  // Chuong trinh ngat timer1 tran
void interrupt_timer1()
{
   if(!hasEcho)                              
   {
      // Tran Timer 1 ma ko co xung echo thi cho qua( Xung echo toi qua 30ms, tran Timer1 52ms)
      hasEcho = 1;
      distance = -1;
   }
   disable_interrupts(int_timer1);
}

void Delay_4us(unsigned int16 t)
{
   while(t>0)  t--;
}

float Convert(float x, float in_min, float in_max, float out_min, float out_max)
   {
      float tam = (out_max - out_min)/(in_max - in_min);
      tam = tam * (x - in_min);
      tam = tam + out_min;
      return tam;
      
   }
   
void Write(unsigned int16 goc, signed int16 t)
{
   T_on = Convert(goc,0,180,265,1205);
   t=t/20;
   for(;t>0;t--)
   {
      output_high(pin_c0);
      Delay_4us(T_on);
      output_low(pin_c0);
      Delay_4us(C-T_on);
   }
}

void trig()
{
    output_high(trigPin);
    Delay_4us(3);
    output_low(trigPin);
}

void lcd_display(unsigned int8 x)
{
   lcd_putc(x/100+0x30);
   lcd_putc(x/10%10+0x30);
   lcd_putc(x%10+0x30);
}

void main()
{
   set_tris_d(0x00);
   set_tris_c(0x84);          // 0x10000100
   set_tris_b(0x00);
   // Khoi tao timer1
   setup_timer_1(t1_internal | t1_div_by_4);
   set_timer1(0);
   // Thiet lap CCP 1 bat canh len
   setup_ccp1(ccp_capture_fe);
   // Khoi tao cho phep ngat
   enable_interrupts(global);
   enable_interrupts(int_timer1);
   enable_interrupts(int_ccp1);
   // Khi tao LCD
   lcd_init();
   output_b(0x00);
   output_c(0x00);
   T_on = 0;
   C = 10000;
   distance = 0;
   getVar = 0;
   hasEcho=0;
   b = '1';
   while(true)
   {
      for(unsigned int8 i=0; i<181; i++)
      {          
         Write(i,20);
         lcd_gotoxy(1,1);   
         lcd_putc("GOC QUAY:");
         sprintf(Angle,"%d", i);
         lcd_gotoxy(10,1);
         lcd_display(atoi(Angle));
         lcd_gotoxy(13,1);   
         lcd_putc(0xDF);
         delay_ms(1);
         set_timer1(0);
         enable_interrupts(int_timer1);
         hasEcho = 0;
         trig();
         output_low(pin_b0);
         while(!hasEcho);
//!         lcd_putc("\f");
         if(distance >=3 && distance <= 90)
         {
            if(b == '1')      output_high(pin_b0);
            else              output_low(pin_b0);
            lcd_gotoxy(1,2);
            lcd_putc("KH/ CACH:");
            lcd_gotoxy(10,2);
            sprintf(Dist,"%3.0f",distance);
            lcd_display(atof(Dist));
            lcd_gotoxy(13,2);   
            lcd_putc("cm");
            delay_ms(1);
            printf("%3.0f\r", distance);
         }
         else
         {
            lcd_gotoxy(1,2);
            lcd_putc("   NO OBJECT   ");
            printf("%d\r",0);
         }
            printf("%u\r", i);
            
       
      }
      
      for(unsigned int8 x=179; x > 0; x--)
      {
         Write(x,20);
         lcd_gotoxy(1,1);   
         lcd_putc("GOC QUAY:");
         sprintf(Angle,"%d",x);
         lcd_gotoxy(10,1);
         lcd_display(atoi(Angle));
         lcd_gotoxy(13,1);   
         lcd_putc(0xDF);
         delay_ms(1);
         set_timer1(0);
         enable_interrupts(int_timer1);
         hasEcho = 0;
         trig();
         output_low(pin_b0);
         while(!hasEcho);
//!         lcd_putc("\f");
         if(distance >=3 && distance <= 90)
         {
            if(b == '1')      output_high(pin_b0);
            else              output_low(pin_b0);
            lcd_gotoxy(1,2);
            lcd_putc("KH/ CACH:");
            lcd_gotoxy(10,2);
            sprintf(Dist,"%3.0f",distance);
            lcd_display(atof(Dist));
            lcd_gotoxy(13,2);   
            lcd_putc("cm");
            delay_ms(1);
            printf("%3.0f\r", distance);
            
         }
         else
         {
            lcd_gotoxy(1,2);
            lcd_putc("   NO OBJECT   ");
            printf("%d\r",0);
         }
         printf("%u\r", x);
         
         
      }
   }
}

