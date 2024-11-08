#pragma once
#include<iostream>
#include<Windows.h>

using namespace std;

class HttpServer {
public:
    HttpServer(int port);
    void start();
    
    string (*getResponse)(string);
    void SetRep(string(*func)(string));

private:
    int server_fd;
    int port;
    void handleClient(int client_fd,char buff[]);
    //std::string getResponse(const std::string& request);
};
