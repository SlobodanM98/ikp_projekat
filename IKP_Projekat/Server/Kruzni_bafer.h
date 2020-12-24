#pragma once

#define RING_SIZE 5
#define MESSAGEBOX_SIZE 20


// Kruzni bafer - FIFO
struct RingBuffer {
	unsigned int tail;
	unsigned int head;
	char data[RING_SIZE][MESSAGEBOX_SIZE];
};
// Operacije za rad sa kruznim baferom
char* ringBufGetChar(RingBuffer *apBuffer) {
	int index;
	index = apBuffer->head;
	apBuffer->head = (apBuffer->head + 1) % RING_SIZE;
	return apBuffer->data[index];
}
void ringBufPutChar(RingBuffer *apBuffer, const char  *c) {
	strcpy_s(apBuffer->data[apBuffer->tail], c);
	apBuffer->tail = (apBuffer->tail + 1) % RING_SIZE;
}