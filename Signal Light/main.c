#include <msp430f2012.h>
#include "Boolean.h"
#include "CircularBuffer.h"

// Bike signal system - Mech 423
// Eric Moller, Billy Fung, March 2014
//

// State definitions
#define NO_TURN 0
#define LEFT_TURN 1
#define RIGHT_TURN 2

#define NO_BRAKES 10
#define BRAKES 11

#define COMPASS_A 20
#define COMPASS_B 21

#define AUTO_CANCEL_DISABLED 30
#define AUTO_CANCEL_INITIALIZING 31
#define AUTO_CANCEL_READY 32
#define AUTO_CANCEL_ACTIVE 33


// Port 1 pins
#define O1_DEBUG_LIGHT BIT0
#define I1_LEFT_BUTTON BIT1
#define O1_LEFT_LIGHT BIT2
#define I1_COMPASS_A BIT3
#define I1_COMPASS_B BIT4
#define I1_LEFT_BRAKE BIT5
#define O1_BRAKE_LIGHT BIT6
#define I1_RIGHT_BRAKE BIT7

// Port 2 pins
#define O2_RIGHT_LIGHT BIT6
#define I2_RIGHT_BUTTON BIT7

// Constants
#define MIN_TURN_VALUE 20

volatile int turnState;
volatile int brakeState;
volatile int compassState;
volatile int autoCancelState;

int genericCompassValue;

int averagedCompassValueA;
int averagedCompassValueB;

volatile int startCompassValueA;
volatile int startCompassValueB;

void cancelTurn();
void leftTurn();
void rightTurn();
void setBrakes();
void cancelBrakes();
int brakesActive();
void startAnalogRead();


