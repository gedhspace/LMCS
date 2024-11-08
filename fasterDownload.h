#pragma once
#include <iostream>
#include <thread>
#include <windows.h>
#include "base.h"
#pragma warning(disable : 4996)
#include <curl/curl.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR '\\'
using namespace std;

bool comp[80000];
long long jin[80000];
bool fatherj[80000][5000];
int net;
int downcount;
int threadcount;
bool cand=false;
double speed = 0;
bool isdown = false;


size_t WriteData(void* ptr, size_t size, size_t nmemb, std::ofstream* stream)
{
    stream->write(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

// 分段下载函数
bool DownloadSegment(const std::string& url, string name, long start, long end,int father,int meid,bool re)
{
    //net++;
    CURL* curl;
    CURLcode res;
    ofstream outputnow(name, std::ios::binary);

    curl = curl_easy_init();
    if (curl) {
        

        // 设置URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // 设置Range头
        std::string range = std::to_string(start) + "-" + std::to_string(end);
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());

        // 设置写入数据的回调函数
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outputnow);
        

        // 设置SSL选项
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 验证服务器的SSL证书
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // 验证证书上的主机名

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

      //  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);

        // 设置总请求超时时间为30秒
       // curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

        // 执行请求
        res = curl_easy_perform(curl);

        // 清理
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            if (!re) {
                
                DownloadSegment(url, name, start, end, father, meid, true);

            }
            else {
                cout << "Try error" << endl;
            }
            //net--;
            cand = true;
            return false;
        }
        outputnow.close();
        fatherj[father][meid] = true;
        //net--;

        return true;
    }
    cand = true;
    //net--;
    return false;
}



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

        // 设置SSL选项
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // 验证服务器的SSL证书
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L); // 验证证书上的主机名

        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

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



int download_thered(string m_url, string name, long long size, int id) {
    std::string url = m_url; // 替换为实际URL
    std::string output_filename = name;
    long segment_size = 1024 * 1024;

    // 获取文件大小
    long file_size = size;

    //std::cout <<endl <<"File size: " << file_size << " bytes" << std::endl;

    // 打开输出文件
    std::ofstream output(output_filename, std::ios::binary);
    if (!output.is_open()) {
        std::cerr << name<<" "<<url << " Failed to open output file." << std::endl;
        //MessageBox(NULL, name.c_str(), " url.c_str()", MB_OK);

        cand = true;
        exit(0);
        return 1;
    }

    int dld = 1;

    //int nowloadcount = 0;

    for (long start = 0; start < file_size; start += segment_size) {
        long end = start + segment_size - 1;
        if (end >= file_size) {
            end = file_size - 1;
        }

        string nowname = output_filename + ".bak." + to_string(dld);
        //cout << nowname << endl;

    

        //std::cout << endl<<"Downloading segment: " << start << "-" << end << std::endl;
        threadcount++;
        thread(DownloadSegment, url, nowname, start, end, id, dld,false).detach();
        //nowloadcount++;
        dld++;
        /*
        if (!DownloadSegment(url, outputnow, start, end,id,dld) ){
            std::cerr << url<<" Failed to download segment." << std::endl;
            output.close();
            cand = true;
           //xit(0);
            return 1;
        }*/
        //cout << endl << id << " " << end * 1.0 / size * 1.0 << endl;
        //jin[id] = end;
        
    }
   // cout << dld << endl;

    bool add[8002];
    memset(add, 0, sizeof(add));

    while (1) {
        bool flag = true;
        for (int i = 1; i <= dld - 1; i++) {
            if (fatherj[id][i] == false) {
                flag = false;
            }
            else {
                if (add[i] == false) {
                    add[i] = true;
                    if (i == dld - 1) {
                        jin[id] += size % segment_size;
                    }
                    else {
                        jin[id] += segment_size;
                    }
                    threadcount--;
                    
                }
            }
        }
        if (flag) {
            break;
        }
        Sleep(500);
    }
    //cout << "merge" << endl;
    for (int i = 1; i <= dld - 1; i++) {
        string next= output_filename + ".bak." + to_string(i);
        //cout << next << endl;
        if (mergeFiles(output_filename, next) == false) {
            cand = true;
        }
        //mergeFiles(output_filename, next);
        //rmfile(appeddp(next.c_str(), ""));
    }

    Sleep(500);

    for (int i = 1; i <= dld - 1; i++) {
        string next = output_filename + ".bak." + to_string(i);
        //cout << next << endl;
        //mergeFiles(output_filename, next);
        rmfile(appedd(next.c_str(), ""));
    }

    // 关闭输出文件
    output.close();
    downcount--;
    jin[id] = size;
    //std::cout << endl << id << "Download complete." << std::endl;
    comp[id] = true;

}

