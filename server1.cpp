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
#include <ctime>



std::string make_msg(std::string message){  // Функция оформления сообщения. На вход принимает строку, а возвращает оформленную строку.
    char curtime[80];
    time_t seconds = time(NULL);
    tm* timeinfo = localtime(&seconds);
    char* format = "%A, %B %d, %Y %H:%M:%S";
    strftime(curtime, 80, format, timeinfo);
    std::string usertime = curtime;
    
    message = "------------------------------------------\n" + usertime +"\n" + message + "\n" + "------------------------------------------\n";
    return message;
}

std::string find_cmd(std::string message){  // Функция для поиска команды в сообщении. На вход принимает строку.
    std::string cmd = "";                   // При наличии идентификатора команды (#) возвращает стркоу без #
    if(message[0] == '#'){
        for(int i = 1; i < message.length(); i++){
            if(message[i] == ' ') break;
            cmd += message[i];
        }
    }
    return cmd;
}


void handle_client(int client_socket, std::vector<std::pair<int, std::string>>& clients) {  // Функция для коммуникации между клиентами.
    char buffer[1024];                                                                      // На вход принимает две переменных: сокет клиента от которого поступило сообщение
    while (true) {                                                                          // Вектор пар {сокет, ник} активных клиентов
        memset(&buffer, 0, sizeof(buffer));
        int recv_len = recv(client_socket, buffer, 1024, 0);  // Получение сообщения от клиента с заданным сокетом
        if (recv_len == -1) {
            std::cerr << "Failed to receive message from client." << std::endl;
            break;
        }

        std::string sender = "";
        std::string message = buffer;
        bool havename = false;
        for(int i = 0; i < clients.size(); i++){    // Поиск в нашей базе nickname клиента
            if(client_socket == clients[i].first){  // Если такового нет, то ником является первое сообщение 
                if(clients[i].second == "/"){       // Иначе для удобства запоминаем его nick в переменную sender
                    clients[i].second = message;
                    havename = true;
                }
                sender = clients[i].second;
            }
        }
        if(havename) continue;
        
        
        
        std::string cmd = find_cmd(message);  // Проверка на наличие команды в сообщении клиента
        std::string pmsg = "";
        if(cmd.length() > 0){  // Если наше сообщение является командой, то для удобства отделяем часть после команды
            int pos = 0;       // в приватное сообщение 
            for(int i = 0; i < message.length(); i++){
                if(message[i] == ' '){
                    pos = i;
                    break;
                }
            }
            for(int i = pos + 1; i < message.length(); i++) pmsg += message[i];
            pmsg = "private message from " + sender +": " + pmsg; 
        }
        
        // Определение какую команду ввел пользователь.
        if(cmd == "USERS"){  // Если это комада #USERS, то отправляем ему список всех активных клиентов
            std::string userslist = "";  
            for(int i = 0; i < clients.size(); i++){
                userslist += std::to_string(i) +": " + clients[i].second + "\n";
            }
            int send_len = send(client_socket, make_msg(userslist).c_str(), make_msg(userslist).length(), 0);
            if (send_len == -1) {
                std::cerr << "Failed to send message to client." << std::endl;
                break;
            }
            continue;
        } else if(cmd == "EXIT"){ // Если это команда #EXIT, то отключаем нашего юзера.
            break;
        } else if(cmd.length() > 0) {  // Если это приватное сообщение, то отпправляем сообщение user-а
            bool flag = false;         // нужному нам пользователю
            bool notuser = true;
            for(int i = 0; i < clients.size(); i++){
                if(clients[i].second == cmd){
                    notuser = false;
                    int send_len = send(clients[i].first, make_msg(pmsg).c_str(), make_msg(pmsg).length(), 0);
                    if (send_len == -1) {
                        std::cerr << "Failed to send message to client." << std::endl;
                        flag = true;
                        break;
                    }
                    break;
                }
            }
            if(flag) break;
            if(notuser){
                std::string message = "NO USERS WITH THIS NICK!\n";
                int c = send(client_socket, make_msg(message).c_str(), make_msg(message).length(), 0);
                if(c == -1){
                    std::cerr << "Failed to send message to client." << std::endl;
                }
            }
        } else { // Если это ни команда, ни приватное сообщение, то пересылаем сообщение всем пользователям
            message = sender +": " + message;
            for (int i = 0; i < clients.size(); i++) {
                if (clients[i].first != client_socket) {
                    int send_len = send(clients[i].first, make_msg(message).c_str(), make_msg(message).length(), 0);
                    if (send_len == -1) {
                        std::cerr << "Failed to send message to client." << std::endl;
                        break;
                    }
                }
            }
        }
        // Чистим буффер
        memset(buffer, 0, sizeof(buffer));
    }
    
    // При отключении клиента чистим нашу базу 
    int pos = clients.size();
    for(int i = 0; i < clients.size(); i++){
      if(clients[i].first == client_socket){
        pos = i;
      }
    }
    if(pos != clients.size()){
      clients.erase(clients.begin() + pos);
    }
    close(client_socket);
}

