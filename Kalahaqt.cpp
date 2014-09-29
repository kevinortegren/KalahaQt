#include "Kalahaqt.h"
#include "ui_ConnectionDialog.h"
#include <QtConcurrent/QtConcurrent>
#include <iostream>
#include <fstream>

KalahaQt::KalahaQt(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	connect(ui.pushButton_connect, SIGNAL(clicked()), this, SLOT(MenuConnect()));
	connect(ui.pushButton_disconnect, SIGNAL(clicked()), this, SLOT(MenuExit()));

	std::cout << "Welcome! Kalaha AI made by Joel and Kevin" << std::endl;
}

KalahaQt::~KalahaQt()
{
}

void KalahaQt::MenuConnect()
{
	Ui::ConnectDialog cdui;
	QDialog dial;
	cdui.setupUi(&dial);

	if(dial.exec())
	{
		SetUpWorker();

		QString portString = QString::number(cdui.spinBox_port->value());
		//QString timeString = QString::number(cdui.spinBox_time->value);
		std::cout << "Connecting to localhost:" << portString.toStdString() << "..." << std::endl;

		aiWorker->init(cdui.spinBox_port->value(), cdui.spinBox_time->value());
		aiWorker->moveToThread(qthread);
		qthread->start();

		ui.pushButton_connect->setDisabled(true);

	}
}

void KalahaQt::MenuExit()
{
	QApplication::quit();
}

void KalahaQt::AIThreadDone()
{
	ui.pushButton_connect->setDisabled(false);
	std::cout << "AI Thread done" << std::endl;
}

void KalahaQt::SetUpWorker()
{
	qthread = new QThread();
	aiWorker = new AIThread();

	//Setup signals and slots
	
	connect(aiWorker, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
	connect(qthread, SIGNAL(started()), aiWorker, SLOT(process()));
	connect(aiWorker, SIGNAL(finished()), qthread, SLOT(quit()));
	connect(aiWorker, SIGNAL(finished()), aiWorker, SLOT(deleteLater()));
	connect(qthread, SIGNAL(finished()), qthread, SLOT(deleteLater()));
	connect(aiWorker, SIGNAL(finished()), this, SLOT(AIThreadDone()));
}
