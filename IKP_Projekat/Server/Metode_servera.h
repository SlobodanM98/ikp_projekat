#pragma once

#define SERVER_SLEEP_TIME 1000

int Selekt(SOCKET *socket) {
	FD_SET set;
	timeval timeVal;

	FD_ZERO(&set);

	FD_SET(*socket, &set);

	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;

	return select(0, &set, NULL, NULL, &timeVal);
}


bool InitializeWindowsSockets()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

int PrimiPoruku(SOCKET *socket, char *poruka, int duzinaPoruke, bool* ugasi, bool integrityUpdate) {
	int primljeniBajtovi = 0;
	char recvbuf[100];
	do {
		int iResult = Selekt(socket);

		if (iResult == SOCKET_ERROR)
		{
			fprintf(stderr, "select failed with error: %ld\n", WSAGetLastError());
			break;
		}

		if (iResult == 0)
		{
			if (ugasi != NULL) {
				if (_kbhit()) {
					char c = _getch();
					if (c == 'q') {
						*ugasi = true;
						break;
					}
				}
				Sleep(SERVER_SLEEP_TIME);
				continue;
			}
		}

		iResult = recv(*socket, recvbuf, duzinaPoruke, 0);
		if (iResult > 0)
		{
			memcpy(poruka + primljeniBajtovi, recvbuf, iResult);
			primljeniBajtovi += iResult;
		}
		else if (iResult == 0)
		{
			if (integrityUpdate == true) {
				return primljeniBajtovi;
			}
			printf("Connection closed.\n");
			closesocket(*socket);
			break;
		}
	} while (primljeniBajtovi < duzinaPoruke);

	return primljeniBajtovi;
}

int PosaljiPoruku(SOCKET *socket, char *poruka, int duzinaPoruke) {
	int poslatiBajtovi = 0;
	do {
		if (poslatiBajtovi == duzinaPoruke) {
			break;
		}
		//char bajt = *(poruka + poslatiBajtovi);
		int iResult = send(*socket, poruka + poslatiBajtovi, 1, 0);
		if (iResult == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(*socket);
			return -1;
		}
		poslatiBajtovi++;
	} while (poslatiBajtovi < duzinaPoruke);

	return poslatiBajtovi;
}