struct downloadinfo {
	string name, url;
	long long size;
};


void DONLOAD_thread(vector<downloadinfo> q) {
    threadcount = 0;
    int i = 1;
    downcount = 0;
    for (auto it : q) {
        if(downcount > 8) {
            while (downcount > 8) {
               //cout << downcount << endl;
                Sleep(200);
            }
        }
        
       // cout << "Download  " << it.url << " " << it.name << " " << it.size << endl;
        downcount++;
        threadcount++;
        thread(download_thered, it.url, it.name, it.size, i).detach();
        threadcount--;
        //thread(download_thered, it.url, it.name, it.size, i).join();
        i++;

        //downcount++;
        if (cand == true) {
            break;
        }
    }
}



class DOWNLOAD {
public:

    
	vector<downloadinfo> q;
    long long totel = 0;

    void init() {
        memset(comp, false, sizeof(comp));
        memset(jin, 0, sizeof(jin));
        cand = false;
    }
	void down() {
        if (isdown == true) {
            cout << "Something is downloading." << endl;
            return;
        }
        isdown = true;

        memset(comp, 0, sizeof(comp));
        int i = 1;
        /*
        for (auto it : q) {
            cout << "Download  " << it.url << " " << it.name << " " << it.size << endl;
            thread(download_thered, it.url,it.name,it.size,i).join();
            //thread(download_thered, it.url, it.name, it.size, i).join();
            i++;
            if (cand == true) {
                break;
            }
        }*/
        thread(DONLOAD_thread, q).detach();
        if (cand) {
            return;
        }
        long long last = 0;
        bool debug = 0;
        while(1){



            long long nowt = 0;
            for (int i = 1; i <= q.size(); i++) {
                nowt += jin[i];
            }
            printf("%.2lf", (nowt * 1.0 / totel * 1.0)*100);
            cout << "%   " ;
            printf("%.2lf/%.2lf ", nowt * 1.0 / 1048576 * 1.0, totel * 1.0 / 1048576 * 1.0);
            printf("%.2lf MB/s ", (nowt - last) * 1.0 / 1048576 * 1.0);
            speed = (nowt - last) * 1.0 / 1048576 * 1.0;
            //cout << threadcount << " ";
            last = nowt;
            bool flag = true;
            long long sy = 0;
            for (int i = 1; i <= q.size(); i++) {
                //cout << comp[i] << endl;
                if (comp[i] == 0) {
                    flag = false;
                    if (debug == 1) {
                        //cout << q[i-1].name << " " << q[i-1].name << endl;
                    }
                    sy++;
                    //break;
                }
            }
            if (sy == 1) {
                debug = 1;
            }

            //5 files remaining
            cout << sy << "files remaining ";
            bool ff = false;
            /*
            for (int i = 1; i <= 80; i++) {
                if ((nowt * 1.0 / totel * 1.0) * 100 >= i * 1.25) {
                    cout << "=";
                    ff = false;
                }
                else {
                    if (!ff) {
                        cout << ">";
                        ff = true;
                    }
                    else {
                        ff = true;
                        cout << " ";
                    }
                    
                }
            }
            cout <<"]" << endl;*/
            cout << endl;
            if (flag) {
                cout << "Download complete." << endl;
                isdown = false;
                return;
            }
            if (cand) {
                isdown = false;
                cout << "Download error" << endl;
                return;
            }
            Sleep(1000);
        }
	}
	void get_q() {
		for (auto i : q) {
			//cout << i.url << endl;
		}
	}
	void Push(string m_url, string m_name, long long m_size) {
		downloadinfo temp;
		temp.url = m_url;
		temp.name = m_name;
		temp.size = m_size;
		q.push_back(temp);
        totel += m_size;
	}

    
    
};
