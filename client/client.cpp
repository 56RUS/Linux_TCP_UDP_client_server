
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <limits>


// Ввод номера порта из консоли
int getPortNumber()
{
    int portNum = 0;
	
	// Пока не будет введен корректное значение
    while (true)
    {
        std::cout << "Input used port number: ";
		// Ввод значения из консоли
        std::cin >> portNum;
		// Если была ошибка при вводе
        if (std::cin.fail())
        {
			// Очистить ошибки
            std::cin.clear();
			// Очистить входной буфер
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else
        {
			// Проверка, что введен корректный номер порта
            if ((portNum > 0) && (portNum < 65536))
                break;
        }
        std::cout << "\n" << "Input invalid number. Try again..." << "\n\n";
    }
    return portNum;
}

// Создание UDP сокета для получения данных
int createUdpServer(int portNum)
{
    int srvSock = -1;
    int res = 0;
    int optval = 1;
    sockaddr_in addr;
    
    
	// Создание сокета
    srvSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (srvSock < 0)
    {
        std::cout << "\n\n" << "ERROR: error on create UDP server socket! Error = [" << errno << "]" << "\n\n";
        return -1;
    }

	// Установка опции, что порт может быть использоваться многократно
    res = setsockopt(srvSock, SOL_SOCKET, SO_REUSEPORT, (const void *)&optval, sizeof(int));
    if (res != 0)
    {
        std::cout << "\n\n" << "ERROR: Can't set UDP option (SO_REUSEPORT). Error = [" << errno << "]" << "\n\n";
        close(srvSock);
        return -1;
    }
    
	// Задание параметров
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(portNum);

	// Связывание сокета с портом
    res = bind(srvSock, (sockaddr *)&addr, sizeof(addr));
    if (res < 0)
    {
        std::cout << "\n\n" << "ERROR: error bind UDP socket on port = [" << portNum << "]. Error = [" << errno << "]" << "\n\n";
        close(srvSock);
        return -1;
    }

    std::cout << "\n" << "SUCCESS: UDP server successfully started on port = [" << portNum << "]" << "\n\n";
    return srvSock;
}

// Работа клиента по UDP
void workClientUdp(int portNum)
{
    bool runWhile = true;
    int sockUdp = -1;
    int res = -1;
    int bytesRdSnd = 0;
    char recvBuf[1024];
    char tmpBuf[64];
    std::string strInput = "";
    sockaddr_in addrUdpSend;
    pollfd pollRecvUdp;

	
	// Создание UDP Сокета для отправки данных
    sockUdp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockUdp < 0)
    {
        std::cout << "\n\n" << "ERROR: error on create UDP socket for send! Error = [" << errno << "]" << "\n\n";
        return;
    }

	// Настроки UDP для передачи данных
    memset(&addrUdpSend, 0, sizeof(sockaddr_in));
    addrUdpSend.sin_family = AF_INET;
    addrUdpSend.sin_addr.s_addr = htonl(INADDR_ANY);
    addrUdpSend.sin_port = htons(portNum);

	// Настройка события для приема данных по UDP
    pollRecvUdp.fd = sockUdp;
    pollRecvUdp.events = POLLIN;

	// Основной рабочий цикл
    while (runWhile)
    {
		// Ввод в консоли данных, которые нужно отправить серверу
        std::cout << "Input data for send:" << "\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, strInput);

		// Если данные введены
        if (strInput.length() > 0)
        {
			// Отправка данных
            bytesRdSnd = sendto(sockUdp, strInput.c_str(), strInput.length(), 0, (sockaddr *)&addrUdpSend, sizeof(addrUdpSend));
			// Если данные успешно отправлены. Иначе - ошибка
            if (bytesRdSnd > -1)
            {
                std::cout << "Send [" << bytesRdSnd << "] bytes on UDP. Data = [" << strInput << "]. Wait receive data...\n";

                // ----- Проверка наличия входящих данных по UDP -----
                res = poll(&pollRecvUdp, 1, 10000);
                // Ошибка
                if (res == -1)
                {
                }
                // Вышел таймаут, данных нет
                else if (res == 0)
                {
					std::cout << "WARNING: expired timeout for receive data from server on UDP. No data...\n";
                }
                // Есть данные, которые нужно прочитать
                else
                {
                    // Сброс события
                    pollRecvUdp.revents = 0;

					// Чтение данных
                    bytesRdSnd = recv(sockUdp, recvBuf, sizeof(recvBuf), 0);
					// Добавление признака конца строки после данных
                    if (bytesRdSnd > 0)
                    {
                        if (bytesRdSnd < sizeof(recvBuf))
                            recvBuf[bytesRdSnd] = '\0';
                        else
                            recvBuf[sizeof(recvBuf) - 1] = '\0';

                        std::cout << "RECEIVE: On UDP receive [" << bytesRdSnd << "] bytes, data = [" << recvBuf << "]" << "\n";
                    }
                }
                // ---------------------------------------------------
            }
            else
            {
                std::cout << "ERROR: Data not sended. Error = [" << errno << "]" << "\n";
                runWhile = false;
            }
        }
        else
            std::cout << "No data for send" << "\n\n";
        
        if (runWhile)
        {
			// Вопрос, нужно ли отправить еще одно сообщение серверу
            std::cout << "\n" << "Do you want send one more message? (0 - No; 1 - Yes): ";
            std::cin >> tmpBuf;
			// Если введено что-то, отличное от '1' - прекращение работы
            if (tmpBuf[0] != '1')
                runWhile = false;
        }
    }

    close(sockUdp);
    std::cout << "\n" << "End communication with remote host..." << "\n\n";
}

