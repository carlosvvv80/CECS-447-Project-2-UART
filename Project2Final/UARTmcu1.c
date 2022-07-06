// Carlos Verduzco
// CECS 447
// March 21 2022
// UARTmcu1.c
// Main program for programming to MCU1
// Uses UART to communicate with PC terminal and MCU2
// 3 modes, Communication between MCU1 and Terminal for color change
// Communication between MCU1 and MC2 for color change with each other
// Communication between terminal, MCU1, MC2, and Nokia LCD to display text

#include "UART.h"
#include "Nokia5110.h"
#include "tm4c123gh6pm.h"
#include <string.h>
#include <stdio.h>

#define	LEDS 		(*((volatile unsigned long *)0x40025038))
#define R 0x02
#define G 0x08
#define B 0x04
#define P 0x06
#define W 0x0E
#define D 0x00

#define NVIC_EN0_PORTF 0x40000000

char menu_choice;
char led_choice;

unsigned int i;

unsigned int mode2;

unsigned int interrupt;
//---------------------OutCRLF---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void PortF_Init(void);

extern void DisableInterrupts(void);
extern void EnableInterrupts(void);  

void OutCRLF(void){
  UART0_OutChar(CR);
  UART0_OutChar(LF);
}

void PortF_Init(void){ 
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // (a) activate clock for port F
  while((SYSCTL_PRGPIO_R&0x02) == 0){};
  GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x11;           // allow changes to PF4-0
    
  GPIO_PORTF_AMSEL_R &= ~0x11;        // 3) disable analog function
  GPIO_PORTF_PCTL_R &= 0x000F000F;   // 4) GPIO clear bit PCTL  
  GPIO_PORTF_DIR_R &= ~0x11;          // 5) PF4,PF0 input   
  GPIO_PORTF_AFSEL_R &= ~0x11;        // 6) no alternate function
  GPIO_PORTF_PUR_R |= 0x11;          // enable pullup resistors on PF4,PF0       
  GPIO_PORTF_DEN_R |= 0x11;          // 7) enable digital pins PF4-PF0        
	GPIO_PORTF_DIR_R |= 0x0E;    // (c) make PF3-1 output (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x0E;  //     disable alt funct on PF3-1
	GPIO_PORTF_DEN_R |= 0x0E;     //     enable digital I/O on PF3-1
		
	GPIO_PORTF_IS_R &= ~0x11;     // (d) PF0,4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    //     PF0,4 is not both edges
  GPIO_PORTF_IEV_R |= 0x11;    //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x11;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x11;      // (f) arm interrupt on PF4
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF0FFFFF)|0x00C00000; // (g) priority 6
  NVIC_EN0_R |= NVIC_EN0_PORTF;      // (h) enable interrupt 30 in NVIC
}
//debug code
int main(void){
  char string[101];  // global to assist in debugging
	char string2[101];
	unsigned char color;

  UART_Init();              // initialize UART
  OutCRLF();
	PortF_Init();
	UART_Init();
	EnableInterrupts();       
  while(1){
		UART0_OutString("Welcome to CECS 447 Project 2 UART");
		OutCRLF();
		UART0_OutString("Please choose a communication mode(type 1 or 2 or 3):");
		OutCRLF();
		UART0_OutString("1. PC to MCU1 Only");
		OutCRLF();
		UART0_OutString("2. MC1 to MC2 LED Control");
		OutCRLF();
		UART0_OutString("3. MC1 to MC2 to LCD");
		OutCRLF();
		
		menu_choice = UART0_InChar(); //Receives menu input from terminal
		UART1_OutChar(menu_choice);   //Sends menu choice to MCU2
		OutCRLF();
		if(menu_choice == '1') {
			UART0_OutString("Type letter for LED color: r,b,g,p,w, or d");
			OutCRLF();
			led_choice = UART0_InChar(); //Receives led choice from terminal
			OutCRLF();
			if(led_choice == 'r') {
				LEDS = R;
				UART0_OutString("Red LED is on");
				OutCRLF();
			}
			else if(led_choice == 'b') {
				LEDS = B;
				UART0_OutString("Blue LED is on");
				OutCRLF();
			}
			else if(led_choice == 'g') {
				LEDS = G;
				UART0_OutString("Green LED is on");
				OutCRLF();
			}
			else if(led_choice == 'p') {
				LEDS = P;
				UART0_OutString("Purple LED is on");
				OutCRLF();
			}
			else if(led_choice == 'w') {
				LEDS = W;
				UART0_OutString("White LED is on");
				OutCRLF();
			}
			else if(led_choice == 'd') {
				LEDS = D;
				UART0_OutString("LED is off");
				OutCRLF();
			}
		}
		if(menu_choice == '2') {
			interrupt = 2; //flag that is set allowing interrupts to function
			LEDS = R;
			color = UART1_InChar(); //Receives led color from MC2
			LEDS = color;
			
		}
		if(menu_choice == '3') {
			
			UART0_OutString("Type a message");
			OutCRLF();
			UART0_InString(string, 100); //Receives message from terminal
			OutCRLF();
			UART1_OutString(string); //sends message to MC2
			OutCRLF();
			UART1_InString(string, 100); //receives message from MC2, clears string
			UART1_InString(string2, 100); //receives message from MC2
			UART0_OutString(string2);
			UART1_InString(string2, 100); //receives message from MC2
			UART0_OutString(string2);
			OutCRLF();
			
		}
	}
}

void GPIOPortF_Handler(void)
{	
	for(i=0;i<=200000;i++){}//delay for button press to stabilize
  if(GPIO_PORTF_RIS_R & 0x10) {
		if(interrupt == 2) {
			switch (LEDS) {
				case D:
					LEDS = R;
					break;
				case R:
					LEDS = G;
					break;
				case G:
					LEDS = B;
					break;
				case B:
					LEDS = P;
					break;
				case P:
					LEDS = W;
					break;
				case W:
					LEDS = D;
					break;
				default:
					break;
			}
		}
		GPIO_PORTF_ICR_R = 0x10;
	}

	if(GPIO_PORTF_RIS_R & 0x01) {
		
		if(interrupt == 2) {
		UART1_OutChar('1'); //sends flag value to MC2
		UART1_OutChar(LEDS);		
		GPIO_PORTF_ICR_R = 0x01;
		UART1_OutChar(CR);
		interrupt = 0;
		}
	}
	GPIO_PORTF_ICR_R = 0x01;
}
