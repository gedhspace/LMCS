#pragma once
#include<iostream>
#include <fstream>
#include <io.h>
#include <curl/curl.h>

#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR '\\'
#pragma warning(disable : 4996)

using namespace std;

bool isrun = false;
bool ishide = false;

char* appedd(const char* a, const char* b) {
    const char* str1 = a;
    const char* str2 = b;
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    size_t new_length = len1 + len2 + 1;
    char* result = new char[new_length];
    strcpy(result, str1);
    strcat(result, str2);
    return result;
}

bool file_exists(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file != nullptr) {
        fclose(file);
        return true;
    }
    return false;
}

string getFileContent(const std::string& filePath) {
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


void mkdir(char path[]) {



    int mode = 0; // 判定目录或文件是否存在的标识符

    if (!_access(path, mode))
    {
        return;
    }



    for (int i = 0; path[i]; i++) {
        if (path[i] == '/') {
            path[i] = PATH_SEPARATOR;
        }

    }

    // 创建目录命令的字符串
    char mkdir_command[1024];
    sprintf(mkdir_command, "mkdir %s", path);

    // 调用系统命令
    system(mkdir_command);
}

bool mergeFiles(const std::string& file1, const std::string& file2) {
    std::ifstream inFile1(file1, std::ios::binary);
    std::ifstream inFile2(file2, std::ios::binary);
    std::ofstream outFile(file1, std::ios::binary | std::ios::app);

    if (!inFile1.is_open() || !inFile2.is_open() || !outFile.is_open()) {
        cout << "Merge error" << endl;
        //cand = true;
        return false;
    }

    outFile << inFile2.rdbuf();

    inFile1.close();
    inFile2.close();
    outFile.close();

    return true;
}

void rmfile(char path[]) {
    for (int i = 0; path[i]; i++) {
        if (path[i] == '/') {
            path[i] = PATH_SEPARATOR;
        }

    }

    // 创建目录命令的字符串
    char mkdir_command[1024];
    sprintf(mkdir_command, "del %s", path);

    // 调用系统命令
    system(mkdir_command);
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;

}

string getweb(string url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        //User-Agent
        headers = curl_slist_append(headers, "User-Agent: LMCS-get");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);


        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else {

            return readBuffer;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

    }

    curl_global_cleanup();
}