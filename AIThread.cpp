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
	this->time = time - 1;

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
	running = true;
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
					int cMove = MakeMove(curBoard);

					QString moveStr = "MOVE " + QString::number(cMove) + " " + QString::number(player) + "\n";
					socket.write(moveStr.toStdString().c_str());
					while(!socket.waitForReadyRead()){}
					str = socket.readAll();
					if (!str.startsWith("ERROR"))
					{
						validMove = true;
						std::cout << "Made move " +  std::to_string(cMove) << std::endl;
						//std::cout << "Nodes visited: " +  std::to_string(nodesVisited) << std::endl;
					}
					else
					{
						std::cout << str.toStdString().c_str() << std::endl; 
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
	rootNode.maxiPlayer = player;
	rootNode.depth = 0;

	//Start depth
	globalDepth = 0;
	
	int theUltimateMove = 0;
	nodesVisited = 0;

	playerStart = (player == 1) ? START_S : START_N;
	enemyStart = (player == 2) ? START_S : START_N;
	playerHouse = (player == 1) ? HOUSE_S : HOUSE_N;
	enemyHouse = (player == 2) ? HOUSE_S : HOUSE_N;
	playerEnd = (player == 1) ? END_S : END_N;
	enemyEnd = (player == 2) ? END_S : END_N;

	std::vector<short> depthIncr;
	depthIncr.push_back(10);
	depthIncr.push_back(2);

	short threadCount = 0;
	timer.start();
	exitThread = false;
	short loopCount = 0;

	std::cout << "Thinking..."<< std::endl;

	//Start the main loop!
	while(1)
	{
		//Increase loop by a custom amount for two steps, else increase by one
		if(loopCount < 2)
			globalDepth += depthIncr[loopCount]; 
		else
			globalDepth++;

		//Create 6 handles to threads
		QFuture<short> threadWatchers[6];
		threadCount = 0;

		//Create threads and fire away!
		for(int a = 0; a < 6; ++a)
		{
			//Check if legal move
			if (rootNode.boardState[playerStart + a] != 0) 
			{
				Node childNode = MoveAmbo(rootNode, a); //Create child node for each start move 
				threadWatchers[a] = QtConcurrent::run(this, &AIThread::AlphaBetaRecursive, childNode, -9999, 9999); //This part is asynchronous and will continue directly
				threadCount++; //Count threads to later know if all threads are done
			}
		}

		//Timer / wait for threads to finish part
		//Loop here until threads are done or time is out!
		int counter = 0;
		while (1)
		{
			//Do some arbitrary work to keep the CPU busy instead of running the expensive timer.elapsed() every iteration
			if (counter == 1000000)
			{
				counter = 0;
				//Check time, in milliseconds. The time entered at the begining of the program i reduced by 1ms to allow the program to aactually finish all tasks in time
				if (timer.elapsed() > time)
				{
					//Set exit thread flag, which is read by all the threads. 
					exitThread = true;
					break;
				}
				else
				{
					//If there's still time we check how many threads are finished
					short threadsFinished = 0;
					for(int a = 0; a < 6; ++a)
					{
						if (rootNode.boardState[playerStart + a] != 0) 
						{
							if(threadWatchers[a].isFinished())
							{
								threadsFinished++;
							}
						}
					}
					//If all threads are done, leave loop
					if(threadsFinished == threadCount)
					{
						break;
					}
				}
			}

			counter++;
		}

		short val = -9999;
		//Loop for retrieving the results from the threads. All threads are done when this code is executed.
		for(int a = 0; a < 6; ++a)
		{
			if (rootNode.boardState[playerStart + a] != 0) 
			{
				//Get evaluation result from thread.
				short retVal = threadWatchers[a].result();
				//Only use return value if all threads reached tree depth. 
				if(retVal > val && !exitThread)
				{
					val = retVal;
					theUltimateMove = a+1; //This is the move!
				}
			}
		}

		//If exitThread is true, means that the iterative deepening iteration is done and a move has been chosen! 
		if(exitThread)
		{
			break;
		}

		loopCount++;
	}
	
	std::cout << "Ended on level "<< globalDepth << " in " << timer.elapsed() << "ms"<< std::endl;
	return theUltimateMove;
}

