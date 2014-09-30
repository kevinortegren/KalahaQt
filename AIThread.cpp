#include "AIThread.h"
#include <iostream>
#include <fstream>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>

void AIThread::process()
{
	QHostAddress ip(QHostAddress::LocalHost);
	socket.connectToHost(ip, port);
}

void AIThread::init( int port, int time )
{
	this->port = port;
	this->time = time;

	connect(&socket, SIGNAL(hostFound()), this, SLOT(HostFound()));
	connect(&socket, SIGNAL(connected()), this, SLOT(ConnectionSuccess()));
	connect(&socket, SIGNAL(disconnected()), this, SLOT(ConnectionDropped()));
	connect(&socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(ConnectionError(QAbstractSocket::SocketError)));
}

void AIThread::HostFound()
{
	std::cout << "Host found!" << std::endl;
}

void AIThread::ConnectionSuccess()
{
	std::cout << "Connection successfull!" << std::endl;

	socket.write("HELLO\n");
	while(!socket.waitForReadyRead()){}
	QByteArray cmd = socket.readAll();
	QString str = cmd.split(' ')[1];
	player = str.toInt();

	while (running)
	{
		//Check if game has ended. No need to change this.
		if(!CheckWinner())
			break;

		//Check if it is my turn. If so, do a move
		socket.write("PLAYER\n");
		while(!socket.waitForReadyRead()){}
		str = socket.readAll();
		if (str != "ERROR GAME_FULL")
		{
			int nextPlayer = str.toInt();

			if(nextPlayer == player)
			{
				socket.write("BOARD\n");
				while(!socket.waitForReadyRead()){}
				QString curBoard = socket.readAll();
				bool validMove = false;
				while (!validMove)
				{
					//long startT = System.currentTimeMillis();
					//This is the call to the function for making a move.
					//You only need to change the contents in the getMove()
					//function.
					//GameState currentBoard = new GameState(currentBoardStr);
					int cMove = MakeMove(curBoard);

					//Timer stuff
					//long tot = System.currentTimeMillis() - startT;
					//double e = (double)tot / (double)1000;
					QString moveStr = "MOVE " + QString::number(cMove) + " " + QString::number(player) + "\n";
					socket.write(moveStr.toStdString().c_str());
					while(!socket.waitForReadyRead()){}
					str = socket.readAll();
					if (!str.startsWith("ERROR"))
					{
						validMove = true;
						std::cout << "Made move " +  std::to_string(cMove) << std::endl;
						std::cout << "Nodes visited: " +  std::to_string(nodesVisited) << std::endl;
					}
				}
			}
		}
	}

	socket.disconnectFromHost();
	emit finished();
}

void AIThread::ConnectionDropped()
{
	std::cout << "Disconnected from host" << std::endl;
}

void AIThread::ConnectionError( QAbstractSocket::SocketError socketError )
{
	std::cout << socket.errorString().toStdString() << std::endl;
	socket.disconnectFromHost();
}

bool AIThread::CheckWinner()
{
	socket.write("WINNER\n");
	while(!socket.waitForReadyRead()){}
	QString str = socket.readAll();
	if(str[0] == '1' || str[0] == '2') 
	{
		int w = str.toInt();
		if (w == player)
		{
			std::cout << "I WIN :)" << std::endl;
		}
		else
		{
			std::cout << "I LOSE :(" << std::endl;
		}
		running = false;
		return false;
	}
	else if(str[0] == '0')
	{
		std::cout << "DRAW :|" << std::endl;

		running = false;
		return false;
	}

	return true;
}

int AIThread::MakeMove( const QString& board )
{
	//Get string list from board string
	QStringList boardState = board.split(';');
	//Create and setup Node struct
	Node rootNode;
	for(int i = 0; i < 14; ++i)
	{
		rootNode.boardState[i] = boardState[i].toInt();
	}
	rootNode.alpha = -9999;
	rootNode.beta = 9999;
	rootNode.maxiPlayer = player;
	rootNode.depth = 0;

	//Start depth
	globalDepth = 1;
	short val = -9999;
	int theUltimateMove = 0;
	nodesVisited = 0;
	short depthIncr[] = {3, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1};

	QElapsedTimer timer;
	timer.start();

	for(int k = 0; k < 10; ++k)//TODO timer based
	{
		
		QFuture<short> threadWatchers[6];
		//Create threads and fire away!
		for(int a = 0; a < 6; ++a)
		{
			//Check if legal move
			if (rootNode.boardState[a+1] != 0) 
			{
				Node tempNode = MoveAmbo(rootNode, a);
				threadWatchers[a] = QtConcurrent::run(this, &AIThread::AlphaBetaRecursive, tempNode); 
			}
		}

		//Loop for waiting on threads and results
		for(int a = 0; a < 6; ++a)
		{
			if (rootNode.boardState[a+1] != 0) 
			{
				short retVal = threadWatchers[a].result();
				//std::cout << "Eval value " << retVal << " from thread "<< a << std::endl;
				if(retVal > val)
				{
					val = retVal;
					theUltimateMove = a+1;
				}
			}
		}
		
		globalDepth += depthIncr[k]; 
	}
	
	std::cout << "Time elapsed to search level "<< globalDepth << ": " << timer.elapsed() << std::endl;
	return theUltimateMove;
}

