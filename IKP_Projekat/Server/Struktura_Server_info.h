#pragma once

typedef struct server_info {
	char ipAdresa[20];
	int port;
	char messageBox[20];
	HANDLE primljenOdgovor;
	LPDWORD serverID;
	HANDLE hServerKonekcija;
	struct server_info *sledeci;
}Server_info;

void Inicijalizacija(Server_info **head) {
	*head = NULL;
}

void Dodaj(Server_info **glava, char *novaAdresa, int noviPort, HANDLE semafor, LPDWORD serverID, HANDLE hServerKonekcija) {
	Server_info *novi;
	novi = (Server_info*)malloc(sizeof(Server_info));
	strcpy(novi->ipAdresa, novaAdresa);
	novi->port = noviPort;
	novi->primljenOdgovor = semafor;
	novi->serverID = serverID;
	novi->hServerKonekcija = hServerKonekcija;
	if (*glava == NULL) {
		novi->sledeci = NULL;
		*glava = novi;
		return;
	}
	novi->sledeci = *glava;
	*glava = novi;
}

int BrojClanova(Server_info *glava) {
	int broj = 0;
	Server_info *trenutni = glava;
	while (trenutni != NULL) {
		broj++;
		trenutni = trenutni->sledeci;
	}
	return broj;
}