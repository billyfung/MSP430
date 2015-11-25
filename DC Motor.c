#include <msp430.h>

unsigned int duty1 = 1200;
unsigned int duty2 = 1200;

#pragma vector=USCI_A1_VECTOR
__interrupt void UART_RX(void)
{
	if(UCA1IFG & UCRXIFG)
	{
		unsigned char rx_byte = UCA1RXBUF;
		if(rx_byte>127){
			duty1 = (255 - rx_byte) * 9;
			duty2 = 1200;
		}
		else{
			duty1 = 1200;
			duty2 = (128 - rx_byte) * 9;
		}
		UCA1TXBUF = rx_byte;										// echo back rx_byte
	}
}

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;										// Stop watchdog timer

	//set up clock; use DCO for everything
	CSCTL0 = CSKEY;													// enable clock write access
	CSCTL1 = DCORSEL + DCOFSEL_3;									// DCO = 24MHz
	CSCTL2 = SELA_3 + SELS_3 + SELM_3;								// DCO for MCLK, SMCLK and ACLK
	CSCTL3 = DIVA0;													// Divider = 1 for MCLK & SMCLK; Divider = 2 for ACLK


	//set up timer B0
	TB0CTL = TASSEL1 + MC0;											// SMCLK divider = 1; Up mode
	TB0CCTL0 = OUTMOD2;												// toggle mode
	TB0CCTL1 = OUTMOD0 + OUTMOD1;									// set/reset mode
	TB0CCTL2 = OUTMOD0 + OUTMOD1;									// set/reset mode
	TB0CCR0 = 1200;													// 16bit number to limit upcount; 20kHz
	TB0CCR1 = 600;
	TB0CCR2 = 300;

	//set up timer B1
	TB1CTL = TASSEL1 + MC0;											// SMCLK divider = 1; Up mode
	TB1CCTL0 = OUTMOD2;												// toggle mode
	TB1CCTL1 = OUTMOD0 + OUTMOD1;									// set/reset mode
	TB1CCTL2 = OUTMOD0 + OUTMOD1;									// set/reset mode
	TB1CCR0 = 1200;													// 16bit number to limit upcount; 20kHz
	TB1CCR1 = 600;
	TB1CCR2 = 300;

	//PWM output to AIN1_DRV1 & AIN2_DRV1
	P1DIR |= BIT4 + BIT5;// + BIT0 + BIT1;
	P1SEL0 |= BIT4 + BIT5;// + BIT0 + BIT1;
	P1SEL1 &= ~(BIT4 + BIT5);// + BIT0 + BIT1);

	//PWM output to BIN1_DRV1 & BIN2_DRV1
	P3DIR |= BIT4 + BIT5;// + BIT0 + BIT1;
	P3SEL0 |= BIT4 + BIT5;// + BIT0 + BIT1;
	P3SEL1 &= ~(BIT4 + BIT5);// + BIT0 + BIT1);

	//set up UART
	P2SEL1 |= BIT5 + BIT6;											// UCA1 P2.5 & P2.6
	P2SEL0 &= ~(BIT5 + BIT5);
	UCA1CTLW0 = UCSSEL0; 											// use ACLK (12MHz) for UART
	UCA1BRW = 78;													// set baud rate to 9600 from 12MHz clock
	UCA1MCTLW = UCBRF1 + UCOS16;									// set modulation bits
	UCA1IE |= UCRXIE;												// enable receive interrupt


	_EINT();			    // enable global interrupts

	while(1)
	{
		TB0CCR1 = duty1;
		TB0CCR2 = duty2;
		//UCA1TXBUF = '.';
		volatile unsigned int i = 10000;                          // SW Delay
		do i--;
		while (i != 0);

	}

}
