// Carlos Verduzco
// CECS 447
// March 21 2022
// UARTmcu2.c
// Main program for programming to MCU2
// Uses UART to communicate with PC terminal and MCU2
// Slave MCU that receives data from MCU1
// Part of Mode 2 and 3

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
char send_color_back;
unsigned int i;
unsigned int mode2;
unsigned int interrupt;
char string[101];  // global to assist in debugging
unsigned char colors[] = {R,G,B,P,W,D};  // global to assist in debugging
unsigned char color;

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

int main(void){
	Nokia5110_Init();
  Nokia5110_Clear();
	Nokia5110_SetCursor(0, 0); 

  UART_Init();              // initialize UART
  OutCRLF();
	PortF_Init();
	EnableInterrupts();       
  while(1){
		menu_choice = UART1_InChar(); // Receives menu choice from MC1
		if(menu_choice == '2') {
			send_color_back = UART1_InChar(); //flag to allow MC2 to change colors only when MC1 has sent a color
			if(send_color_back == '1') {
				interrupt = 2;
				color = UART1_InChar();//Receives color from MCU1
				LEDS = color;
			}
	}
		if(menu_choice == '3') {
			Nokia5110_Init();
			Nokia5110_Clear();
			Nokia5110_SetCursor(0, 0); 
			UART1_InString(string, 100); //Receives Input from MC1
			OutCRLF();
			Nokia5110_OutString((unsigned char*)string); //outputs string on Nokia LCD
			UART1_OutString(""); //Sends empty string to MC1 to clear previous message
			OutCRLF();
			UART1_OutString("Message Received:"); //Sends message to MC1
			OutCRLF();
			UART1_OutString(string); //Sends message to MC1
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
		UART1_OutChar(LEDS);	//sends LED color to MCU1	
		GPIO_PORTF_ICR_R = 0x01;
		interrupt = 0; //flag indicating whether Port F handler is functional
		send_color_back = '0'; 
		}
	}
	GPIO_PORTF_ICR_R = 0x01;
}
