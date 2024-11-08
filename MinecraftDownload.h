#pragma once
#include "fasterDownload.h"
#include<iostream>
#include<vector>
#include "json.hpp"


using json = nlohmann::json;
using namespace std;

struct version {
    string version;
    string jsonurl;
    string type;
    string time;
    string releaseTime;
};

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

void downloadmc(string verdov) {
    DOWNLOAD k;

    //curl_global_init(CURL_GLOBAL_ALL);
    //CCurlDownloadMgr mgr;
    //CCurlDownload task;
    vector<version> ve = getversions();
    string dov = verdov;
    string mcjson = "";
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

    cout << "start library" << endl;
    //cout << mclibraries << endl;


    for (int i = 0; i < mclibraries.size(); i++) {
        //cout << mclibraries[i]["downloads"]["artifact"]["url"] << endl;
        if (mclibraries[i].contains("rules")) {
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
                temp.erase(temp.begin() + mmmax, temp.end());
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
        }
        else {
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


    cout << "end library" << endl;
    cout << "start assets" << endl;
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

        if (it.key().find(".ogg") != -1) {
            continue;
        }


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
        k.Push(appedd(appedd(appedd("https://resources.download.minecraft.net/", ttt.c_str()), "/"), hash.c_str()), appedd(appedd(appedd(".minecraft/assets/objects/", ttt.c_str()), "/"), hash.c_str()), ob[it.key()]["size"]);
    }

    cout << "Json is ok" << endl;

    k.down();
    /*
    mgr.StartDownload([](long long dlNow, long long dlTotal, double lfProgress) {
        _tprintf(_T("%.2lf%% %lld/%lld\n"), lfProgress * 100, dlNow, dlTotal);
        });

    curl_global_cleanup();*/
}