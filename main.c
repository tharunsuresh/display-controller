/*
Lab 3 - Improved Display Case Light Controller
Tharun Suresh, Oliver Jin
Nov. 8, 2018

*/

#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdio.h>

#include "defines.h"
#include <util/delay.h>
#include <util/atomic.h>

#include <stdbool.h>
#include "hd44780.h"
#include "lcd.h"
#include <avr/pgmspace.h>


/*interrupt section*/
#define interrupt_pin (1 << 4)

volatile int LED_toggle = on; /*by default local led display is on*/

ISR(PCINT1_vect){

	if(!(PINC & interrupt_pin)){ /*if touch sensor touched*/
		LED_toggle ^= 1;		/*toggle the local led display switch*/
	}
}

/*LED and PWM section*/
#define Red_LED (1 << 6)
#define Green_LED (1 << 5)
#define Blue_LED (1 << 1)

#define Red_button (1 << 2)
#define Green_button (1 << 1)
#define Blue_button (1 << 0)

#define timer0_mode (1 << WGM00) | (1 << WGM01)			/*fast PWM*/
#define timer0_pins (1 << COM0A0) | (1 << COM0A1) | (1 << COM0B0) | (1 << COM0B1) /*both OC0A, 0C0B on*/
#define timer0_clock (1 << CS01) /*Pre-scale factor of 8*/

#define timer1_mode_tccr1a (1 << WGM11)
#define timer1_mode_tccr1b (1 << WGM12) | (1 << WGM13) /*fast PWM*/
#define timer1_pins (1 << COM1A1) | (1 << COM1A0)  /*pin OC1A on*/
#define timer1_clock (1 << CS11) /*Pre-scale factor of 8*/

#define max_brightness 255
#define maxpercent 100
#define percent_increment 10
#define brightness_increment 2.5

uint8_t RED_brightness = off;		/*variables for the brightness of each color*/
uint8_t GREEN_brightness = off;
uint8_t BLUE_brightness = off;

#define button_delay 250


/*LCD setup*/
FILE lcd_stream = FDEV_SETUP_STREAM(lcd_putchar, NULL,_FDEV_SETUP_WRITE);

const char b0[] PROGMEM = "  0";
const char b1[] PROGMEM = " 10";
const char b2[] PROGMEM = " 20";
const char b3[] PROGMEM = " 30";
const char b4[] PROGMEM = " 40";
const char b5[] PROGMEM = " 50";
const char b6[] PROGMEM = " 60";
const char b7[] PROGMEM = " 70";
const char b8[] PROGMEM = " 80";
const char b9[] PROGMEM = " 90";
const char b10[] PROGMEM = "100";

const char LCD_topline[] PROGMEM = "R:    %  G:    %";
const char LCD_bottomline[] PROGMEM = "B:    %         ";
const char state_0[] PROGMEM = "Off";
const char state_1[] PROGMEM = "On ";

PGM_P const string_table[] PROGMEM = {
	b0, b1, b2, b3, b4, b5, b6, b7, b8, b9, b10,
LCD_topline, LCD_bottomline, state_0, state_1 };

#define on 1
#define off 0


