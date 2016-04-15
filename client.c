/*!
 * \brief	Данная программа является клиентской частью. Она
 *			соединяется с сервером на текущей же машине и пытается
 *			войти в систему, отправляя пароль. При успешном вводе пароля
 *			соединение закрывается.
 *
 * \author	Рогоза А. А.
 * \author	Романов С. А.
 * \date	15/04/2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

/*!
 * \brief Выводит сообщение об ошибке и аварийно (с кодом 1) завершает работу программы.
 * \param msg Сообщение
 */
void error( const char * msg )
{
	printf( "%s\n", msg );
	exit( 1 );
}

/*!
 * \brief Выводит меню с доступным управлением.
 */
void printMenu()
{
	printf( "Menu:\n" );
	printf( "1 - try to connect to server\n" );
	printf( "0 - exit\n" );
}

int main()
{
	struct sockaddr_in server = { AF_INET, 10001, 0, 0 }; //! сокет сервера
	server.sin_addr.s_addr = inet_addr( "0.0.0.0" );

	int clientSocket = socket( AF_INET, SOCK_STREAM, 0 );
	if ( clientSocket == -1 ) {
		error( "Socket error!" );
	}

	//! Подключаемся к серверу, связав сокет с адресом сервера
	uint len = sizeof( struct sockaddr_in );
	if ( connect( clientSocket, (const struct sockaddr_in *)&server, len ) == -1 ) {
		error( "Connect error!" );
	}

	char choise = 1;
	char space;
	while ( choise ) {
		printMenu(); //! Выводим меню
		scanf( "%c", &choise );
		scanf( "%c", &space ); //! Дополнительное считывание для знака перевода на новую строку
		switch ( choise ) {
		case '1': {
			char message[50];
			//! Считываем сообщение от сервера
			if ( read( clientSocket, message, sizeof( message ) ) > 0 ) {
				printf( "%s\n", message );
				if ( strcmp( message, "Enter password: " ) == 0 ) {
					//! Присланное сообщение является приглашение ко вводу пароля
					char password[30];
					scanf( "%s", password );
					scanf( "%c", &space );
					//! Отправляем сообщение с паролем на сервер.
					send( clientSocket, password, sizeof( password ),
						  MSG_WAITALL );
					//! Читаем ответ сервера на правильность введенного пароля.
					read( clientSocket, message, sizeof( message ) );
					printf( "%s", message );
					if ( strcmp( message, "Wrong password!\n" ) != 0 ) {
						//! Пароль верный
						printf( "Woohoo!\n" );
						exit( 0 );
					}
				}
				else {
					//! Присланное сообщение свидетельствует о том, что
					//! наш адрес заблокирован.
					exit( 0 );
				}
				break;
			}
			else {
				//! Ошибка соединения
				printf( "Can't connect to server\n" );
				exit( 0 );
			}
		}
		case '0':
			printf( "Closing\n" );
			exit( 0 );
			break;
		default:
			printf( "Wrong input\n\n" );
		}
	}

	return 0;
}
