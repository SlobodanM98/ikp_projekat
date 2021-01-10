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

void Obrisi(Memorija **glava, int indeks) {
	Memorija *predhodni = *glava;
	Memorija *temp = *glava;

	if (temp != NULL && temp->indeks == indeks) {
		*glava = temp->sledeci;
		free(temp);
		return;
	}

	while (temp != NULL && temp->indeks != indeks)
	{
		predhodni = temp;
		temp = temp->sledeci;
	}

	if (temp == NULL)
		return;

	predhodni->sledeci = temp->sledeci;
	free(temp);
}

void Izmeni(Memorija **glava, char* ime, char* prezime, int indeks) {
	Memorija *temp = *glava;
	while (temp != NULL) {
		if (temp->indeks == indeks) {
			strcpy(temp->ime, ime);
			strcpy(temp->prezime, prezime);
			return;
		}
		temp = temp->sledeci;
	}
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