#ifndef KALAHAQT_H
#define KALAHAQT_H

#include <QtWidgets/QMainWindow>
#include "ui_Kalahaqt.h"
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QThread>
#include "AIThread.h"

class KalahaQt : public QMainWindow
{
	Q_OBJECT

public:
	KalahaQt(QWidget *parent = 0);
	~KalahaQt();

private:
	Ui::KalahaQtClass ui;
	QThread* qthread;
	AIThread* aiWorker;
	void SetUpWorker();

private slots:
	void MenuConnect();
	void MenuExit();
	void AIThreadDone();
};

#endif // KALAHAQT_H
