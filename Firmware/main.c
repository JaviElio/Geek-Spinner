/*
 * GeekSpinner.c
 *
 * Created: 21/11/2017 19:54:01
 * Author : javie
 */ 
#define F_CPU	8000000UL

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "font.h"
#include "textAndShapes.h"


uint8_t spinning = 0;							// spinning -> 1 / not spinning -> 0
volatile uint8_t magnetFlag = 0;				// hall sensor activation flag
uint8_t mode = 1;								// Led mode

volatile uint16_t revs = 0;						// total revolutions
char revsString[6]= "";							// total revolutions (string)
volatile uint16_t timer0_OV = 0;				// timer overflow counter
char millisString[5]="";						// millis per revolution (string)
volatile uint16_t millisRev = 0;				// milliseconds per revolution. 


ISR (PCINT1_vect) {								// magnet sensed
	if (!(PINB & ( 1 << PINB1))) {
		revs++;									// increase counter if PB1 is low
		millisRev = timer0_OV * 0.256;			// Calculate milliseconds per revolution 
		timer0_OV = 0;
		magnetFlag = 1;
	}
}

ISR	(TIM0_OVF_vect) {
	timer0_OV++;
}

void ChangeMode(void){							// Change spinner mode
	
	mode++;
	magnetFlag = 0;								
	
	if (mode < 1 || mode > 5) mode = 1;			// Check boundaries
	
	while (!(PINB & (1 << PINB0))) {			// Wait until button is released
		PORTA = (1 << (mode -1));				// Show active mode while button is pressed
	}
	//_delay_ms(50);
	PORTA = 0x00;								// Switch off leds
	
}

// Returns delay in us
uint16_t CalculateDelay(uint16_t inputMillis) {
	
	float temp = inputMillis*9;	
									
	if (temp > 2000) return 2000;
	if (temp < 100) return 100;
	return (uint16_t) temp;
}

// custom delay
void Delay_Us (uint16_t n) {
	
	while(n--) {
		_delay_us(1);
	}
}


// Returns number of digits of a given int variable
int8_t NumberOfDigits(uint16_t number) {

	if (number<10) return 1;
	if (number<100) return 2;
	if (number<1000) return 3;
	if (number<10000) return 4;
	if (number<100000) return 5;
	
}


int main(void)
{

	DDRA	=	0xFF;					// PORTA outputs
 	DDRB	&=	~(1 << DDB0);			// PB0 input
 	DDRB	&=  ~(1 << DDB1);			// PB1 input
	PORTB	|=	(1 << PB0);				// Pull-Up resistor
	PORTB	|=	(1 << PB1);				// Pull-Up resistor	

	TCCR0B	|=	(1 << CS01);			// Start timer0 and set prescaler clk/8
	TIMSK0	|=	(1 << TOIE0);			// Enable Overflow Interrupt

	PCMSK1	|=	(1 << PCINT9);			// Enable pin interrupt at PB1
	GIMSK	|=	(1 << PCIE1);			// Enable pin change interrupt 1
	sei();								// Enable global interrupts





    /* Replace with your application code */
    while (1) 
    {
		
				
		// Change spinner mode
		if (!(PINB & (1 << PINB0)))	ChangeMode();
		
		
		//Spinner modes
		
		if (mode==1){								// lily pad
			
			for (int Led=0; Led<8; Led++)
			{
				PORTA = 1 << Led;
				_delay_ms(1);
			}
					
			for (int Led=7; Led > 0;Led--)
			{
				PORTA = 1 << Led;
				_delay_ms(1);
			}

		}					
		
		if (mode==2 && magnetFlag == 1) {
			
			
			uint16_t calculatedDelay = CalculateDelay(millisRev);						// Calculate delay for this revolution
			
			for(int ch=0; ch < sizeof(text)-1; ch++) {									// iterates over each character
				for (int col=0; col < 5; col++)											// iterate over each column			
				{
					uint8_t column = pgm_read_byte_near(&(font[text[ch]-32][col]));
					
					for (int dot=0; dot < 8; dot++)										// iterate over each dot 
					{
						// Swap bits
						if (column & (1 << dot)) PORTA |= (uint8_t) (0x80 >> dot);		// (uint8_t) is necessary,if not the byte is filled with '1'
					}
					Delay_Us(calculatedDelay);//_delay_ms(1);
					PORTA = 0x00;	
				}
				PORTA = 0x00;
				Delay_Us(calculatedDelay);//_delay_ms(1);
					
			}
			
			magnetFlag = 0;
			
		}

		if (mode==3 && magnetFlag ==1) {
					
			uint16_t calculatedDelay = CalculateDelay(millisRev);						// Calculate delay for this revolution
					
			sprintf(revsString, "%d", revs);											// change "int" variable to string
		
			for(int ch=0; ch < NumberOfDigits(revs); ch++) {							// iterates over each character
				
				for (int col=0; col < 5; col++)											// iterate over each column
				{
					uint8_t column = pgm_read_byte_near(&(font[revsString[ch]-32][col]));
				
					for (int dot=0; dot < 8; dot++)										// iterate over each dot
					{
						// Swap bits
						if (column & (1 << dot)) PORTA |= (uint8_t) (0x80 >> dot);		// (uint8_t) is necessary,if not the byte is filled with '1'
					}
					Delay_Us(calculatedDelay);
					PORTA = 0x00;
				}
				PORTA = 0x00;
				Delay_Us(calculatedDelay);
			
			}
		
			magnetFlag = 0;
			
		}


		
		if (mode==4 && magnetFlag ==1)
		{
		
			uint16_t calculatedDelay = CalculateDelay(millisRev);

			for ( int idx=0; idx<sizeof(shape_1); idx++)
			{
				PORTA = pgm_read_byte_near(shape_1+idx);
				Delay_Us(calculatedDelay);
			}
			
			PORTA = 0x00;
			magnetFlag=0;

		}
		
		
		if (mode==5 && magnetFlag==1)
		{

			uint16_t calculatedDelay = CalculateDelay(millisRev);

			for ( int idx=0; idx<sizeof(shape_2); idx++)
			{
				PORTA = pgm_read_byte_near(shape_2+idx);
				Delay_Us(calculatedDelay);
			}
			
			PORTA = 0x00;			
			magnetFlag=0;

		}
			

		
		
    }
}
