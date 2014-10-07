#pragma once

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <stdint.h>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

class AIThread : public QObject
{
	Q_OBJECT

	enum BoardSlots
	{
		HOUSE_S = 7,		//6
		START_S = 1,		//0
		END_S = 6,			//5
		HOUSE_N = 0,		//13
		START_N = 8,		//7
		END_N = 13,			//12
		NEXT_PLAYER = 14,
	};

	struct Node
	{
		uint8_t boardState[14];
		short depth;
		short maxiPlayer;
		short alpha;
		short beta;
	};

public slots:
	void process();
	void init(int port, int time);
signals:
	void finished();
	void error(QString err);

protected:
	
private:
	QTcpSocket	socket;
	short		player;
	bool		running;
	int			port;
	int			time;
	int			globalDepth;
	int			nodesVisited;
	bool		exitThread;
	QElapsedTimer timer;
	short		playerStart;
	short		enemyStart;
	short		playerHouse;
	short		enemyHouse;

	bool CheckWinner();
	int MakeMove(const QString& board);
	Node MoveAmbo(const Node& boardState, int ambo);
	int GetOppositeAmbo(int ambo);
	short AlphaBetaRecursive(Node node);
	short EvalFunc(const Node& node);

private slots:
	void HostFound();
	void ConnectionSuccess();
	void ConnectionDropped();
	void ConnectionError(QAbstractSocket::SocketError socketError);
};

