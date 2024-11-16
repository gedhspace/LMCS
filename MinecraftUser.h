#pragma once
#include<iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <string>
#include "json.hpp"
#include "base.h"

using namespace std;

using json = nlohmann::json;

string startname;



unsigned int random_char() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    return dis(gen);
}

std::string generate_hex(const unsigned int len) {
    std::stringstream ss;
    for (auto i = 0; i < len; i++) {
        const auto rc = random_char();
        std::stringstream hexstream;
        hexstream << std::hex << rc;
        auto hex = hexstream.str();
        ss << (hex.length() < 2 ? '0' + hex : hex);
    }
    return ss.str();
}

string Getuuid(string name) {

        if (file_exists("minecraftuser.lmcs.json")) {
           // cout << getFileContent_m("minecraftuser.lmcs.json") << endl;
            
            json data=json::parse(getFileContent("minecraftuser.lmcs.json").c_str());
            string re;
            if (data.contains(name)) {
                re = data[name];
                data["lastest"] = name;
                ofstream file("minecraftuser.lmcs.json");
                file << data.dump() << endl;
            }
            else {
                return "Not Found";
            }
            
            return re;
        }
        else {
            ofstream file("minecraftuser.lmcs.json");
            file.close();
            return "Not Found";
        }

}

bool CreatUser(int p, string name) {
    if (file_exists("minecraftuser.lmcs.json")) {
        string now=getFileContent("minecraftuser.lmcs.json");
        json data = json::parse(now.c_str());
        data[name] = generate_hex(16);
        data["lastest"] = name;
        ofstream file("minecraftuser.lmcs.json");
        file << data.dump() << endl;


    }
    else {

        ofstream file("minecraftuser.lmcs.json");
        file.close();
        CreatUser(p, name);
        
    }
    return true;
}

void Getstartname() {
    if (file_exists("minecraftuser.lmcs.json")) {
        string now = getFileContent("minecraftuser.lmcs.json");

        json data = json::parse(now.c_str());
        if (data.contains("lastest")) {
            startname = data["lastest"];
        }
        else {
            startname = "None";
        }
        


    }
    else {

        ofstream file("minecraftuser.lmcs.json");
        file.close();
        startname = "None";

    }
}


void MincosoftLogin() {
    //GetUUID

    system("start https://login.live.com/oauth20_authorize.srf?client_id=00000000402b5328&response_type=code&scope=service%3A%3Auser.auth.xboxlive.com%3A%3AMBI_SSL&redirect_uri=https%3A%2F%2Flogin.live.com%2Foauth20_desktop.srf");

}