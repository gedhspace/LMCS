#pragma once
#include<Windows.h>
#define statusright 40
#define chossadd 2
#define chosscount 5

void gotoxy(int y, int x) {
	COORD pos = { x,y };
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(hOut, pos);
}