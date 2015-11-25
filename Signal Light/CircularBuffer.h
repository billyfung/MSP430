// Lab 2 of Mech 423
// Eric Moller, February 2014

#include "Boolean.h"
#include <msp430f2274.h>

#define maxBufferSize 12

bool push(int* data);
bool pop(int* data);
bool sniff(int* data);
void flush();
bool bufferFull();
bool bufferEmpty();

unsigned int getBufferSize();

int getAverage();
