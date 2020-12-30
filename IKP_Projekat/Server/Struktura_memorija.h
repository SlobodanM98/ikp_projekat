#pragma once

typedef struct memorija {
	char ime[20];
	char prezime[20];
	int indeks;
	struct memorija *sledeci;
}Memorija;

void Inicijalizacija(Memorija **head) {
	*head = NULL;
}

void Dodaj(Memorija **glava, char* ime, char* prezime, int indeks) {
	Memorija *novi;
	novi = (Memorija*)malloc(sizeof(Memorija));
	strcpy(novi->ime, ime);
	strcpy(novi->prezime, prezime);
	novi->indeks = indeks;

	if (*glava == NULL) {
		novi->sledeci = NULL;
		*glava = novi;
		return;
	}
	novi->sledeci = *glava;
	*glava = novi;
}

int BrojClanova(Memorija *glava) {
	int broj = 0;
	Memorija *trenutni = glava;
	while (trenutni != NULL) {
		broj++;
		trenutni = trenutni->sledeci;
	}
	return broj;
}