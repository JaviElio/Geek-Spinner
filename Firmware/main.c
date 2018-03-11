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
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "font.h"
#include "textAndShapes.h"


uint8_t spinning = 0;							// spinning -> 1 / not spinning -> 0
volatile uint8_t magnetFlag = 0;				// hall sensor activation flag
uint8_t mode = 1;								// Led mode

volatile uint16_t revs = 0;						// total revolutions
char revsString[6]= "";							// total revolutions (string)
char rpmString[6]="";							// rpm (string)
volatile uint16_t timer0_OV = 0;				// timer overflow counter
volatile uint16_t oldTimer0_OV = 800;			// previous overflow counter
volatile uint16_t millisRev = 0;				// milliseconds per revolution. 


// Magnet sensed interrupt
// calculate time per revolution
ISR (PCINT1_vect) {								// magnet sensed
	if (!(PINB & ( 1 << PINB1))) {

		millisRev = timer0_OV * 0.256;			// Calculate microseconds per revolution
		oldTimer0_OV = timer0_OV;				// Keep overflow counter
		timer0_OV = 0;							// Reset overflow counter
		
		revs++;									// increase revolutions counter
		magnetFlag = 1;							// flag "magnet sensed"
	}
}

// Timer 0 overflow interrupt
// Used to calculate timer per revolution
ISR	(TIM0_OVF_vect) {
	timer0_OV++;
	if (timer0_OV>65500) timer0_OV=65500;
}


ISR(WDT_vect) {

	wdt_disable();  // disable watchdog
}


// Change spinner mode when button is pressed
void ChangeMode(void){							// Change spinner mode
	
	mode++;
	spinning = 0;
	magnetFlag = 0;								
	
	if (mode < 1 || mode > 8) mode = 1;			// Check boundaries
	
	while (!(PINB & (1 << PINB0))) {			// Wait until button is released
		PORTA = (1 << (mode -1));				// Show active mode while button is pressed
		PORTB &= ~(1 << PB2);					// Blue led off
		_delay_ms(50);
	}

	PORTA = 0x00;								// Switch off leds
	PORTB |= (1 << PB2);						// Blue led off
}

// Returns delay in us
uint16_t CalculateDelay(uint16_t inputMillis) {

	return (uint16_t) inputMillis*9.1;
}

// custom delay
void Delay_Us (uint16_t n) {
	
	while(n--) {
		_delay_us(1);
	}
}


//Delay to print lower text
//Functions needs number of character of first line
void DelayLowerText(uint8_t charsFirstLine, uint16_t calculatedDelay) {
	
	// (5 columns x 6 characters) x calculatedDelay;
	Delay_Us( 5*(6-charsFirstLine)*calculatedDelay);
	
}

// Returns number of digits of a given int variable
int8_t NumberOfDigits(uint16_t number) {

	if (number<10) return 1;
	if (number<100) return 2;
	if (number<1000) return 3;
	if (number<10000) return 4;
	if (number<100000) return 5;
	
}


// Print shapes
void ShowShape( const char shape[], uint8_t shapeSize){

	uint16_t calculatedDelay = CalculateDelay(millisRev);

	for ( int idx=0; idx< shapeSize; idx++)
	{
		PORTA = pgm_read_byte_near(shape+idx);
		Delay_Us(calculatedDelay);
	}
	
	PORTA = 0x00;
	magnetFlag=0;
	
};


