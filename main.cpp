#include "Kalahaqt.h"
#include <QtWidgets/QApplication>
#include <windows.h>

int main(int argc, char *argv[])
{
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	QApplication a(argc, argv);
	KalahaQt w;
	w.show();
	return a.exec();
}
