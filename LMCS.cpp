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
#include "MinecraftUser.h"
#include "CommandUI.h"
#include "Jiyu.h"
#include "HttpServer.h"
#include "base.h"
#include "MinecraftDownload.h"
#include "UI.h"


#pragma warning(disable:4996)
#define myvi "0.0.0.5"
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR '\\'

#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)

using namespace std;

using json = nlohmann::json;




typedef void (*Func)(void);

string getweb(string);




bool IsKeyPressed(unsigned int key) {
    return (GetAsyncKeyState(key) & 0x8000) != 0;
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
    if (startname == "None") {
        cout << "未设置玩家名" << endl;
        return;
    }
    string username=startname;
    
    string uuid;
    uuid = Getuuid(username);
    if (uuid == "Not Found") {
        CreatUser(1, username);
        uuid = Getuuid(username);
    }
    
    commad = commad + alljar+" net.minecraft.client.main.Main"+" --username "+username+" --version "+lunchver+" --gameDir "+runPath+".minecraft" + " --assetsDir E:\\LMCS\\LMCS\\.minecraft\\assets --assetIndex 5 --uuid "+uuid+" --accessToken "+uuid +" --clientId ${clientid} --xuid ${auth_xuid} --userType msa --versionType \"LMCS\" --width 854 --height 480";


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

void choss_menu(int wz) {
    gotoxy(13, statusright + chossadd);cout << (wz == 1 ? ">" : " ") << "下载游戏" << endl;
    gotoxy(14, statusright + chossadd); cout << (wz == 2 ? ">" : " ") << "启动游戏" << endl;
    gotoxy(15, statusright + chossadd); cout << (wz == 3 ? ">" : " ") << "启动Minecraft隐藏程序" << endl;
    gotoxy(16, statusright + chossadd); cout << (wz == 4 ? ">" : " ") << "检查更新"<<endl ;
    gotoxy(17, statusright + chossadd); cout << (wz == 5 ? ">" : " ") << "注入极域Dll" ;
    cout << endl << endl;
    //gotoxy(100, 100);
    
}

void menu_print(int p) {
    //system("cls");
    print_logo();
    cout << "*************************************************************************************" <<endl;
    cout << "状态" << endl; gotoxy(12, statusright); cout << "* 选项" << endl;
    cout << "Minecraft是否运行:" << (isrun == 0 ? "否" : "是"); gotoxy(13, statusright); cout << "*" << endl;
    cout << "Minecraft隐藏是否启动:" << (ishide == 0 ? "否" : "是"); gotoxy(14, statusright); cout << "*" << endl;
    cout << "Minecraft是否下载:" << (isdown == 0 ? "否" : "是"); gotoxy(15, statusright); cout << "*" << endl;
    cout << "Minecraft下载速度:"; printf("%.2lfMB/s", speed); gotoxy(16, statusright); cout << "*" << endl;
    cout << "Minecraft启动账号:" << startname; gotoxy(17, statusright); cout << "*" << endl;
    cout << "极域运行状态:" << (jiyurun == 0 ? "否" : "是"); gotoxy(18, statusright); cout << "*" << endl;

    choss_menu(p);


}

void menu() {
    int wz = 1;
    while (true) {
        if (KEY_DOWN(VK_DOWN)) {
            wz++;
            if (wz > chosscount) {
                wz = chosscount;
            }
            while(KEY_DOWN(VK_DOWN)){}
        }
        if (KEY_DOWN(VK_UP)) {
            wz--;
            if (wz < 1) {
                wz = 1;
            }

            while (KEY_DOWN(VK_UP)) {}
        }
        gotoxy(0, 0);
        menu_print(wz);
    }
}
void init() {
    print_logo();


    cout << "正在检查更新..." << endl;
    //checkupdata();
    cout << "LMCS正在启动..." << endl;
    Getstartname();
    //Sleep(1000);
    system("cls");
    print_logo();

    

    menu();
}

int main() {
    
   //system("mode con cols=120 lines=40");
    //init();
   // CreatUser(1, "1");
    //downloadmc("1.20");
    
    //lunchminecraft();
   // mchide();

   // DOWNLOAD t;
    //https://bmclapi2.bangbang93.com/version/1.20/client
    
   // t.Push("https://piston-data.mojang.com/v1/objects/0b48c22e8ed722bcae66e25a03531409681e648b/client.jar", "test/client.jar", 27403978);

   web_start();
  // downloadmc("1.20");
  // MincosoftLogin();
    
   // t.down();

    return 0;
}