short AIThread::AlphaBetaRecursive(Node parentNode, short alpha, short beta)
{
	if(exitThread)
		return EvalFunc(parentNode);
	//Check for terminal states and evaluate them
	if(parentNode.boardState[playerHouse] > 36)
		return EvalFunc(parentNode) + 100;
	else if (parentNode.boardState[enemyHouse] > 36)
		return EvalFunc(parentNode) - 100;
	else if (parentNode.boardState[enemyHouse] == 36 && parentNode.boardState[playerHouse] == 36)
		return  EvalFunc(parentNode) - 90;

	//Reached iterative deepening depth? Return evaluation
	if(++parentNode.depth == globalDepth)
	{
		return EvalFunc(parentNode);
	}

	Node childNode;
	//nodesVisited++;

	//Maxi-move
	if(parentNode.maxiPlayer == player)
	{
		for(int i = 0; i < 6; ++i)
		{
			//Check if legal move
			if (parentNode.boardState[playerStart + i] != 0) 
			{
				childNode = MoveAmbo(parentNode, i);
				alpha = std::max(alpha, AlphaBetaRecursive(childNode, alpha, beta));
				if(beta <= alpha)
					break;
			}
		}

		return alpha;
	}
	else //Mini-move
	{
		for(int i = 0; i < 6; ++i)
		{
			//Check if legal move
			if (parentNode.boardState[enemyStart + i] != 0) 
			{
				childNode = MoveAmbo(parentNode, i);
				beta = std::min(beta, AlphaBetaRecursive(childNode, alpha, beta));
				if(beta <= alpha)
					break;
			}
		}
		return beta;
	}
}

/************************************************************************/
/* Eval function
	-Check for amboes with 13 seeds in them and add 3 + number of seeds in opposite ambo
	-Check for empty amboes, adds 1
	-Check for amboes with number of seeds that grants the player an extra turn, adds 1
	-Add (Player seeds - enemy seeds)
*/
/************************************************************************/
short AIThread::EvalFunc( const Node& node )
{
	short specialScore = 0;

	
	for(short i = 0; i < 6; ++i)
	{
		if(node.boardState[playerStart + i] == 13)
			specialScore += 3 + node.boardState[GetOppositeAmbo(playerStart + i)];
		if(node.boardState[playerStart + i] == 0)
			specialScore ++;
		if(node.boardState[playerStart + i] == 6 - i)
			specialScore ++;
	}
	
	return (node.boardState[playerHouse] - node.boardState[enemyHouse]) + specialScore; 
}

AIThread::Node AIThread::MoveAmbo(const Node& boardState, int ambo )
{
	Node newBoardState = boardState;

	int curPlayer = boardState.maxiPlayer;
	ambo = (curPlayer == 1) ? START_S + ambo : START_N + ambo;

	//Pickup seeds
	int seeds = newBoardState.boardState[ambo];
	newBoardState.boardState[ambo] = 0;
	//Get curr player 
	
	if(curPlayer == 1)
	{
		//Sow seeds
		while (seeds > 0)
		{
			//Take a step
			ambo++;
			if (ambo >= 14) 
				ambo = 0;

			if ((ambo != HOUSE_N))
			{
				//Sow a seed
				newBoardState.boardState[ambo]++;
				seeds--;
			}
		}

		//Check special cases for last seed

		//Check for extra move
		if (ambo == HOUSE_S) 
			newBoardState.maxiPlayer = 1;
		else
			newBoardState.maxiPlayer = 2;

		//Check capture
		if (newBoardState.boardState[ambo] == 1)
		{
			if (ambo >= START_S && ambo <= END_S) //Capture if true
			{
				int oi = GetOppositeAmbo(ambo);
				if (newBoardState.boardState[oi] > 0)
				{
					newBoardState.boardState[HOUSE_S] += newBoardState.boardState[ambo] + newBoardState.boardState[oi];
					
					newBoardState.boardState[ambo] = 0;
					newBoardState.boardState[oi] = 0;
				}
			}
		}
	}
	else
	{
		//Sow seeds
		while (seeds > 0)
		{
			//Take a step
			ambo++;
			if (ambo >= 14) 
				ambo = 0;

			if (ambo != HOUSE_S)
			{
				newBoardState.boardState[ambo]++;
				seeds--;
			}
		}

		//Check special cases for last seed
		if (ambo == HOUSE_N) 
			newBoardState.maxiPlayer = 2;
		else
			newBoardState.maxiPlayer = 1;

		//Check capture
		if (newBoardState.boardState[ambo] == 1)
		{
			if (ambo >= START_N && ambo <= END_N) 
			{
				int oi = GetOppositeAmbo(ambo);
				if (newBoardState.boardState[oi] > 0)
				{
					newBoardState.boardState[HOUSE_N] += newBoardState.boardState[ambo] + newBoardState.boardState[oi];
					
					newBoardState.boardState[ambo] = 0;
					newBoardState.boardState[oi] = 0;
				}
			}
			
		}
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