int main(void)
{
	/*interrupt enables*/
	sei();						/*global enable*/
	PCICR |= (1 << PCIE1);		/*enable PCINT1*/
	PCMSK1 |= (1 << PCINT12);	/*enable interrupt pin*/
	
	PORTC |= interrupt_pin;		/*enable pull-up for touch sensor input*/
	DDRC &= ~interrupt_pin;		/*make input */
	
	/*LED setup*/
	PORTD = 0xFF;			/*LED default off + inputs pull ups on*/
	DDRD = Red_LED | Green_LED;	/*LED pins are outputs */
	
	PORTB |= Blue_LED;
	DDRB |= Blue_LED;
	
	/*PWM Control*/
	
	TCCR0A = timer0_mode | timer0_pins;
	TCCR0B |= timer0_clock;
	
	OCR0A = off;
	OCR0B = off;
	
	ICR1 = max_brightness;
	TCCR1A = timer1_mode_tccr1a | timer1_pins;
	TCCR1B = timer1_mode_tccr1b | timer1_clock;
	OCR1A = off;
	
	/*LCD initialization*/
	lcd_init();
	stdout = &lcd_stream;
	char buffer[16];
	
	hd44780_wait_ready(true);
	hd44780_outcmd(HD44780_DISPCTL(1,0,0)); /*remove cursor*/
	hd44780_wait_ready(true);

	hd44780_outcmd(0x80);		/*top row for LCD*/
	hd44780_wait_ready(false);
	strcpy_P(buffer, (PGM_P)pgm_read_word(&(string_table[11])));
	fputs(buffer, stdout);
	hd44780_wait_ready(false);
	
	hd44780_outcmd(0xC0);		/*bottom row*/
	hd44780_wait_ready(false);
	strcpy_P(buffer, (PGM_P)pgm_read_word(&(string_table[12])));
	fputs(buffer, stdout);
	hd44780_wait_ready(false);

	
	while (1) {
		
		/*adjusting brightness*/
		if (!(PIND & Blue_button)){		/*if pin D0 is low (i.e. switch for blue was pressed)*/
			if (BLUE_brightness == maxpercent){		/*if max brightness wrap back to 0*/
				BLUE_brightness = off;
			}
			else{
				BLUE_brightness += percent_increment; /*increase incremental brightness*/
			}
			_delay_ms(button_delay);
		}
		if (!(PIND & Green_button)){		/*if pin D1 is low (i.e. switch for green was pressed)*/
			if (GREEN_brightness == maxpercent){		/*if max brightness wrap back to 0*/
				GREEN_brightness = off;
			}
			else{
				GREEN_brightness += percent_increment; /*increase incremental brightness*/
			}
			_delay_ms(button_delay);
		}
		if (!(PIND & Red_button)){		/*if pin D5 is low (i.e. switch for red was pressed)*/
			if (RED_brightness == maxpercent){		/*if max brightness wrap back to 0*/
				RED_brightness = off;
			}
			else{
				RED_brightness += percent_increment; /*increase incremental brightness*/
			}
			_delay_ms(button_delay);
		}
		
		/*displaying LED with appropriate brightness*/
		if(LED_toggle) {
			OCR0A = RED_brightness*brightness_increment;
			OCR0B = GREEN_brightness*brightness_increment;
			OCR1A = BLUE_brightness*brightness_increment;
		}
		else{
			OCR0A = off;
			OCR0B = off;
			OCR1A = off;
		}
		
		/*updating LCD to reflect state*/
	
		hd44780_outcmd(0xCB);	 /*display state on or off*/
		hd44780_wait_ready(false);
		if(LED_toggle){
			strcpy_P(buffer, (PGM_P)pgm_read_word(&(string_table[14])));
		}
		else{strcpy_P(buffer, (PGM_P)pgm_read_word(&(string_table[13])));}
		fputs(buffer, stdout);
		hd44780_wait_ready(false);
	
		hd44780_outcmd(0x83);		/*LCD red brightness level*/
		hd44780_wait_ready(false);
		strcpy_P(buffer, (PGM_P)pgm_read_word(&(string_table[RED_brightness/10])));
		fputs(buffer, stdout);
		hd44780_wait_ready(false);
	 
		hd44780_outcmd(0x8C);		/*LCD green brightness level*/
		hd44780_wait_ready(false);
		strcpy_P(buffer, (PGM_P)pgm_read_word(&(string_table[GREEN_brightness/10])));
		fputs(buffer, stdout);
		hd44780_wait_ready(false);
	
		hd44780_outcmd(0xC3);		/*LCD blue brightness level*/
		hd44780_wait_ready(false);
		strcpy_P(buffer, (PGM_P)pgm_read_word(&(string_table[BLUE_brightness/10])));
		fputs(buffer, stdout);
		hd44780_wait_ready(false);		
		
		
		
	}
}

