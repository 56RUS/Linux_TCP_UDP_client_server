
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <limits>
#include <vector>


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

// Проверка, что строка является числом. true - число, false - не число
bool isNumber(const std::string &inStr)
{
    size_t pos = 0;

    pos = inStr.find_first_not_of("-0123456789");
    if (pos == std::string::npos)
        return true;
    else
        return false;
}

// Обработка сообщения, приянтого от клиента. Если в данных есть числа - вернется их сумма в виде строки. Если чисел нет - вернется пустая строка
void prcsMsgFromClient(const char *pData, std::string &outRes)
{
    bool existNumber = false;
    int calcRes = 0;
    size_t pos = 0;
    std::string inStr = "";
    std::string tmpStr = "";
    std::vector<std::string> vSubstr;

    outRes = "";
    if (pData == NULL)
        return;
    
    inStr.assign(pData, (pData + strlen(pData)));
	// Из входной строки получаю вектор строк, разделенных пробелами
    while (true)
    {
		// Ищу пробел в строке
        pos = inStr.find(' ');
		// Если пробел найден
        if (pos != std::string::npos)
        {
			// Получаю подстроку от начала строки до пробела
            tmpStr = inStr.substr(0, pos);
            inStr = inStr.substr(pos + 1);
			// Если подстрока не нулевой длины - добавляю ее в вектор
            if (tmpStr.length() > 0)
                vSubstr.push_back(tmpStr);
        }
        else
            break;
    }
	// Если во входной строке остались данные - добавить их в вектор
    if (inStr.length() > 0)
        vSubstr.push_back(inStr);

	// Проверяю каждое значение в векторе
    for (size_t i = 0; i < vSubstr.size(); i++)
    {
		// Если строка - это число
        if (isNumber(vSubstr[i]))
        {
			// Ставлю флаг
            existNumber = true;
			// К результату прибаляю значение числа
            calcRes += std::stoi(vSubstr[i]);

        }
    }

	// Если во входной строке были числа - преобразовать сумму в строку
    if (existNumber)
        outRes = std::to_string(calcRes);
}

