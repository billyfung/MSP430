#include <msp430.h>

unsigned int duty1 = 1200;
unsigned int duty2 = 1200;
unsigned int statePWM = 0;//PWM state variable (0-7)				VICEDIT
unsigned int stateDir = 0;//Direction state variable 0:FWD 1:BWD	VICEDIT

#pragma vector=USCI_A1_VECTOR
__interrupt void UART_RX(void)
{
	if(UCA1IFG & UCRXIFG)
	{
		unsigned char rx_byte = UCA1RXBUF;

		if(rx_byte>128){
			stateDir = 0;
			CSCTL3 = DIVA0 + DIVS_0;
			TB0CCR0 = (255-rx_byte)*400 + 100;
		}
		else if (rx_byte < 128){
			stateDir = 1;
			CSCTL3 = DIVA0 + DIVS_0;
			TB0CCR0 = (127-rx_byte)*400 + 100;
		}
		else{ // 3Hz
			stateDir = 0;
			CSCTL3 = DIVA0 + DIVS_4;
			TB0CCR0 = 60100;
		}
		//UCA1TXBUF = rx_byte;										// echo back rx_byte
	}
}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer_B (void)
{
	P2OUT ^= BIT2;
	if(stateDir == 0)
	{
		  P1OUT = 0;
		  P3OUT = 0;
		if(statePWM == 0)
		{
			  P1OUT = 0;
			  P3OUT = BIT4;
			  statePWM++;
		}
		else if(statePWM == 1)
		{
			  P1OUT = BIT5;
			  P3OUT = BIT4;
			  statePWM++;
		}
		else if(statePWM == 2)
		{
			  P1OUT = BIT5;
			  P3OUT = 0;
			  statePWM++;
		}
		else if(statePWM == 3)
		{
			  P1OUT = BIT5;
			  P3OUT = BIT5;
			  statePWM++;
		}
		else if(statePWM == 4)
		{
			  P1OUT = 0;
			  P3OUT = BIT5;
			  statePWM++;
		}
		else if(statePWM == 5)
		{
			  P1OUT = BIT4;
			  P3OUT = BIT5;
			  statePWM++;
		}
		else if(statePWM == 6)
		{
			  P1OUT = BIT4;
			  P3OUT = 0;
			  statePWM++;
		}
		else if(statePWM == 7)
		{
			  P1OUT = BIT4;
			  P3OUT = BIT4;
			  statePWM = 0;
		}
	}
	else
	{
		P1OUT = 0;
		P3OUT = 0;
		if(statePWM == 0)
		{
			  P1OUT = 0;
			  P3OUT = BIT5;
			  statePWM++;
		}
		else if(statePWM == 1)
		{
			  P1OUT = BIT5;
			  P3OUT = BIT5;
			  statePWM++;
		}
		else if(statePWM == 2)
		{
			  P1OUT = BIT5;
			  P3OUT = 0;
			  statePWM++;
		}
		else if(statePWM == 3)
		{
			  P1OUT = BIT5;
			  P3OUT = BIT4;
			  statePWM++;
		}
		else if(statePWM == 4)
		{
			  P1OUT = 0;
			  P3OUT = BIT4;
			  statePWM++;
		}
		else if(statePWM == 5)
		{
			  P1OUT = BIT4;
			  P3OUT = BIT4;
			  statePWM++;
		}
		else if(statePWM == 6)
		{
			  P1OUT = BIT4;
			  P3OUT = 0;
			  statePWM++;
		}
		else if(statePWM == 7)
		{
			  P1OUT = BIT4;
			  P3OUT = BIT5;
			  statePWM = 0;
		}
	}
}

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;										// Stop watchdog timer

	//set up clock; use DCO for everything
	CSCTL0 = CSKEY;													// enable clock write access
	CSCTL1 = DCORSEL + DCOFSEL_3;									// DCO = 24MHz
	CSCTL2 = SELA_3 + SELS_3 + SELM_3;								// DCO for MCLK, SMCLK and ACLK
	CSCTL3 = DIVA0 + DIVS_4;										// Divider = 1 for MCLK & SMCLK; Divider = 2 for ACLK

	//set up timer B0
	TB0CTL = TBSSEL_2 + MC0 + ID_0;									// SMCLK divider = 1; Up mode
	TB0CCTL0 = OUTMOD2 + CCIE;										// toggle mode
	TB0CCTL1 = OUTMOD0 + OUTMOD1;									// set/reset mode
	TB0CCTL2 = OUTMOD0 + OUTMOD1;									// set/reset mode
	TB0CCR0 = 100;												// 16bit number to limit upcount; 20kHz

	//PWM output to AIN1_DRV1 & AIN2_DRV1
	P1DIR |= BIT4 + BIT5;// + BIT0 + BIT1;
	P1SEL0 &= ~(BIT4 + BIT5);// + BIT0 + BIT1;						VICEDIT
	P1SEL1 &= ~(BIT4 + BIT5);// + BIT0 + BIT1);

	//PWM output to BIN1_DRV1 & BIN2_DRV1
	P3DIR |= BIT4 + BIT5;// + BIT0 + BIT1;
	P3SEL0 &= ~(BIT4 + BIT5);// + BIT0 + BIT1;						VICEDIT
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
		volatile unsigned int i = 10000;                          // SW Delay
		do i--;
		while (i != 0);

	}

}
