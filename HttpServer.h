#pragma once
#include<iostream>
#include<Windows.h>


class HttpServer {
public:
    HttpServer(int port);
    void start();

    std::string(*getResponse)(std::string);
    void SetRep(std::string(*func)(std::string));

private:
    int server_fd;
    int port;
    void handleClient(int client_fd, int client);
    //std::string getResponse(const std::string& request);
};