// Lab 2 of Mech 423
// Eric Moller, February 2014

#include "CircularBuffer.h"


int circularBuffer[maxBufferSize];
unsigned int bufferSize = 0;
unsigned int writeIndex = 0;
unsigned int readIndex = 0;


bool push(int* data) {
	circularBuffer[writeIndex] = *data;
	writeIndex++;
	if (writeIndex >= maxBufferSize)
		writeIndex -= maxBufferSize;
	// If the buffer is full, overwrite the last element and update the read index
	if (bufferFull()) {
		readIndex++;
		if (readIndex >= maxBufferSize)
			readIndex -= maxBufferSize;
		return true;
	}
	else {
		bufferSize++;
		return false;
	}
}

bool pop(int *data) {
	if (bufferEmpty()) {
		return true;
	}
	else {
		*data = circularBuffer[readIndex];
		bufferSize--;
		readIndex++;
		if (readIndex >= maxBufferSize)
			readIndex -= maxBufferSize;
		return false;
	}
}

// Like pop(), but does not remove the element from the buffer
bool sniff(int *data) {
	if (bufferEmpty()) {
		return true;
	}
	else {
		*data = circularBuffer[readIndex];
		return false;
	}
}

void flush() {
	bufferSize = 0;
	writeIndex = 0;
	readIndex = 0;
}

bool bufferFull() {
	return (bufferSize == maxBufferSize);
}

bool bufferEmpty() {
	return (bufferSize == 0);
}

unsigned int getBufferSize() {
	return bufferSize;
}


int getAverage() {
	int i;
	long sum = 0;

	if (bufferFull()) {
		for (i=0; i<maxBufferSize; i++) {
			sum += circularBuffer[i];
		}
		return sum / maxBufferSize;
	}
	// Case: buffer data doesn't wrap around last array element
	else if(readIndex < writeIndex) {
		for (i=readIndex; i<writeIndex; i++) {
			sum += circularBuffer[i];
		}
		return sum / bufferSize;
	}
	// Case: buffer data wraps around last array element
	else if (readIndex > writeIndex) {
		for (i=readIndex; i<maxBufferSize; i++) {
			sum += circularBuffer[i];
		}
		for (i=0; i<writeIndex; i++) {
			sum += circularBuffer[i];
		}
		return sum / bufferSize;
	}
	// Case: buffer is size 0
	else {
		return 0;
	}


}