int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    // Clock Settings -----------------
    BCSCTL1 = XT2OFF;													// Disable XT2
    BCSCTL2 = SELM1 + SELM0 + DIVM1 + DIVM0 + SELS + DIVS1 + DIVS0; 	// Set all clocks to use VLOCLK / 8
    BCSCTL3 = LFXT1S1;

    P1SEL = 0X00;
    P2SEL = 0X00;

    // Port Settings ------------------------
	P1DIR = O1_LEFT_LIGHT + O1_BRAKE_LIGHT + O1_DEBUG_LIGHT;			// Set outputs
	P1REN = I1_LEFT_BRAKE + I1_RIGHT_BRAKE + I1_LEFT_BUTTON;			// Enable pull up resistors
	P1OUT = I1_LEFT_BRAKE + I1_RIGHT_BRAKE + I1_LEFT_BUTTON;			// Set pull up resistors
	P1OUT &= ~(O1_LEFT_LIGHT + O1_BRAKE_LIGHT); 						// Turn off lights
	P1IE = I1_LEFT_BUTTON + I1_LEFT_BRAKE + I1_RIGHT_BRAKE;				// Set interrupt sources
	P1IES &= ~(I1_LEFT_BUTTON + I1_LEFT_BRAKE + I1_RIGHT_BRAKE);		// Interrupt on rising edge

	P2DIR = O2_RIGHT_LIGHT;												// Set outputs
	P2REN = I2_RIGHT_BUTTON;											// Enable pull up resistors
	P2OUT = I2_RIGHT_BUTTON;											// Enable pull up resistors
	P2OUT &= ~(O2_RIGHT_LIGHT);											// Turn off lights
	P2IE = I2_RIGHT_BUTTON;												// Set interrupt sources
	P2IES &= ~(I2_RIGHT_BUTTON); 										// Interrupt on rising edge.

	// Timer A settings
	TACCR0 = 600;
	TACCR1 = 300;
	TACTL = TASSEL1;						// Use SMCLK
	TACCTL1 = OUTMOD2 + OUTMOD1 + OUTMOD0 + CCIE;	// Reset/Set mode, enable interrupts


	// ADC settings
	ADC10CTL0 &= ~(ENC);		// Disable ADC before setting up
	// 8 cycle sample/hold, use 2.5V internal reference, use slower sampling rate, reference burst (power save)
	ADC10CTL0 = SREF0 + ADC10SHT0 + ADC10SR + REFBURST + REF2_5V + REFON + ADC10ON;// + ADC10IE;
	ADC10CTL1 = ADC10SSEL1 + ADC10SSEL0;		// Run off SMCLK
	ADC10AE0 = I1_COMPASS_A + I1_COMPASS_B;


	turnState = NO_TURN;
	brakeState = NO_BRAKES;
	compassState = COMPASS_A;
	autoCancelState = AUTO_CANCEL_DISABLED;


	_EINT();  // Global enable interrupts
	LPM4;

	while (1) {
		// Power down logic
		if (brakeState == NO_BRAKES && turnState == NO_TURN) {
			// Check that we haven't missed a brake signal before going to sleep
			if (brakesActive()) {
				//setBrakes();
			}
			else {
				_EINT();
				LPM4;		// Enter low power mode until a brake or turn event
			}
		}

		// Braking logic
		if (brakeState == BRAKES) {
			if(brakesActive()) {
				// currently braking, do nothing
			}
			else {
				// stop braking
				cancelBrakes();
			}
		}
		// Check that brakes aren't active, and turn on light if necessary
		else {
			if(brakesActive())
				setBrakes();
		}

		// Entry condition: in a turn state AND ADC not busy AND no ADC interrupts pending
		if (turnState != NO_TURN && !(ADC10CTL1 & ADC10BUSY) && !(ADC10CTL0 & ADC10IFG)) {
			// Start the ADC
			flush();
			startAnalogRead();
		}


		// Entry condition: ADC data ready AND in a turn state
		if ((ADC10CTL0 & ADC10IFG) && turnState != NO_TURN) {

			// Put data in buffer
			genericCompassValue = ADC10MEM;
			// Clear interrupt flag before proceeding
			ADC10CTL0 &= ~(ADC10IFG);
			bool bufferOverflow = push(&genericCompassValue);

			if (bufferOverflow) {
				switch(compassState) {
					case COMPASS_A:
						averagedCompassValueA = getAverage();
						compassState = COMPASS_B;
						flush();
						startAnalogRead();
						break;
					case COMPASS_B:
						averagedCompassValueB = getAverage();
						compassState = COMPASS_A;
						flush();
						startAnalogRead();
						break;
				}

				// We need to pass through two buffer overflows to get current compass data from both
				// channels. On the first pass we set the state to initializing, and on the second pass
				// we set the state to ready.
				switch (autoCancelState) {
					case AUTO_CANCEL_DISABLED:
						autoCancelState = AUTO_CANCEL_INITIALIZING;
					case AUTO_CANCEL_INITIALIZING:
						autoCancelState = AUTO_CANCEL_READY;
						break;
				}
			}
			// Keep polling compass
			else {
				startAnalogRead();
			}


			// Debug test  B-670
			if(averagedCompassValueA > 650)
				P1OUT |= O1_DEBUG_LIGHT;
			else
				P1OUT &= ~O1_DEBUG_LIGHT;
			// end debug
		}


		// Entry condition: in a turn state AND auto cancel is active
		// In here we have auto-cancel logic
		if (turnState != NO_TURN && autoCancelState == AUTO_CANCEL_ACTIVE) {
			int absTurnA = startCompassValueA - averagedCompassValueA;
			int absTurnB = startCompassValueB - averagedCompassValueB;

			absTurnA = (absTurnA >= 0) ? absTurnA : -absTurnA;
			absTurnB = (absTurnB >= 0) ? absTurnB : -absTurnB;

			if (absTurnA + absTurnB > MIN_TURN_VALUE) {
				cancelTurn();
				autoCancelState = AUTO_CANCEL_DISABLED;
			}
		}
	}

	return 0;
}



