#include "HttpServer.h"
#include <fstream>
#include<string>
#include <thread>
#include <future>
#include <corecrt_io.h>


//#include <io.h>
#pragma warning(disable : 4996)

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
  
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    //listen(server_fd, 3);
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
}

void HttpServer::start() {
    std::cout << "Server is running on port " << port << std::endl;



    while (true) {
       std::cout << "Wait." << std::endl;
       std::cout << "------------" << std::endl;
       
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        
        std::cout << "Accept" << std::endl;

        //std::cout << "next" << std::endl;
        
        // cout << buff << endl;
        std::thread th(&HttpServer::handleClient, this, client_fd, client_fd);
        th.detach();
        std::cout << "thread is run" << std::endl;
        std::cout << "detch" << std::endl;
        std::cout<<"Debug end." << std::endl << std::endl << std::endl;


        //close(client_fd);
    }
}

void HttpServer::SetRep(std::string(*func)(std::string))
{
    getResponse = func;
}

void HttpServer::handleClient(int client_fd,int client) {
    // char buffer[1024];

    // memset(buffer, 0, sizeof(buffer));
    // cout << client_fd;
    // read(client_fd, buffer, sizeof(buffer));
    //std::cout << "now+" << bu << std::endl;
     //std::cout << "Received request:\n" << buffer << std::endl;

    char buff[1024];
    memset(buff, 0, sizeof(buff));
    int len = recv(client_fd, buff, sizeof(buff), 0);
    if (len < 0) {
        std::cerr << "Receive failed: " << strerror(errno) << std::endl;
        close(client_fd);
        return;
    }

   // Sleep(1000);
    std::cout << buff << std::endl;
    std::string s = buff;
    std::cout << "Get response." << std::endl;
    std::string response = getResponse(s);
    Sleep(200);

    if (send(client_fd, response.c_str(), response.size(), 0) < 0) {
        std::cerr << "Send failed: " << strerror(errno) << std::endl;
    }

   std::cout << "Can next \n --------------------------" << std::endl;

   return;
}