void bind_socket(int port, int server_socket){  // Функция для привязки порта к сокету сервера.
    struct sockaddr_in server_address;          // На вход принимает две переменные типа int: порт и сокет сервера
    bzero((char*)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_address.sin_port = htons(port); 

    int check = bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if(check < 0){
        std::cerr << "Error in binding socket.\n";
        close(server_socket);
        exit(0);
    } else {
        std::cout << "Socket binded successful. Waiting for connections...\n";
    }
}

void listening_for_connection(int server_socket) {  // Функция для прослушивания подключений на данном сокете
    if(listen(server_socket, 5) == -1){             // На вход принимает переменную int: сокет сервера для прослушки
        std::cerr << "Failed in connection listening.\n";
        close(server_socket);
        exit(0);
    } else {
        std::cout << "Listening for connection successful.\n";
    }
}

int accept_user(int server_socket){  // Функция для принятия User-а на данном сокете 
    struct sockaddr_in client_address; // На вход принимает переменную int: сокет сервера
    socklen_t addrlen = sizeof(client_address);
    int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &addrlen);
    if (client_socket < 0) {
        std::cerr << "Error, can't accept user.\n";
    } else {
        std::cout << "Connected to client\n";
    }
    return client_socket; // Если подключение произошло успешно, то возвращает сокет клиента (Int)
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

int main(int argc, char *argv[]) { // Основная функция для запуска сервера. На вход принимает порт
    if(argc != 2){
        std::cerr << "Error. Write down a port\n";
        exit(0);
    }
    int server_socket = create_socket();  // Создание сокета сервера
    int port = atoi(argv[1]);  
    bind_socket(port, server_socket);  //Соединение порта и сокета сервера
    listening_for_connection(server_socket);  // Прослушивание входящих подключений
    std::vector<std::pair<int, std::string>> clients;
    while (true) {
        int client_socket = accept_user(server_socket);  // Попытка принятия User-а
        std::string name = "/";
        if(client_socket < 0){
            continue;
        } else {
            std::string message = "----------------------------------------------\nYou are succefull connected to server.\nWrite down your nickname in next message.\nTo see active users send #USERS\nTo send privete message write #NICKNAME your message\nTo exit chat write down #EXIT\nHave fun.\n----------------------------------------------\n";
            int f1 = send(client_socket , message.c_str(), message.length(), 0);
            if(f1 < 0){std::cerr << "Error in sending message to client.\n";}
        }
        
        clients.push_back({client_socket, name}); // Если user принят успешно, то добавляем его в вектор подключенных user-ов
        std::thread t(handle_client, client_socket, std::ref(clients)); // Разделение на два потока, один из которыз принемает новых пользователей
        t.detach();                                                     // а второй получает и отправляет сообщения
    }
    close(server_socket); // Закрытие сокета сервера.
    return 0;
}