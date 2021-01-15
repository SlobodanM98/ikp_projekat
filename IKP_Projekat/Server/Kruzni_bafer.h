#pragma once

#define RING_SIZE 10
#define MESSAGEBOX_SIZE (1000000)


// Kruzni bafer - FIFO
struct RingBuffer {
	unsigned int tail;
	unsigned int head;
	unsigned int alociranaMemorija;
	unsigned int zauzetaMemorija;
	unsigned int brojDozvoljenihPoruka;
	char *data;
	char *velicinaPrethodnihPoruka;
};
// Operacije za rad sa kruznim baferom
char* ringBufGetChar(RingBuffer *apBuffer) {
	int index;
	index = apBuffer->head;
	apBuffer->head = apBuffer->head + 1;
	int duzinaPrethodnih = *((int*)apBuffer->velicinaPrethodnihPoruka + index);
	return &(*(apBuffer->data + duzinaPrethodnih));
}
void ringBufPutChar(RingBuffer *apBuffer, const char  *c, int velicinaPoruke) {
	memcpy(apBuffer->data + apBuffer->zauzetaMemorija, c, velicinaPoruke);
	memcpy(apBuffer->velicinaPrethodnihPoruka + (apBuffer->tail * sizeof(int)), (char *)&(apBuffer->zauzetaMemorija), sizeof(int));
	apBuffer->zauzetaMemorija += velicinaPoruke;
	apBuffer->tail = apBuffer->tail + 1;
}