// Создание TCP сервера. Функция возвращает дескриптор созданного сокета. Или -1 в случае ошибки
int createTcpServer(int portNum)
{
    int srvSock = -1;
    int res = 0;
    sockaddr_in addr;
	

	// Создание сокета
    srvSock = socket(AF_INET, SOCK_STREAM, 0);
    if (srvSock < 0)
    {
        std::cout << "\n\n" << "ERROR: error on create TCP server socket! Error = [" << errno << "]" << "\n\n";
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
        std::cout << "\n\n" << "ERROR: error bind TCP socket on port = [" << portNum << "]. Error = [" << errno << "]" << "\n\n";
        close(srvSock);
        return -1;
    }

	// Постановка сокета на прослушивание порта
    res = listen(srvSock, 1000);
    if (res != 0)
    {
        std::cout << "\n\n" << "ERROR: can't set TCP socket in listen state. Error = [" << errno << "]" << "\n\n";
        close(srvSock);
        return -1;
    }

    std::cout << "\n" << "SUCCESS: TCP server successfully started on port = [" << portNum << "]" << "\n\n";
    return srvSock;
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

// Основная логика работы
int main()
{
    int portNum = 0;
    int retPoll = -1;
    int srvSockTcp = -1;
    int tcpClntSock = -1;
    pollfd pollCnctTcp;
    int sockUdpRecv = -1;
    int sockUdpSend = -1;
    sockaddr_in addrUdpSend;
    socklen_t addrRecvFromLen = 0;
    sockaddr_in addrRecvFrom;
    pollfd pollRecvUdp;
    int bytesRdSnd = 0;
    char recvBuf[1024];
    std::string tmpStr = "";
	// Вектор, содержащий события для приема данных от подключенных клиентов
    std::vector<pollfd> vTcpClnts;


	// Ввод с консоли номера порта, с которым будут работать TCP и UDP
    portNum = getPortNumber();
    std::cout << "Use port number = [" << portNum << "]" << "\n\n";

	// Создание TCP сервера
    srvSockTcp = createTcpServer(portNum);
    if (srvSockTcp < 0)
        std::cout << "\n\n" << "ERROR: Can't create TCP server. Error = [" << errno << "]. Program not running..." << "\n\n";

	// Создание UDP для получения данных
    sockUdpRecv = createUdpServer(portNum);
    if (sockUdpRecv < 0)
        std::cout << "\n\n" << "ERROR: Can't create UDP socket for receive. Error = [" << errno << "]. Program not running..." << "\n\n";

	// Создание UDP для передачи данных
    sockUdpSend = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockUdpSend < 0)
        std::cout << "\n\n" << "ERROR: error on create UDP socket for send. Error = [" << errno << "]. Program not running..." << "\n\n";

	// Настроки UDP для передачи данных
    memset(&addrUdpSend, 0, sizeof(sockaddr_in));
    addrUdpSend.sin_family = AF_INET;
    addrUdpSend.sin_addr.s_addr = htonl(INADDR_ANY);
    addrUdpSend.sin_port = htons(portNum);

    // Если все сокеты успешно созданы
    if ((srvSockTcp > -1) && (sockUdpRecv > -1) && (sockUdpSend > -1))
    {
        vTcpClnts.reserve(64);

		// Настройка события для приема подключений к TCP серверу
        pollCnctTcp.fd = srvSockTcp;
        pollCnctTcp.events = POLLIN;

		// Настройка события для приема данных по UDP
        pollRecvUdp.fd = sockUdpRecv;
        pollRecvUdp.events = POLLIN;

		// Рабочий цикл
        while (true)
        {
            // ----- Проверка наличия входящих данных для UDP сервера -----
            retPoll = poll(&pollRecvUdp, 1, 10);
            // Ошибка
            if (retPoll == -1)
            {
            }
            // Вышел таймаут, данных нет
            else if (retPoll == 0)
            {
            }
            // Есть данные, которые нужно прочитать
            else
            {
                // Сброс события
                pollRecvUdp.revents = 0;

                addrRecvFromLen = sizeof(sockaddr_in);
                memset(&addrRecvFrom, 0, sizeof(sockaddr_in));
				// Получение данных с заполнением информации от кого они пришли
                bytesRdSnd = recvfrom(pollRecvUdp.fd, recvBuf, sizeof(recvBuf), 0, (sockaddr *)&addrRecvFrom, &addrRecvFromLen);
                
				// Если есть данные
                if (bytesRdSnd > 0)
                {
					// Добавление признака конца строки после данных
                    if (bytesRdSnd < sizeof(recvBuf))
                        recvBuf[bytesRdSnd] = '\0';
                    else
                        recvBuf[sizeof(recvBuf) - 1] = '\0';

                    std::cout << "RECEIVE: On UDP receive [" << bytesRdSnd << "] bytes, data = [" << recvBuf << "]" << "\n";

					// Обработка полученных данных в соответствии с алгоритмом
                    prcsMsgFromClient(recvBuf, tmpStr);
					// Если в данных были числа - поменять отправляемые данные
                    if (tmpStr.length() > 0)
                        strcpy(recvBuf, tmpStr.c_str());

					// Для порта, куда отправить данные, задаю порт, откуда данные пришли
                    addrUdpSend.sin_port = addrRecvFrom.sin_port;

					// Отправка ответа
                    bytesRdSnd = sendto(sockUdpSend, recvBuf, strlen(recvBuf), 0, (sockaddr *)&addrUdpSend, sizeof(addrUdpSend));
                    if (bytesRdSnd > -1)
                        std::cout << "SEND: Send [" << bytesRdSnd << "] bytes on UDP. Data = [" << recvBuf << "]\n";
                }
            }
            // ------------------------------------------------------------

            // ----- Проверка наличия клиентов, подключенных к TCP серверу -----
            retPoll = poll(&pollCnctTcp, 1, 10);
            // Ошибка
            if (retPoll == -1)
            {
            }
            // Вышел таймаут, клиентов нет
            else if (retPoll == 0)
            {
            }
            // Есть клиенты, готовые к подключению
            else
            {
                // Сброс события
                pollCnctTcp.revents = 0;
                // Прием подключения клиента
                tcpClntSock = accept(srvSockTcp, NULL, NULL);
                if (tcpClntSock < 0)
                    std::cout << "ERROR: error on accept client connect to TCP" << "\n\n";
                else
                {
					// Добавить новое значение в вектор
                    vTcpClnts.push_back(pollfd());
					// Задать дескриптор нового подключения
                    vTcpClnts[vTcpClnts.size() - 1].fd = tcpClntSock;
					// Задать отслеживаемое событие
                    vTcpClnts[vTcpClnts.size() - 1].events = POLLIN;

                    std::cout << "ACCEPT: on TCP accept new client with descriptor = [" << tcpClntSock << "]. Number active clients = [" << vTcpClnts.size() << "]" << "\n";
                    tcpClntSock = -1;
                }
            }
            // ------------------------------------------------------------

            // ----- Проверка наличия входящих данных для TCP сервера -----
            retPoll = poll(vTcpClnts.data(), vTcpClnts.size(), 10);
            // Ошибка
            if (retPoll == -1)
            {
            }
            // Вышел таймаут, данных нет
            else if (retPoll == 0)
            {
            }
            // Есть данные, которые нужно прочитать
            else
            {
				// Ищу подключение, от которого пришли данные
                for (size_t i = 0; i < vTcpClnts.size(); )
                {
					// Нашел от кого пришли данные
                    if (vTcpClnts[i].revents & POLLIN)
                    {
						// Сброс события
                        vTcpClnts[i].revents = 0;
                        
						// Получение данных
                        bytesRdSnd = recv(vTcpClnts[i].fd, recvBuf, sizeof(recvBuf), 0);
						// Если прочитано больше нуля байт
                        if (bytesRdSnd > 0)
                        {
							// Добавление признака конца строки после данных
                            if (bytesRdSnd < sizeof(recvBuf))
                                recvBuf[bytesRdSnd] = '\0';
                            else
                                recvBuf[sizeof(recvBuf) - 1] = '\0';

                            std::cout << "RECEIVE: From TCP client with descriptor = [" << vTcpClnts[i].fd << "] receive [" << bytesRdSnd << "] bytes, data = [" << recvBuf << "]" << "\n";

							// Обработка полученных данных в соответствии с алгоритмом
                            prcsMsgFromClient(recvBuf, tmpStr);
							// Если в данных были числа - поменять отправляемые данные
                            if (tmpStr.length() > 0)
                                strcpy(recvBuf, tmpStr.c_str());

							// Отправка ответа
                            bytesRdSnd = send(vTcpClnts[i].fd, recvBuf, strlen(recvBuf), 0);
							// Если данные успешно отправлены
                            if (bytesRdSnd > 0)
                            {
                                std::cout << "SEND: To TCP client with descriptor = [" << vTcpClnts[i].fd << "] send [" << bytesRdSnd << "] bytes, data = [" << recvBuf << "]" << "\n";
								// Проверить следующего клиента
                                i++;
                            }
                            else
                            {
                                std::cout << "ERROR: Error on send data to TCP client with descriptor = [" << vTcpClnts[i].fd << "], disconnect it. Number active clients = [" << (vTcpClnts.size() - 1) << "]" << "\n";
								// Удаление данных о подключении из вектора
                                vTcpClnts.erase((vTcpClnts.begin() + i));
                            }
                        }
						// Если прочитано 0 байт - клиент отключился
                        else
                        {
                            std::cout << "DISCONNECT: TCP client with descriptor = [" << vTcpClnts[i].fd << "] disconnected. Number active clients = [" << (vTcpClnts.size() - 1) << "]" << "\n";
							// Удаление данных о подключении из вектора
                            vTcpClnts.erase((vTcpClnts.begin() + i));
                        }
                    }
					// Если событие не у текущего клиента - перейти к следующему
                    else
                        i++;
                }
            }
            // ------------------------------------------------------------
        }
    }

	// Закрытие дескрипторов
    close(srvSockTcp);
    close(sockUdpRecv);
    close(sockUdpSend);

    std::cout << "\n\n" << "Press any key and ENTER for exit..." << "\n\n";
    std::cin >> portNum;

    return 0;
}