// Работа клиента по TCP
void workClientTcp(int portNum)
{
    bool runWhile = true;
    int clntSock = 0;
    int res = 0;
    int bytesRdSnd = 0;
    char recvBuf[1024];
    char sndBuf[1024];
    char tmpBuf[64];
    std::string strInput = "";
    sockaddr_in addrTcpSend;
    pollfd pollRecvTcp;

	
	// Создание сокета
    clntSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clntSock < 0)
    {
        std::cout << "\n\n" << "ERROR: error on create client TCP socket! Error = [" << errno << "]" << "\n\n";
        return;
    }
	
	// Настроки TCP для передачи данных
    memset(&addrTcpSend, 0, sizeof(sockaddr_in));
    addrTcpSend.sin_family = AF_INET;
    addrTcpSend.sin_addr.s_addr = htonl(INADDR_ANY);
    addrTcpSend.sin_port = htons(portNum);
	
	// Настройка события для приема данных по TCP
    pollRecvTcp.fd = clntSock;
    pollRecvTcp.events = POLLIN;

	// Подключение к TCP серверу. Если нет подключения - ошибка
    res = connect(clntSock, (sockaddr *)&addrTcpSend, sizeof(addrTcpSend));
    if (res < 0)
    {
        std::cout << "\n\n" << "ERROR: can't connect to TCP server! Error = [" << errno << "]" << "\n\n";
        return;
    }
    std::cout << "\n\n" << "SUCCESS: connected to TCP server!" << "\n\n";

	// Рабочий цикл
    while (runWhile)
    {
		// Ввод в консоли данных, которые нужно отправить серверу
        std::cout << "Input data for send:" << "\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, strInput);

		// Если данные введены
        if (strInput.length() > 0)
        {
			// Отправка данных
            bytesRdSnd = send(clntSock, strInput.c_str(), strInput.length(), 0);
			// Если данные успешно отправлены. Иначе - ошибка
            if (bytesRdSnd > -1)
            {
                std::cout << "Send [" << bytesRdSnd << "] bytes on TCP. Wait receive data...\n";
				
				// ----- Проверка наличия входящих данных по TCP -----
                res = poll(&pollRecvTcp, 1, 10000);
                // Ошибка
                if (res == -1)
                {
                }
                // Вышел таймаут, данных нет
                else if (res == 0)
                {
					std::cout << "WARNING: expired timeout for receive data from server on TCP. No data...\n";
                }
                // Есть данные, которые нужно прочитать
                else
                {
                    // Сброс события
                    pollRecvTcp.revents = 0;

					// Чтение данных
					bytesRdSnd = recv(clntSock, recvBuf, sizeof(recvBuf), 0);
					// Добавление признака конца строки после данных
					if (bytesRdSnd > 0)
					{
						if (bytesRdSnd < sizeof(recvBuf))
							recvBuf[bytesRdSnd] = '\0';
						else
							recvBuf[sizeof(recvBuf) - 1] = '\0';

						std::cout << "On TCP receive [" << bytesRdSnd << "] bytes, data = [" << recvBuf << "]" << "\n";
					}
					else
					{
						std::cout << "TCP, connection reseted by remote host" << "\n";
						runWhile = false;
					}
                }
                // ---------------------------------------------------
            }
            else
            {
                std::cout << "ERROR: Data not sended. Error = [" << errno << "]" << "\n";
                runWhile = false;
            }
        }
        else
            std::cout << "No data for send" << "\n\n";
        
        if (runWhile)
        {
			// Вопрос, нужно ли отправить еще одно сообщение серверу
            std::cout << "\n" << "Do you want send one more message? (0 - No; 1 - Yes): ";
            std::cin >> tmpBuf;
			// Если введено что-то, отличное от '1' - прекращение работы
            if (tmpBuf[0] != '1')
                runWhile = false;
        }
    }

    close(clntSock);
    std::cout << "\n" << "End communication with remote host..." << "\n\n";
}

// Основная логика работы
int main()
{
    int portNum = 0;
    char tmpBuf[64];

	// Выбор протокола
    while (true)
    {
        std::cout << "\n" << "Select protocol: (0 - TCP; 1 - UDP): ";
        std::cin >> tmpBuf;
		// Цикл крутится, пока не будет введено валидное значение
        if (tmpBuf[0] == '0')
        {
            std::cout << "\n" << "Using protocol: TCP" << "\n\n";
            break;
        }
        else if (tmpBuf[0] == '1')
        {
            std::cout << "\n" << "Using protocol: UDP" << "\n\n";
            break;
        }
    }

	// Ввод с консоли номера порта, с которым будут работать по TCP и UDP
    portNum = getPortNumber();
    std::cout << "Use port number = [" << portNum << "]" << "\n\n";

	// Работа по соответсвующему протоколу
    if (tmpBuf[0] == '0')
        workClientTcp(portNum);
    else if (tmpBuf[0] == '1')
        workClientUdp(portNum);
    else
        std::cout << "ERROR: selected undefined protocol. Program not running..." << "\n\n";

    std::cout << "\n\n" << "Press any key and ENTER for exit..." << "\n\n";
    std::cin >> portNum;

    return 0;
}
