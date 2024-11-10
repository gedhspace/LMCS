#include "HttpServer.h"
#include <corecrt_io.h>
#include <fstream>
#include<string>
#include <thread>
#include <future>
//#include <io.h>
#pragma warning(disable : 4996)
using namespace std;

HttpServer::HttpServer(int port) : port(port) {
    WORD wVersion = MAKEWORD(2, 2);
    WSADATA wsadata;
    if (WSAStartup(wVersion, &wsadata) != 0)
    {
        perror("WSA failed");
        exit(0);
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    //cout << server_fd << endl;
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));

    if (0 < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
}

void HttpServer::handleClient(int client_fd, char buff[]) {
    // char buffer[1024];

    // memset(buffer, 0, sizeof(buffer));
    // cout << client_fd;
    // read(client_fd, buffer, sizeof(buffer));

     //std::cout << "Received request:\n" << buffer << std::endl;

    std::string response = getResponse(buff);
    send(client_fd, response.c_str(), response.size(), 0);
}

void HttpServer::start() {
    std::cout << "Server is running on port " << port << ".open http://127.0.0.1:"<<port << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }
        char buff[1024];
        memset(buff, 0, sizeof(buff));
        int len = recv(client_fd, buff, sizeof(buff), 0);
       // cout << buff << endl;
        thread::thread(mem_fn(&HttpServer::handleClient),client_fd,buff);
        //close(client_fd);
    }
}

void HttpServer::SetRep(string(*func)(string))
{
    getResponse = func;
}