short AIThread::AlphaBetaRecursive(Node node)
{
	Node tempNode;
	nodesVisited++;
	if(++node.depth == globalDepth)
	{
		return EvalFunc(node);
	}

	if(node.maxiPlayer == player)
	{
		for(int i = 0; i < 6; ++i)
		{
			//Check if legal move
			if (node.boardState[i+1] != 0) //TOTOTOTO
			{
				tempNode = MoveAmbo(node, i);
				tempNode.alpha = std::max(tempNode.alpha, AlphaBetaRecursive(tempNode));
				if(node.beta <= tempNode.alpha)
					break;
			}
		}
		return tempNode.alpha;
	}
	else
	{
		for(int i = 0; i < 6; ++i)
		{
			//Check if legal move
			if (node.boardState[i+1] != 0) //TOTOTOTO
			{
				tempNode = MoveAmbo(node, i);
				tempNode.beta = std::min(tempNode.beta, AlphaBetaRecursive(tempNode));
				if(node.beta <= tempNode.alpha)
					break;
			}
		}
		return tempNode.beta;
	}
}

short AIThread::EvalFunc( const Node& node )
{
	short house = node.maxiPlayer ? START_S : START_N;
	short specialScore = 0;
	for(short i = 0; i < 6; ++i)
	{
		if(node.boardState[house + i] == 13)
			specialScore += 12 + node.boardState[GetOppositeAmbo(house + i)] * 4;
	}

	short southHouse = node.boardState[HOUSE_S];
	short northHouse = node.boardState[HOUSE_N];

	return player ? ((southHouse - northHouse) * 4 + specialScore) : ((northHouse - southHouse) * 4 - specialScore); 
}

AIThread::Node AIThread::MoveAmbo(const Node& boardState, int ambo )
{
	Node newBoardState = boardState;

	int curPlayer = boardState.maxiPlayer;
	ambo = curPlayer ? START_S + ambo : START_N + ambo;

	//Pickup seeds
	int seeds = newBoardState.boardState[ambo];
	newBoardState.boardState[ambo] = 0;
	bool lastIsHouse = false;
	//Get curr player 
	

	//Sow seeds
	while (seeds > 0)
	{
		//Take a step
		ambo++;
		if (ambo >= 14) 
			ambo = 0;

		if ( (curPlayer == 1 && ambo == HOUSE_N) || (curPlayer == 2 && ambo == HOUSE_S) )
		{
			//Don't sow in opponents house
		}
		else
		{
			//Sow a seed
			newBoardState.boardState[ambo]++;
			seeds--;
		}

		//Check special cases for last seed
		if (seeds == 0)
		{
			//Check for extra move
			if (curPlayer == 1 && ambo == HOUSE_S) 
				lastIsHouse = true;
			if (curPlayer == 2 && ambo == HOUSE_N) 
				lastIsHouse = true;

			//Check capture
			bool capture = false;
			if (newBoardState.boardState[ambo] == 1)
			{
				if (curPlayer == 1)
				{
					if (ambo >= START_S && ambo <= END_S) 
						capture = true;
				}
				if (curPlayer == 2)
				{
					if (ambo >= START_N && ambo <= END_N) 
						capture = true;
				}
			}

			//Possible capture of opponent's seeds
			if (capture)
			{
				int oi = GetOppositeAmbo(ambo);
				if (newBoardState.boardState[oi] > 0)
				{
					if (curPlayer == 1)
					{
						newBoardState.boardState[HOUSE_S] += newBoardState.boardState[ambo] + newBoardState.boardState[oi];
					}
					else if (curPlayer == 2)
					{ 
						newBoardState.boardState[HOUSE_N] += newBoardState.boardState[ambo] + newBoardState.boardState[oi];
					}

					newBoardState.boardState[ambo] = 0;
					newBoardState.boardState[oi] = 0;
				}
			}
		}
	}

	if (!lastIsHouse)
	{
		//Toggle player 
		if (curPlayer == 1) 
			newBoardState.maxiPlayer = 2;
		else 
			newBoardState.maxiPlayer = 1;
	}

	//Call to update game state in
	//case any player won.
	seeds = 0;

	//Player 1 - South
	for (int i = START_S; i <= END_S; i++)
	{
		seeds += newBoardState.boardState[i];
	}
	if (seeds == 0)
	{
		//Gather opponents seeds (if any)
		//Rule 6
		for (int i = START_N; i <= END_N; i++)
		{
			if (newBoardState.boardState[i] > 0)
			{
				newBoardState.boardState[HOUSE_N] += newBoardState.boardState[i];
				newBoardState.boardState[i] = 0;
			}
		}
	}

	//Player 2 - North
	seeds = 0;
	for (int i = START_N; i <= END_N; i++)
	{
		seeds += newBoardState.boardState[i];
	}
	if (seeds == 0)
	{
		//Gather opponents seeds (if any)
		//Rule 6
		for (int i = START_S; i <= END_S; i++)
		{
			if (newBoardState.boardState[i] > 0)
			{
				newBoardState.boardState[HOUSE_S] += newBoardState.boardState[i];
				newBoardState.boardState[i] = 0;
			}
		}
	}

	return newBoardState;
}

int AIThread::GetOppositeAmbo( int ambo )
{
	if (ambo == START_S) return END_N;
	if (ambo == START_S+1) return END_N-1;
	if (ambo == START_S+2) return END_N-2;
	if (ambo == START_S+3) return END_N-3;
	if (ambo == START_S+4) return END_N-4;
	if (ambo == START_S+5) return END_N-5;
	if (ambo == START_N) return END_S;
	if (ambo == START_N+1) return END_S-1;
	if (ambo == START_N+2) return END_S-2;
	if (ambo == START_N+3) return END_S-3;
	if (ambo == START_N+4) return END_S-4;
	if (ambo == START_N+5) return END_S-5;

	return -1;
}

void AIThread::ToggleNextPlayer()
{
	if (player == 1) 
		player = 2;
	else 
		player = 1;
}

