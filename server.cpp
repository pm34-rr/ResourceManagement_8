/*!
 * \brief	Данная программа является серверной частью. Она
 *			принимает новые соединения и требует пароль. Если пароль
 *			введен неверно трижды, соответствующий адрес добавляется в список
 *			заблокированных. Успешность ввода пароля сообщается клиенту.
 *
 * \author	Рогоза А. А.
 * \author	Романов С. А.
 * \date	15/04/2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <chrono>
#include <vector>
#include <algorithm>
#include <ratio>

/*!
 * \brief Структура для хранения заблокированных адресов
 */
struct Block {
	std::chrono::steady_clock::time_point	start;
	struct sockaddr_in						socket;
};

bool needBreak = false;

/*!
 * \brief Выводит сообщение об ошибке и аварийно (с кодом 1) завершает работу программы.
 * \param msg Сообщение
 */
void error( const char * msg )
{
	printf( "%s\n", msg );
	exit( 1 );
}

int main()
{
	std::vector<Block> blocked; //! Список заблокированных IP-адресов

	int serverSocket = socket( AF_INET, SOCK_STREAM, 0 ); //! Создаем серверный сокет
	if ( serverSocket == -1 ) {
		error( "Socket error!" );
	}

	struct sockaddr_in server = { AF_INET, 10001, INADDR_ANY, 0 }; //! Структура с информацией о серверном сокете
	//! Связываем адрес сервера с сокетом
	if ( bind( serverSocket, (struct sockaddr*) &server, sizeof( struct sockaddr_in ) ) != 0 ) {
		perror( "Errr" );
		error( "Bind error!" );
	}

	//! Включаем прием TCP-соединений
	if ( listen( serverSocket, 10 ) == -1 ) {
		error( "Lister error!" );
	}

	struct sockaddr_in client; //! сокет клиента
	while ( 1 ) {
		printf( "Waiting for connection\n" );
		uint clientLength = sizeof( struct sockaddr_in );
		//! Ждём желающих подключиться
		int clientSocket = accept( serverSocket, (struct sockaddr*) &client, &clientLength );
		if ( clientSocket == -1 ) {
			//! Error
			continue;
		}

		bool canConnect = true;
		//! Ищем в списке заблокированных
		for ( auto it = blocked.begin(); it != blocked.end(); ++it ) {
			if ( it->socket.sin_addr.s_addr == client.sin_addr.s_addr ) {
				//! Текущее время
				std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
				//! Количество минут, прошедших с момента блокировки
				std::chrono::minutes elapsed = std::chrono::duration_cast<std::chrono::minutes>( end - it->start );
				canConnect =  elapsed.count() > 5;
				//! Если время прошло, удаляем из списка
				if ( canConnect )
					blocked.erase( it );
			}
		}

		if ( canConnect ) {
			char truePassword[] = "truePassword";
			int wrongAttempt = 0;
			char welcomeMessage[] = "Enter password: ";
			while ( !needBreak && wrongAttempt != 3 ) {
				//! Отправляем клиенту сообщение с просьбой ввести пароль
				//! Флаг MSG_WAITALL заставляет отправлять сообщение одним пакетом
				//! Флаг MSG_NOSIGNAL не вызывает сигнал SIGPIPE в случае обрыва соединения.
				if ( send( clientSocket, welcomeMessage, sizeof( welcomeMessage ), MSG_WAITALL | MSG_NOSIGNAL ) == -1 ) {
					needBreak = true;
					break;
				}

				char password[30];
				//! Читаем пароль, присланный подключившимся
				if ( read( clientSocket, password, sizeof( password ) ) > 0 ) {
					if ( strcmp( truePassword, password ) != 0 ) {
						char answer[] = "Wrong password!\n";
						send( clientSocket, answer, sizeof( answer ), MSG_WAITALL | MSG_NOSIGNAL );
						++wrongAttempt;
						sleep( 1 );
					}
					else
						break;
				}
				else {
					//! Если не смогли прочитать, то произошел разрыв соединения, не иначе
					needBreak = true;
					break;
				}
			}

			if ( needBreak ) {
				//! Сюда зайдет программа только в случае разрыва соединения
				close( clientSocket );
				needBreak = false;
				continue;
			}

			if ( wrongAttempt == 3 ) {
				//! Добавляем адрес в список заблокированных
				std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
				Block block = { begin, client };
				blocked.push_back( block );

				char answer[] = "Your attempt's count is ended!";
				send( clientSocket, answer, sizeof( answer ), MSG_WAITALL | MSG_NOSIGNAL );
			}
			else {
				//! Сообщение об успешной авторизации
				char answer[] = "You are logged in!\n";
				send( clientSocket, answer, sizeof( answer ), MSG_WAITALL | MSG_NOSIGNAL );
			}
			sleep( 1 );
		}
		else {
			//! Уведомляем подключившегося о том, что его адрес в списке заблокированных
			char answer[] = "You're blocked";
			send( clientSocket, answer, sizeof( answer ), MSG_WAITALL | MSG_NOSIGNAL );
		}

		//! Закрываем соединение с клиентом
		close( clientSocket );
		needBreak = false;
	}
	return 0;
}