#pragma vector=PORT1_VECTOR
__interrupt void interrupt1() {

	if (P1IFG & I1_LEFT_BUTTON) {
		switch (turnState) {
		case NO_TURN:
			leftTurn();
			autoCancelState = AUTO_CANCEL_DISABLED;
			break;
		case LEFT_TURN:
			cancelTurn();
			break;
		case RIGHT_TURN:
			cancelTurn();
			break;
		}

		P1IFG &= ~I1_LEFT_BUTTON;				// Clear interrupt flag
	}


	if (P1IFG & I1_LEFT_BRAKE || P1IFG & I1_RIGHT_BRAKE) {
		switch (brakeState) {
		case NO_BRAKES:
			setBrakes();
			break;
		}

		P1IFG &= ~(I1_LEFT_BRAKE + I1_RIGHT_BRAKE);				// Clear interrupt flag
	}


	LPM4_EXIT;
}


#pragma vector=PORT2_VECTOR
__interrupt void interrupt2() {

	if (P2IFG & I2_RIGHT_BUTTON) {
		switch (turnState) {
		case NO_TURN:
			rightTurn();
			autoCancelState = AUTO_CANCEL_DISABLED;
			break;
		case LEFT_TURN:
			cancelTurn();
			break;
		case RIGHT_TURN:
			cancelTurn();
			break;
		}

		P2IFG &= ~I2_RIGHT_BUTTON;				// Clear interrupt flag
	}

	LPM4_EXIT;
}


#pragma vector=TIMERA1_VECTOR
__interrupt void pollCompassAverages() {

	switch(autoCancelState) {
		case AUTO_CANCEL_READY:
			startCompassValueA = averagedCompassValueA;
			startCompassValueB = averagedCompassValueB;
			autoCancelState = AUTO_CANCEL_ACTIVE;
			break;
		case AUTO_CANCEL_ACTIVE:
			break;	}

	TACCTL1 &= ~(CCIFG);
}

void cancelTurn() {
	// Turn OFF blinkers
	TACTL &= ~(MC0);					// Stop timer
	P1SEL &= ~(O1_LEFT_LIGHT);		// Get pins out of PWM mode
	P2SEL &= ~(O2_RIGHT_LIGHT);		// Get pins out of PWM mode
	P1OUT &= ~(O1_LEFT_LIGHT);		// Set pins low
	P2OUT &= ~(O2_RIGHT_LIGHT);
	turnState = NO_TURN;
}

void leftTurn() {
	// Turn on left blinker
	P1SEL |= O1_LEFT_LIGHT;
	TACTL |= MC0;					// Start timer
	turnState = LEFT_TURN;
}

void rightTurn() {
	// Turn on right blinker
	P2SEL |= O2_RIGHT_LIGHT;
	TACTL |= MC0;					// Start timer
	turnState = RIGHT_TURN;
}

void setBrakes() {
	// Turn on brake signal
	P1OUT |= O1_BRAKE_LIGHT;
	brakeState = BRAKES;
}

void cancelBrakes() {
	// Turn off brake signal
	P1OUT &= ~(O1_BRAKE_LIGHT);
	brakeState = NO_BRAKES;
}

int brakesActive() {
	// Returns 1 if either brake is being applied, and 0 if no brakes are being applied

	// Normally high logic
	if ((P1IN & I1_LEFT_BRAKE) && (P1IN & I1_RIGHT_BRAKE))
		return 0;
	else
		return 1;

	/*
	// Normally low logic
	if (P1IN & (LEFT_BRAKE + RIGHT_BRAKE))
		return 1;
	else
		return 0;
	*/
}


// Sets up ADC to read a compass axis, pass in one of the state constants
// Function sets ADC state for interrupt vector to use
// Read ADC value through interrupt vector
// Assumes ADC has been properly set up in main.
void startAnalogRead() {

	ADC10CTL0 &= ~(ENC);								// Clear the enable bit, so other bits can be modified
	ADC10CTL1 &= ~(INCH3 + INCH2 + INCH1 + INCH0);		// Clear selection bits

	switch(compassState) {
		case COMPASS_A:
			ADC10CTL1 |= INCH1 + INCH0;
			break;
		case COMPASS_B:
			ADC10CTL1 |= INCH2;
			break;
		}

	ADC10CTL0 &= ~(ADC10IFG);							// Kill any pending interrupts
	ADC10CTL0 |= ENC + ADC10SC; 						// Start conversion
}
