#pragma once

#define RING_SIZE 20
#define MESSAGEBOX_SIZE (4 + 4 + 20 + 20 + 1)


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
	return &(apBuffer->data[index][0]);
}
void ringBufPutChar(RingBuffer *apBuffer, const char  *c, int velicinaPoruke) {
	memcpy(apBuffer->data[apBuffer->tail], c, velicinaPoruke);
	apBuffer->tail = (apBuffer->tail + 1) % RING_SIZE;
}