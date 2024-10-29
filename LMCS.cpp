#include <iostream>
#include <curl/curl.h>
#include <string>
#include<direct.h>
#include "json.hpp"
#include <filesystem>
#include <Windows.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <shlwapi.h>
#include <filesystem>
#include <sys/stat.h>
#include <stdio.h>
#include <iostream>
#include <tchar.h>
//#include "CCurlDownloadMgr.h"
//#include "ThreadPool.h"
#include <fstream>
#include <thread>
#include <atlstr.h>
//#include "CCurlDownloadMgr.h"
#include "fasterDownload.h"
#include "CCurlDownloadMgr.h"

#pragma warning(disable:4996)
#define myvi "0.0.0.5"
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR '\\'

#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)

using namespace std;

using json = nlohmann::json;


typedef void (*Func)(void);

string getweb(string);

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
    
}

struct version {
    string version;
    string jsonurl;
    string type;
    string time;
    string releaseTime;
};


bool IsKeyPressed(unsigned int key) {
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

char* appedd(const char* a,const char* b) {
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

// 写入数据的回调函数
/*
size_t WriteData(void* ptr, size_t size, size_t nmemb, std::ofstream* stream)
{
    //cout << ptr << endl;
    stream->write(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}*/
/*
// 分段下载函数
bool DownloadSegment(const std::string& url, std::ofstream& output, long start, long end)
{
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        // 设置URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // 设置Range头
        std::string range = std::to_string(start) + "-" + std::to_string(end);
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());

        // 设置写入数据的回调函数
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

        // 设置SSL选项
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 验证服务器的SSL证书
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // 验证证书上的主机名

        // 执行请求
        res = curl_easy_perform(curl);

        // 清理
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }
        return true;
    }
    return false;
}

// 获取文件大小
long GetFileSize(const std::string& url)
{
    CURL* curl;
    CURLcode res;
    double file_size = 0.0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // 不下载内容
        curl_easy_setopt(curl, CURLOPT_HEADER, 1L); // 只获取头部信息
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // 设置SSL选项
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 验证服务器的SSL证书
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // 验证证书上的主机名

        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &file_size);
            if ((res == CURLE_OK) && (file_size > 0.0)) {
                curl_easy_cleanup(curl);
                return static_cast<long>(file_size);
            }
        }
        else {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    return -1;
}


void download(string url,string name) {
    
    std::string output_filename = name;
    long segment_size = 1024*4096; // 2MB

    // 获取文件大小
    long file_size = GetFileSize(url);
    if (file_size == -1) {
        std::cerr << "Failed to get file size." << std::endl;
        //return 0;
    }

    std::cout << "File size: " << file_size << " bytes" << std::endl;

    // 打开输出文件
    std::ofstream output(output_filename, std::ios::binary);
    if (!output.is_open()) {
        std::cerr << "Failed to open output file." << std::endl;
       // return 0;
    }

    // 分段下载
    for (long start = 0; start < file_size; start += segment_size) {
        long end = start + segment_size - 1;
        if (end >= file_size) {
            end = file_size - 1;
        }
        std::cout << "Downloading segment: " << start << "-" << end << std::endl;
        if (!DownloadSegment(url, output, start, end)) {
            std::cerr << "Failed to download segment." << std::endl;
            output.close();
            //return 0;
        }
    }

    // 关闭输出文件
    output.close();
    std::cout << "Download complete." << std::endl;
   // return 1;
}
*/

string checkupdata() {
    string ge = getweb("https://api.github.com/repos/gedhspace/LMCS/releases/latest");
    json gt = json::parse(ge.c_str());
    string nowvi = gt["tag_name"];
    if (nowvi == myvi) {
        cout << "已是最新版本" << endl;
        return "First";
    }
    else {
        cout << "发现新版本是否更新(Y/N)";
        char t;
        cin >> t;

        if (t == 'Y' || t == 'y') {
            string download_url;
            json tt = gt["assets"];
            for (int i = 0; i < tt.size(); i++) {
                if (tt[i]["name"] == "LMCS.exe") {
                    download_url = tt[i]["browser_download_url"];
                    break;
                }
            }
            //download("https://ojproxy.gedh2011.us.kg/" + download_url, "update.exe");
            DOWNLOAD t;
            t.Push("https://ojproxy.gedh2011.us.kg/" + download_url, "updata.exe", GetFileSize("https://ojproxy.gedh2011.us.kg/" + download_url));
            t.down();
                cout << "更新成功，请运行update.exe" << endl;
                while (1) {

                }
                

        }
        else {
            return "";
        }
    }

    return "";
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

vector<version> getversions() {
    vector<version> ve;
    json data = json::parse(getweb("http://launchermeta.mojang.com/mc/game/version_manifest.json").c_str());
    //cout << getweb("https://bmclapi2.bangbang93.com/mc/game/version_manifest.json") << endl;
    //json data = json::parse(getweb("https://bmclapi2.bangbang93.com/mc/game/version_manifest.json").c_str());
    //string tmp = data["versions"];
    //cout << data << endl;
    json vi = data["versions"];
    //cout << vi.size() << endl;
    for (unsigned int i = 0; i < vi.size(); i++)
    {
        version tmp;
        tmp.version = vi[i]["id"];
        tmp.jsonurl = vi[i]["url"];
        tmp.releaseTime = vi[i]["releaseTime"];
        tmp.type = vi[i]["type"];
        tmp.time = vi[i]["time"];
        ve.push_back(tmp);
    }

    return ve;
}

void print_logo() {
    cout << "|                     |\\                            /      --------      --- -------- " << endl;
    cout << "|                     |  \\                        / |    /              /   " << endl;
    cout << "|                     |    \\                     /  |  /                |" << endl;
    cout << "|                     |      \\                  /   | /                 \\" << endl;
    cout << "|                     |        \\               /    | |                   ----------" << endl;
    cout << "|                     |          \\           /      |  \\                            \\" << endl;
    cout << "|                     |            \\        /       |   \\                            |" << endl;
    cout << "|                     |              \\    /         |    \\                          /" << endl;
    cout << "|\\\\\\\\\\\\\\\              |                \\/           |      --------      ----------" << endl << endl << endl;
}

void downloadmc(string verdov) {
    DOWNLOAD k;

    //curl_global_init(CURL_GLOBAL_ALL);
    //CCurlDownloadMgr mgr;
    //CCurlDownload task;
    vector<version> ve = getversions();
    string dov =verdov;
    string mcjson="";
    for (auto i : ve) {
        if (i.version == dov) {
            mcjson = i.jsonurl;
        }
        //cout << i.version << endl;
    }
    //cout << mcjson << endl;

    json mcjsondata = json::parse(getweb(mcjson));
    //cout << mcjsondata << endl;
    
    CreateDirectory(".minecraft", NULL);
    CreateDirectory(".minecraft/versions", NULL);
    CreateDirectory(".minecraft/assets", NULL);
    CreateDirectory(".minecraft/libraries", NULL);

    string mcassetjsonurl = mcjsondata["assetIndex"]["url"];
    //cout << mcjsondata["assetIndex"]["url"];

    json mcassetjsondata = json::parse(getweb(mcassetjsonurl));

    mkdir(appedd(".minecraft/versions/", dov.c_str()));
    //thread t1(download, mcjsondata["downloads"]["client"]["url"], appedd(appedd(appedd(appedd(".minecraft/versions/", dov.c_str()), "/"), dov.c_str()), ".jar"));
    //t1.detach();
    //download(mcjsondata["downloads"]["client"]["url"], appedd(appedd(appedd(appedd(".minecraft/versions/", dov.c_str()),"/"), dov.c_str()),".jar"));
    //task.SetDownloadInfo(mcjsondata["downloads"]["client"]["url"], appedd(appedd(appedd(appedd(".minecraft/versions/", dov.c_str()), "/"), dov.c_str()), ".jar"), 6);
   // mgr.PushBack(task);
   
    //k.Push(appedd(appedd(appedd("https://bmclapi2.bangbang93.com/version/", dov.c_str()), "/"), "client"), appedd(appedd(appedd(appedd(".minecraft/versions/", dov.c_str()), "/"), dov.c_str()), ".jar"), mcjsondata["downloads"]["client"]["size"]);
    k.Push(mcjson, appedd(appedd(appedd(appedd(".minecraft/versions/", dov.c_str()), "/"), dov.c_str()), ".json"), GetFileSize(mcjson));
    k.Push(mcjsondata["downloads"]["client"]["url"], appedd(appedd(appedd(appedd(".minecraft/versions/", dov.c_str()), "/"), dov.c_str()), ".jar"), mcjsondata["downloads"]["client"]["size"]);
    json mclibraries = mcjsondata["libraries"];
   

    
    
    for (int i = 0; i < mclibraries.size(); i++) {
        //cout << mclibraries[i]["downloads"]["artifact"]["url"] << endl;
        if (mclibraries[i].contains("rules")){
            //cout << mclibraries[i]["rules"] << endl;
            if (mclibraries[i]["rules"].dump().find("windows") != -1) {
                
                string temp = mclibraries[i]["downloads"]["artifact"]["path"];
                //cout << temp << endl;
                int mmmax = -1;
                for (int j = 1; j < temp.size(); j++) {
                    if (temp[j] == '/') {
                        mmmax = max(mmmax, j);
                    }
                }
               // cout << mmmax << endl;
                temp.erase(temp.begin()+mmmax,temp.end());
                //cout << temp << endl;
                //cout << appedd(".minecraft/libraries/", temp.c_str()) << endl;
                mkdir(appedd(".minecraft/libraries/", temp.c_str()));
                string ad = mclibraries[i]["downloads"]["artifact"]["path"];
                if (file_exists(appedd(".minecraft/libraries/", ad.c_str()))) {
                    //continue;
                }
                //thread t1(download, mclibraries[i]["downloads"]["artifact"]["url"], appedd(".minecraft/libraries/", ad.c_str()));
                //t1.detach();
              //  task.SetDownloadInfo(mclibraries[i]["downloads"]["artifact"]["url"], appedd(".minecraft/libraries/", ad.c_str()), 6);
               // mgr.PushBack(task);
                //download(mclibraries[i]["downloads"]["artifact"]["url"], appedd(".minecraft/libraries/", ad.c_str()));
                k.Push(mclibraries[i]["downloads"]["artifact"]["url"], appedd(".minecraft/libraries/", ad.c_str()), mclibraries[i]["downloads"]["artifact"]["size"]);
            }
        }else{
            string temp = mclibraries[i]["downloads"]["artifact"]["path"];
            //cout << temp << endl;
            int mmmax = -1;
            for (int j = 0; j < temp.size(); j++) {
                if (temp[j] == '/') {
                    mmmax = max(mmmax, j);
                }
            }
            //cout << mmmax << endl;
            temp.erase(temp.begin() + mmmax, temp.end());
            //cout << temp << endl;
           // cout << appedd(".minecraft/libraries/", temp.c_str()) << endl;
            mkdir(appedd(".minecraft/libraries/", temp.c_str()));
            string ad = mclibraries[i]["downloads"]["artifact"]["path"];
            if (file_exists(appedd(".minecraft/libraries/", ad.c_str()))) {
               //continue;
            }
            //thread t1(download, mclibraries[i]["downloads"]["artifact"]["url"], appedd(".minecraft/libraries/", ad.c_str()));
            //t1.detach();
           // task.SetDownloadInfo(mclibraries[i]["downloads"]["artifact"]["url"], appedd(".minecraft/libraries/", ad.c_str()), 6);
           // mgr.PushBack(task);
            //download(mclibraries[i]["downloads"]["artifact"]["url"], appedd(".minecraft/libraries/", ad.c_str()));
            k.Push(mclibraries[i]["downloads"]["artifact"]["url"], appedd(".minecraft/libraries/", ad.c_str()), mclibraries[i]["downloads"]["artifact"]["size"]);
            
        }
    }
   // cout << "1";
   
   

    //library ok
    
    json ob = mcassetjsondata["objects"];
    //cout << ob << endl;
    //cout << ob["icons/icon_128x128.png"];
    //string hash = ob["minecraft/lang/zh_cn.json"]["hash"];
    //string ttt(hash.c_str(), 2);
    // mkdir(appedd(".minecraft/assets/objects/", ttt.c_str()));
    // mkdir(".minecraft/assets/objects/indexes");

     //k.Push(appedd(appedd(appedd("https://bmclapi2.bangbang93.com/assets/", ttt.c_str()), "/"), hash.c_str()), appedd(appedd(appedd(".minecraft/assets/objects/", ttt.c_str()), "/"), hash.c_str()), ob["minecraft/lang/zh_cn.json"]["size"]);
     
    for (auto it = ob.begin(); it != ob.end(); ++it) {
        //cout << it.key() << endl;
        string hash = ob[it.key()]["hash"];
        string ttt(hash.c_str(), 2);
        if (file_exists(appedd(appedd(appedd(".minecraft/assets/objects/", ttt.c_str()), "/"), hash.c_str()))) {
            //continue;
        }
        //cout << ttt << endl;
        // 
        mkdir(appedd(".minecraft/assets/objects/", ttt.c_str()));
        // 
        //cout << appedd(appedd(appedd(".minecraft/assets/", ttt.c_str()), "/"), hash.c_str()) << endl;
        //cout << appedd(appedd(appedd("https://resources.download.minecraft.net/", ttt.c_str()), "/"), hash.c_str()) << endl;
        //task.SetDownloadInfo(appedd(appedd(appedd("https://resources.download.minecraft.net/", ttt.c_str()), "/"), hash.c_str()), appedd(appedd(appedd(".minecraft/assets/", ttt.c_str()), "/"), hash.c_str()), 6);
        //mgr.PushBack(task);
        
        //download(appedd(appedd(appedd("https://resources.download.minecraft.net/",ttt.c_str()),"/"),hash.c_str()), appedd(appedd(appedd(".minecraft/assets/", ttt.c_str()),"/"), hash.c_str()));
        //k.Push(appedd(appedd(appedd("https://resources.download.minecraft.net/", ttt.c_str()), "/"), hash.c_str()), appedd(appedd(appedd(".minecraft/assets/objects/", ttt.c_str()), "/"), hash.c_str()), ob[it.key()]["size"]);
    }
    
    k.down();
    /*
    mgr.StartDownload([](long long dlNow, long long dlTotal, double lfProgress) {
        _tprintf(_T("%.2lf%% %lld/%lld\n"), lfProgress * 100, dlNow, dlTotal);
        });
   
    curl_global_cleanup();*/
}

void lunchminecraft() {

    char buff[FILENAME_MAX];

    getcwd(buff, FILENAME_MAX);

    string runPath=(buff) + string("\\");

   
    
    string lunchver = "1.20";
    string configurationFile = runPath+".minecraft\\versions\\"+lunchver+"\\log4j2.xml";
    string javapath = "\"D:\\Program Files\\Java\\jdk-17\\bin\\java.exe\" ";
    string natives = runPath+ ".minecraft\\versions\\" + lunchver + "\\" + "natives-windows-x86_64";
    string mcjar = runPath+".minecraft\\versions\\"+lunchver+"\\"+lunchver+".jar";
    string commad = javapath+string("-Xmx4658m -Dfile.encoding=GB18030 -Dsun.stdout.encoding=GB18030 -Dsun.stderr.encoding=GB18030 -Djava.rmi.server.useCodebaseOnly=true -Dcom.sun.jndi.rmi.object.trustURLCodebase=false -Dcom.sun.jndi.cosnaming.object.trustURLCodebase=false -Dlog4j2.formatMsgNoLookups=true -Dlog4j.configurationFile=") + configurationFile + string(" -Dminecraft.client.jar=") + mcjar +string(" -XX:+UnlockExperimentalVMOptions -XX:+UseG1GC -XX:G1NewSizePercent=20 -XX:G1ReservePercent=20 -XX:MaxGCPauseMillis=50 -XX:G1HeapRegionSize=32m -XX:-UseAdaptiveSizePolicy -XX:-OmitStackTraceInFastThrow -XX:-DontCompileHugeMethods -Dfml.ignoreInvalidMinecraftCertificates=true -Dfml.ignorePatchDiscrepancies=true -XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump -Djava.library.path=")+natives+string(" -Djna.tmpdir=")+natives+" -Dorg.lwjgl.system.SharedLibraryExtractPath="+natives+" -Dio.netty.native.workdir="+natives+" -Dminecraft.launcher.brand=HMCL -Dminecraft.launcher.version=3.5.9 -cp ";


    string jsonfile;
    try {
        std::string filePath = runPath+".minecraft\\versions\\"+lunchver+"\\"+lunchver+".json";
        jsonfile = getFileContent(filePath);
        //std::cout << "File content: " << content << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
        return;
    }

    json data1 = json::parse(jsonfile.c_str());
    

    json mclibraries = data1["libraries"];


    string alljar = "";
    bool first = true;

    for (int i = 0; i < mclibraries.size(); i++) {
        //cout << mclibraries[i]["downloads"]["artifact"]["url"] << endl;
        if (mclibraries[i].contains("rules")) {
            //cout << mclibraries[i]["rules"] << endl;
            if (mclibraries[i]["rules"].dump().find("windows") != -1) {

                
                string ad = mclibraries[i]["downloads"]["artifact"]["path"];
                if (first) {
                    alljar = alljar + runPath + ".minecraft/libraries/" + ad;
                    //cout << runPath + ".minecraft/libraries/" + ad << endl;
                    first = false;
                }
                else {
                    alljar = alljar + ";" + runPath + ".minecraft/libraries/" + ad;
                    //cout << runPath + ".minecraft/libraries/" + ad << endl;
                }
            }
        }
        else {
            //string temp = mclibraries[i]["downloads"]["artifact"]["path"];
            
            string ad = mclibraries[i]["downloads"]["artifact"]["path"];//appedd(".minecraft/libraries/", ad.c_str())
            if (first) {
                alljar = alljar+runPath + ".minecraft/libraries/" + ad;
                //cout << runPath + ".minecraft/libraries/" + ad << endl;
                first = false;
            }
            else {
                alljar = alljar+";"+runPath +".minecraft/libraries/" + ad;
                //cout << runPath + ".minecraft/libraries/" + ad << endl;
            }
           

        }
    }
    alljar=alljar+";"+mcjar;
    //cout << alljar << endl;
    string username;
    cout << "玩家名:";
    getline(cin, username);
    commad = commad + alljar+" net.minecraft.client.main.Main"+" --username "+username+" --version "+lunchver+" --gameDir "+runPath+".minecraft" + " --assetsDir E : \\LMCS\\LMCS\\.minecraft\\assets --assetIndex 5 --uuid 15d1e1a1eede398095f5bbe7c9a50059 --accessToken 0d3b63b38fe24f06babbf3ebd9e70b6f --clientId ${clientid} --xuid ${auth_xuid} --userType msa --versionType \"LMCS\" --width 854 --height 480";


   cout << commad.c_str() << endl;
    system(commad.c_str());
}


void mchide(){
    bool Boss_key = false;
    HWND hwnd = FindWindowA(NULL, "Minecraft 1.20");
    while(1) {
        if (KEY_DOWN(VK_CONTROL) && KEY_DOWN('Q')) {
            ShowWindow(hwnd, Boss_key);
            Boss_key = !Boss_key;
            while ((KEY_DOWN(VK_CONTROL) && KEY_DOWN('Q'))) Sleep(50);
        }
        Sleep(100);
    }


    //while(true){}

}

int menu() {
    cout << ">+启动" << endl;
    cout << " +下载" << endl;
    cout << " +检查更新" << endl;
    cout << " +启动Minecraft隐藏程序" << endl;
    cout << " +注入极域dll" << endl;
    int wz = 1;
    while (true) {
        if (IsKeyPressed(40)) {
            wz++;
            if (wz == 6) {
                wz = 5;
            }
            while(IsKeyPressed(40)){}

            system("cls");
            print_logo();
            if (wz == 1) {
                cout << ">+启动" << endl;
            }
            else {
                cout << " +启动" << endl;
            }

            if (wz == 2) {
                cout << ">+下载" << endl;
            }
            else {
                cout << " +下载" << endl;
            }

            if (wz == 3) {
                cout << ">+检查更新" << endl;
            }
            else {
                cout << " +检查更新" << endl;
            }

            if (wz == 4) {
                cout << ">+启动Minecraft隐藏程序" << endl;
            }
            else {
                cout << " +启动Minecraft隐藏程序" << endl;
            }

            if (wz == 5) {
                cout << ">+注入极域dll" << endl;
            }
            else {
                cout << " +注入极域dll" << endl;
            }
        }
        if (IsKeyPressed(38)) {
            wz--;
            if (wz == 0) {
                wz = 1;
            }
            while (IsKeyPressed(38)) {}

            system("cls");
            print_logo();
            if (wz == 1) {
                cout << ">+启动" << endl;
            }
            else {
                cout << " +启动" << endl;
            }

            if (wz == 2) {
                cout << ">+下载" << endl;
            }
            else {
                cout << " +下载" << endl;
            }

            if (wz == 3) {
                cout << ">+检查更新" << endl;
            }
            else {
                cout << " +检查更新" << endl;
            }

            if (wz == 4) {
                cout << ">+启动Minecraft隐藏程序" << endl;
            }
            else {
                cout << " +启动Minecraft隐藏程序" << endl;
            }

            if (wz == 5) {
                cout << ">+注入极域dll" << endl;
            }
            else {
                cout << " +注入极域dll" << endl;
            }
        }
        if (IsKeyPressed(VK_RETURN)) {
            if (wz == 1) {
                string ver;
                cout << "版本号:";
                cin >> ver;

                downloadmc(ver);
            }
            if (wz == 2) {
                lunchminecraft();
            }
            if (wz == 3) {
                checkupdata();
            }
            if (wz == 4) {
                thread(mchide).detach();
            }
        }

    }
}
void init() {
    print_logo();


    cout << "正在检查更新..." << endl;
    checkupdata();
    cout << "LMCS正在启动..." << endl;
    Sleep(1000);
    system("cls");
    print_logo();
    menu();
}

int main() {
    
    //init();
    //downloadmc("1.20");
    
    //lunchminecraft();
   // mchide();

   // DOWNLOAD t;
    //https://bmclapi2.bangbang93.com/version/1.20/client
    
   // t.Push("https://piston-data.mojang.com/v1/objects/0b48c22e8ed722bcae66e25a03531409681e648b/client.jar", "test/client.jar", 27403978);


    

    
   // t.down();

    return 0;
}