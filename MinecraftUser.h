#pragma once
#include<iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <string>
#include "json.hpp"

using namespace std;

using json = nlohmann::json;

string startname;

bool file_exists_m(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file != nullptr) {
        fclose(file);
        return true;
    }
    return false;
}

string getFileContent_m(const std::string& filePath) {
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)),
            (std::istreambuf_iterator<char>()));
        file.close();
        return content;
    }
    else {
        throw std::runtime_error("Unable to open file.");
    }
}

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

        if (file_exists_m("minecraftuser.lmcs.json")) {
           // cout << getFileContent_m("minecraftuser.lmcs.json") << endl;
            
            json data=json::parse(getFileContent_m("minecraftuser.lmcs.json").c_str());
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
    if (file_exists_m("minecraftuser.lmcs.json")) {
        string now=getFileContent_m("minecraftuser.lmcs.json");
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
    if (file_exists_m("minecraftuser.lmcs.json")) {
        string now = getFileContent_m("minecraftuser.lmcs.json");

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
