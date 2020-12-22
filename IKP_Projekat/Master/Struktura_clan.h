#pragma once


typedef struct clan {
	char ipAdresa[20];
	int port;
	struct clan *sledeci;
}Clan;

void Inicijalizacija(Clan **head) {
	*head = NULL;
}

void Dodaj(Clan **glava, char *novaAdresa, int noviPort) {
	Clan *novi;
	novi = (Clan*)malloc(sizeof(Clan));
	strcpy(novi->ipAdresa, novaAdresa);
	novi->port = noviPort;
	if (*glava == NULL) {
		novi->sledeci = NULL;
		*glava = novi;
		return;
	}
	novi->sledeci = *glava;
	*glava = novi;
}

int BrojClanova(Clan *glava) {
	int broj = 0;
	Clan *trenutni = glava;
	while (trenutni != NULL) {
		broj++;
		trenutni = trenutni->sledeci;
	}
	return broj;
}