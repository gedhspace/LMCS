#pragma once
#include <iostream>
#include <thread>
#include <windows.h>
#pragma warning(disable : 4996)
#include <curl/curl.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR '\\'
using namespace std;

bool comp[80000];
long long jin[80000];
bool fatherj[80000][5000];
int downcount;
bool cand=false;

size_t WriteData(void* ptr, size_t size, size_t nmemb, std::ofstream* stream)
{
    stream->write(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

// 分段下载函数
bool DownloadSegment(const std::string& url, string name, long start, long end,int father,int meid,bool re)
{
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
            
            cand = true;
            return false;
        }
        outputnow.close();
        fatherj[father][meid] = true;

        return true;
    }
    cand = true;
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

bool mergeFiles(const std::string& file1, const std::string& file2) {
    std::ifstream inFile1(file1, std::ios::binary);
    std::ifstream inFile2(file2, std::ios::binary);
    std::ofstream outFile(file1, std::ios::binary | std::ios::app);

    if (!inFile1.is_open() || !inFile2.is_open() || !outFile.is_open()) {
        cout << "Merge error" << endl;
        cand = true;
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

char* appeddp(const char* a, const char* b) {
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

    

    for (long start = 0; start < file_size; start += segment_size) {
        long end = start + segment_size - 1;
        if (end >= file_size) {
            end = file_size - 1;
        }

        string nowname = output_filename + ".bak." + to_string(dld);
        //cout << nowname << endl;


        //std::cout << endl<<"Downloading segment: " << start << "-" << end << std::endl;
        thread(DownloadSegment, url, nowname, start, end, id, dld,false).detach();
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
                    
                }
            }
        }
        if (flag) {
            break;
        }
    }
    //cout << "merge" << endl;
    for (int i = 1; i <= dld - 1; i++) {
        string next= output_filename + ".bak." + to_string(i);
        //cout << next << endl;
        mergeFiles(output_filename, next);
        //rmfile(appeddp(next.c_str(), ""));
    }

    Sleep(500);

    for (int i = 1; i <= dld - 1; i++) {
        string next = output_filename + ".bak." + to_string(i);
        //cout << next << endl;
        //mergeFiles(output_filename, next);
        rmfile(appeddp(next.c_str(), ""));
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
    int i = 1;
    downcount = 0;
    for (auto it : q) {
        if (downcount > 5) {
            while (downcount > 5) {
                //cout << downcount << endl;
            }
        }
        
       // cout << "Download  " << it.url << " " << it.name << " " << it.size << endl;
        downcount++;
        thread(download_thered, it.url, it.name, it.size, i).detach();
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
            cout << "%   " << nowt << "/" << totel << " " ;
            printf("%.2lf MB/s ", (nowt - last) * 1.0 / 1048576 * 1.0);
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
            cout << sy << "files remaining [";
            bool ff = false;
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
            cout <<"]" << endl;
            if (flag) {
                cout << "Download complete." << endl;
                return;
            }
            if (cand) {
                cout << "Download error" << endl;
                return;
            }
            Sleep(1000);
        }
	}
	void get_q() {
		for (auto i : q) {
			cout << i.url << endl;
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
