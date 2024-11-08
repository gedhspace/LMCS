#pragma once
#include "base.h"
#include "HttpServer.h"
#include <iostream>
#include <thread>
#include <string>

using namespace std;

string getResponse(string request) {
    std::string response;
    string loc;
    int Httpwz = request.find("HTTP");
    for (int i = 4; i <= Httpwz - 2; i++) {
        loc = loc + request[i];
    }
    //cout << loc << endl;
    string findh;
    string can;
    if (loc.find("?") >= 1) {
        findh = loc.substr(1, loc.find("?") - 1);
        can = loc.substr(loc.find("?") + 1, loc.size());
    }
    else {
        findh = loc;
    }



    cout << can << endl;



    bool found = false;

    if (findh == "") {
        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + to_string(getFileContent("index.html").size()) + "\r\n";
        response += "\r\n";
        //cout << getFileContent("index.html") << endl;
        response += getFileContent("index.html");
        found = true;
    }

    if (findh == "download_minecraft") {
        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + to_string(11) + "\r\n";
        response += "\r\n";
        //cout << getFileContent("index.html") << endl;
        response += "Downloading";

        string ver = can.substr(can.find("=")+1, can.size());
        //cout << ver << endl;
        thread (downloadmc,ver).detach();

        found = true;



    }

    if (findh == "getver") {
        
        //cout << getFileContent("index.html") << endl;
        
        vector<version> ver;
        ver = getversions();
        json data;
        for (version i : ver) {
            data[i.version] = i.releaseTime;
        }

        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/json\r\n";
        response += "Content-Length: " + to_string(data.dump().size()) + "\r\n";
        response += "\r\n";

        response += data.dump();

       
        //cout << ver << endl;
        

        found = true;
    }

    if (findh == "getstat") {

        json data;
        data["misrun"] = isrun;
        data["hide"] = ishide;
        data["user"] = startname;
        data["speed"] = speed;
        data["isdown"] = isdown;
        data["jisrun"] = jiyurun;
        
        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/json\r\n";
        response += "Content-Length: " + to_string(data.dump().size()) + "\r\n";
        response += "\r\n";

        response += data.dump();

        found = true;
    }



    if (!found) {
        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + to_string(9) + "\r\n";
        response += "\r\n";
        response += "Not Found";
    }

    cout << response << endl;
    return response;
}

void web_start() {
	HttpServer server(8080);
    server.SetRep(getResponse);
    server.start();
}