int main(void)
{

	// Set I/O
	DDRA	=	0xFF;					// PORTA outputs (red leds)
	DDRB	|=	(1 << PB2);				// PB2 output (blue led)
 	DDRB	&=	~(1 << DDB0);			// PB0 input (Button)
 	DDRB	&=  ~(1 << DDB1);			// PB1 input (Hall Sensor)
	PORTB	|=	(1 << PB0);				// Pull-Up resistor
	PORTB	|=	(1 << PB1);				// Pull-Up resistor	

	// Set Timer0
	TCCR0B	|=	(1 << CS01);			// Start timer0 and set prescaler clk/8
	TIMSK0	|=	(1 << TOIE0);			// Enable Overflow Interrupt

	// Reduce power consumption
	ACSR |= (1 << ACD);										// Disable ADC
	PRR	 |= (1 << PRTIM1) | (1 << PRUSI) | (1 << PRADC);	// Disable Timer1, USI and ADC
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);

	// Set interrupt
	PCMSK1	|=	(1 << PCINT9);			// Enable pin interrupt at PB1
	GIMSK	|=	(1 << PCIE1);			// Enable pin change interrupt 1
	sei();								// Enable global interrupts
	
	
	
	
    /* Replace with your application code */
    while (1) 
    {
		
				
		// Change spinner mode
		if (!(PINB & (1 << PINB0)))	ChangeMode();


		
		// If speed is low -> not spinning
		if (oldTimer0_OV >= 800 || timer0_OV > 800)
		{
			spinning = 0;
			PORTB |= (1 << PB2);						// Blue led on
		}
		 else if (oldTimer0_OV < 800)
		 {
			 spinning = 1;
			 PORTB &= ~(1 << PB2);						// Blue led off
		 }
	
	
		 
		 // If spinner is stopped during 16s. (aprox) then sleep
		 if (timer0_OV >= 65500) {
			
			PORTB &= ~(1 << PB2);									// Blue led off
			WDTCSR |= (1 << WDCE) | (1 << WDE);						// Enable WatchDog
			WDTCSR |= (1 << WDIE) | (1 << WDP3);					// Set prescaler to 4s

			sleep_mode();								// go to sleep

			PORTB |= (1 << PB2);						// Blue led on
			_delay_ms(15);
			PORTB &= ~(1 << PB2);						// Blue led off	

		 }
				
	
		
				///////////////////
				// Spinner modes //
				///////////////////


		// Lily pad
		if (mode==1 && spinning==1){
	
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

			PORTA = 0x00;

		}		

		
		
		// Smiley
		if (mode==2 && magnetFlag==1 && spinning==1)
		{

			ShowShape(shape_2, sizeof(shape_2));

		}
	
		
		// Fan
		if (mode==3 && magnetFlag==1 && spinning==1)
		{

			ShowShape(shape_3, sizeof(shape_3));

		}

	
		
		
		// Heart
		if (mode==4 && spinning == 1 && magnetFlag== 1)
		{
			
			ShowShape(shape_1, sizeof(shape_1));

		}		
		
						

		// RPM
		if (mode==5 && magnetFlag==1 && spinning==1){
			
			uint16_t calculatedDelay = CalculateDelay(millisRev);
			
			uint16_t rpm = (oldTimer0_OV *0.256);
			rpm = 60000 / rpm;															// Calculate rpm
			//uint16_t rpm = oldTimer0_OV;
			sprintf(rpmString, "%d", rpm);												// rpm(int) to string


			//PRINT UPPER TEXT
			for(int ch=0; ch < NumberOfDigits(rpm); ch++) {								// iterates over each character
				
				for (int col=0; col < 5; col++)											// iterate over each column
				{
					uint8_t column = pgm_read_byte_near(&(font[rpmString[ch]-32][col]));
					
					for (int dot=0; dot < 8; dot++)										// iterate over each dot
					{
						// Swap bits
						if (column & (1 << dot)) PORTA |= (uint8_t) (0x80 >> dot);		// (uint8_t) is necessary,if not the byte is filled with '1'
					}
					Delay_Us(calculatedDelay);
					PORTA = 0x00;
				}
				
				Delay_Us(calculatedDelay);
				
			}
			
			//Wait to print lower text
			DelayLowerText(NumberOfDigits(rpm), calculatedDelay);
			
			//PRINT LOWER TEXT
			for (int ch = 2; ch>=0; ch--) {												// iterates over each character
				for (int col=4; col>=0; col--)											// iterates over each column
				{
					PORTA = pgm_read_byte_near(&(font[rpmText[ch]-32][col]));
					Delay_Us(calculatedDelay);
					PORTA=0x00;
				}
				Delay_Us(calculatedDelay);
			}


			
			magnetFlag = 0;
			
			
			
		}


		// Total revolutions
		if (mode==6 && magnetFlag ==1 && spinning ==1) {
			
			uint16_t calculatedDelay = CalculateDelay(millisRev);						// Calculate delay for this revolution
			
			sprintf(revsString, "%d", revs);											// change "int" variable to string

			//PRINT UPPER TEXT
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
				
				Delay_Us(calculatedDelay);
				
			}
			
			//Wait to print lower text
			DelayLowerText(NumberOfDigits(revs), calculatedDelay);
			
			//PRINT LOWER TEXT
			for (int ch = 2; ch>=0; ch--) {												// iterates over each character
				for (int col=4; col>=0; col--)											// iterates over each column
				{
					PORTA = pgm_read_byte_near(&(font[revText[ch]-32][col]));
					Delay_Us(calculatedDelay);
					PORTA=0x00;
				}
				Delay_Us(calculatedDelay);
			}


			
			magnetFlag = 0;
			
		}


		// TEXT		
		if (mode==7 && magnetFlag == 1 && spinning==1) {
			
			
			uint16_t calculatedDelay = CalculateDelay(millisRev);						// Calculate delay for this revolution
			
			
			//PRINT UPPER TEXT
			for(int ch=0; ch < sizeof(upperText)-1; ch++) {								// iterates over each character

				for (int col=0; col < 5; col++)											// iterate over each column			
				{

					uint8_t column = pgm_read_byte_near(&(font[upperText[ch]-32][col]));
					
					for (int dot=0; dot < 8; dot++)										// iterate over each dot 
					{
						// Swap bits
						if (column & (1 << dot)) PORTA |= (uint8_t) (0x80 >> dot);		// (uint8_t) is necessary,if not the byte is filled with '1'
					}
					Delay_Us(calculatedDelay);
					PORTA = 0x00;	
				}

				Delay_Us(calculatedDelay);
			}

			// Delay to write lower text			
			Delay_Us(calculatedDelay*3);
			
			//PRINT LOWER TEXT
			for (int ch = sizeof(lowerText)-2; ch>=0; ch--) {								// iterates over each character

				for (int col=4; col>=0; col--)												// iterates over each column
				{
					PORTA = pgm_read_byte_near(&(font[lowerText[ch]-32][col]));
					Delay_Us(calculatedDelay);
					PORTA=0x00;	
				}			
				Delay_Us(calculatedDelay);
			}
			
			magnetFlag = 0;
			
		}

		if (mode==8 && magnetFlag == 1 && spinning==1) {
			
			static uint8_t repetitionCouneter = 0;
			if (repetitionCouneter>80) repetitionCouneter = 0;

			if (repetitionCouneter < 20) {
				ShowShape(shape_1, sizeof(shape_1));
				repetitionCouneter++;
			}
			
			if (repetitionCouneter >=20 && repetitionCouneter < 40) {
				ShowShape(shape_2, sizeof(shape_2));
				repetitionCouneter++;
			}
			
			if (repetitionCouneter >=40 && repetitionCouneter < 60) {
				ShowShape(shape_3, sizeof(shape_2));
				repetitionCouneter++;
			}
			
			if (repetitionCouneter >= 60)
			{
				
				
				uint16_t calculatedDelay = CalculateDelay(millisRev);						// Calculate delay for this revolution
							
							
				//PRINT UPPER TEXT
				for(int ch=0; ch < sizeof(upperText)-1; ch++) {								// iterates over each character

					for (int col=0; col < 5; col++)											// iterate over each column
					{

						uint8_t column = pgm_read_byte_near(&(font[upperText[ch]-32][col]));
									
						for (int dot=0; dot < 8; dot++)										// iterate over each dot
						{
							// Swap bits
							if (column & (1 << dot)) PORTA |= (uint8_t) (0x80 >> dot);		// (uint8_t) is necessary,if not the byte is filled with '1'
						}
						Delay_Us(calculatedDelay);
						PORTA = 0x00;
					}

					Delay_Us(calculatedDelay);
				}

				// Delay to write lower text
				Delay_Us(calculatedDelay*3);
							
				//PRINT LOWER TEXT
				for (int ch = sizeof(lowerText)-2; ch>=0; ch--) {								// iterates over each character

					for (int col=4; col>=0; col--)												// iterates over each column
					{
						PORTA = pgm_read_byte_near(&(font[lowerText[ch]-32][col]));
						Delay_Us(calculatedDelay);
						PORTA=0x00;
					}
					Delay_Us(calculatedDelay);
				}
							
				magnetFlag = 0;
				repetitionCouneter++;
			}
			
			
		}
		
    }
}