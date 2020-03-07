/*Gonzalo Ruiz
* Lab Section:23
* Assignment: Custom Project
*
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/

#include <avr/io.h>
#include <avr/eeprom.h>
#include "io.h"
#include "timer.h"
#include "scheduler.h"

//define custom characters
enum custom_chars {cn, enmy, plyer};
unsigned char player[8] = {0b01110,0b01010,0b00100,0b01110,0b10101,0b00100,0b01010,0b11011};
unsigned char enemy[8] = {0b00000,0b00100,0b01110,0b10101,0b11011,0b01110,0b01010,0b10001};
unsigned char coin[8] = {0b00000,0b01110,0b10001,0b10101,0b10101,0b10001,0b01110,0b00000};
	
const unsigned short TimerPeriod = 275;
const unsigned short NumTasks = 3;
task task_list [3];

unsigned char A, B; //to hold PINA and output PORTB
unsigned char sp = ' '; //to clear character after moving it
unsigned char pcur = 19; //player cursor
unsigned char ecur = 27; //enemy cursor
unsigned char fcur[2] = {16, 32}; //finish line cursor
unsigned char ccur[2] = {6, 15}; //coins cursor
unsigned char lives = 0x07;	//player lives (3)
unsigned char score = 0;
unsigned char cycle = 0;

enum player_states {pstart, wait, jump, down, lose};
int player_tick(int st)
{
	//A = ~PINA;
	switch(st)
	{
		case pstart:
			st = wait;
			break;
		case wait:
			if(lives == 0) st = lose;
			else if( A == 0x01) st = jump;
			else st = wait;
			
			if(lives != 0) {
				LCD_Cursor(pcur);
				LCD_Data(plyer);
				LCD_Cursor(0); }
			break;
		case jump:
				st = down;
				LCD_Cursor(pcur);
				LCD_WriteData(sp);
				pcur = 3;
				LCD_Cursor(pcur);
				LCD_Data(plyer);
				LCD_Cursor(0);
				break;
		case down:
			st = wait;
			LCD_Cursor(pcur);
			LCD_WriteData(sp);
			pcur = 19;
			LCD_Cursor(0);
			break;
		case lose:
			st = wait;
			LCD_Cursor(pcur);
			LCD_WriteData(sp);
			LCD_Cursor(0);
			break;
		default:
			st = pstart;
			break;
	}
	return st;
}

enum lcd_states {Start, disp, lost};
int lcd_tick(int State)
{
	switch(State)
	{
		case Start:
			State = disp;
			break;
		case disp:
			if(lives == 0) State = lost;
			else State = disp;
			
			if(lives != 0) {
			LCD_Cursor(ecur);
			LCD_Data(enmy);
			LCD_Cursor(ecur + 1);
			LCD_WriteData(sp);
			
			LCD_Cursor(ccur[0]);
			LCD_Data(cn);
			LCD_Cursor(ccur[0] + 1);
			LCD_WriteData(sp);
			
			LCD_Cursor(ccur[1]);
			LCD_Data(cn);
			LCD_Cursor(ccur[1] + 1);
			LCD_WriteData(sp);
			
			if(cycle > 5) {
				LCD_Cursor(fcur[0]);
				LCD_Data(cn);
				LCD_Cursor(fcur[0] + 1);
				LCD_WriteData(sp);
				LCD_Cursor(fcur[1]);
				LCD_Data(cn);
				LCD_Cursor(fcur[1] + 1);
				LCD_WriteData(sp);
			}
			
			LCD_Cursor(1);
			LCD_WriteData(sp);
			LCD_Cursor(17);
			LCD_WriteData(sp); }
			LCD_Cursor(0);
			break;
		case lost:
			State = disp;
			LCD_DisplayString(1, "Game Over!! ");
			LCD_WriteData(score + '0');
			break;
		default:
			State = Start;
			break;
	}
	return State;
}

enum move_states {start, mov};
int move_tick(int state)
{
	switch(state)
	{
		case start:
			state = mov;
			break;
		case mov:
			ecur--;
			ccur[0]--;
			ccur[1]--;
			if(ecur == 16) {
				ecur = 32;
				cycle++; }
			if(ccur[0] == 0)
				ccur[0] = 16;
			if(ccur[1] == 0)
				ccur[1] = 16;
			if(cycle > 5) {
				fcur[0]--;
				fcur[1]--; }
			break;
		default:
			state = start;
			break;
	}
	return state;
}

void inc_score() 
{ 
	B = 0x01;
	score++; 
}

void dec_lives()
{
	if(lives == 0x07) lives = 0x03;
	else if (lives == 0x03) lives = 0x01;
	else if (lives == 0x01) lives = 0;
}

void reset()
{
	//LCD_ClearScreen();
	LCD_DisplayString(1, "                ");
	pcur = 19; //player cursor
	ecur = 25; //enemy cursor
	ccur[0] = 6; //coin cursor
	ccur[1] = 12;
	lives = 0x07;	//player lives (3)
	score = 0;
}

int main(void)
{	
	DDRA = 0x00; PORTA = 0xFF; // input
	DDRB = 0xFF; PORTB = 0x00; // output
	DDRD = 0xFF; PORTD = 0x00; // output
	DDRC = 0xFF; PORTC = 0x00; // output
	
	/*unsigned char Data = 0x0F;
	unsigned char rd = 0x00;
	eeprom_update_byte (( uint8_t *) 46, Data );
	rd = eeprom_read_byte (( uint8_t *) 46);*/
	
	LCD_init();
	LCD_CreateCustomChar(player, plyer);
	LCD_CreateCustomChar(enemy, enmy);
	LCD_CreateCustomChar(coin, cn);
	
	TimerSet(TimerPeriod);
	TimerOn();
	
	unsigned short i = 0;
	task_list[i].elapsedTime = 0;
	task_list[i].period = TimerPeriod;
	task_list[i].state = Start;
	task_list[i++].TickFct = &lcd_tick;
	task_list[i].elapsedTime = 0;
	task_list[i].period = TimerPeriod;
	task_list[i].state = start;
	task_list[i++].TickFct = &move_tick;
	task_list[i].elapsedTime = 0;
	task_list[i].period = 250;
	task_list[i].state = TimerPeriod;
	task_list[i++].TickFct = &player_tick;
	
    while (1) 
    {
		B = 0;
		A = ~PINA;
		if(pcur == ecur) dec_lives();
		if(pcur == ccur[0] || pcur == ccur[1]) inc_score();
		if(A == 0x02) reset();
		PORTB = B;
		PORTD = ~lives;
		
		for(i = 0; i < NumTasks; i++){
			if(task_list[i].elapsedTime >= task_list[i].period){
				task_list[i].state = task_list[i].TickFct(task_list[i].state);
				task_list[i].elapsedTime = 0;
			}task_list[i].elapsedTime += TimerPeriod;
		}
		while(!TimerFlag);
		TimerFlag = 0;
		
    }
}

