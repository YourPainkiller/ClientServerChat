#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <thread>
#include <algorithm>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <chrono>



void receive_messages(int socket) {  //Функция для получения сообщений. На вход принимает переменную int (сокет) на котором в последующем 
    char buffer[1024] = {0};         // прослушиваются сообщения.
    while (true) {
        int recv_len = recv(socket, buffer, 1024, 0);  // Функция для получения сообщений от сервера с заданного сокета.
        if (recv_len == -1) {
            std::cerr << "Failed to receive message from server.\n";
            break;
        }
        std::cout << buffer << "\n";
        
        memset(buffer, 0, sizeof(buffer));  // Очистка буфера 
    }
}

void connect_to_server(int client_socket, int port, char *serverIp){  // Функция для подключения к серверу. На вход принимает три переменных
    struct hostent* host = gethostbyname(serverIp);                   // типа int: сокет клиента, порт и Ip сервера.
    struct sockaddr_in server_address;
    bzero((char*)&server_address, sizeof(server_address)); 
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list)); // Подключение к локал хосту
    server_address.sin_port = htons(port); 
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) { // Если подключение не успешно, то в консоли
        std::cerr << "Failed to connect to server.\n";                                              // появляется сообщение об ошибке, иначе сообщение об успехе
        close(client_socket);
        exit(0);
    } else {
        std::cout << "Connection successful.\n";
    }
}

int create_socket(){  // creating IPv4 socket 
    int my_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(my_socket < 0){
        std::cerr << "Failed to create socket.\n";
        exit(0);
    } else {
        std::cout << "Socket created successful.\n";
    }
    return my_socket;
}

int main(int argc, char *argv[]) {  // Основная часть программы. 
    if(argc != 3){                  // На вход принимает два параметра: Ip адресс сервера и порт к которому будет подключение.
        std::cerr << "Error. Write down Ip and port.\n";
        exit(0);
    }
    char *serverIp = argv[1];
    int port = atoi(argv[2]);

    int client_socket = create_socket();  // Создание сокета клиента.
    
    connect_to_server(client_socket, port, serverIp);  // Подключение к серверу.
    
    std::thread t(receive_messages, client_socket);  // Разделение на 2 потока.
    t.detach();                                      // Один для получения сообщений, другой для отправки.

    std::string message;
    while (true) {  // Бесконечный цикл на чтение сообщений из терминала и их отправку.
        std::getline(std::cin, message);
        int send_len = send(client_socket, message.c_str(), message.length(), 0);  //Отправка сообщения из терминала.
        if (send_len == -1) {
            std::cerr << "Failed to send message to server." << std::endl;
            break;
        }
        if(message == "#EXIT"){
            break;
        }
    }

    std::cout << "You are successful disconnect.\n";  //Отключение клиента и закрытие сокета.
    close(client_socket);
    return 0;
}