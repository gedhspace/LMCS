#pragma once
#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <functional>
#include <map>
#include "curl/include/curl/curl.h"
#include <windows.h>

#ifdef _UNICODE
using _tstring = std::wstring;
#else
using _tstring = std::string;
#endif

//请求头信息
using CURL_HEADER_INFO = std::map<std::string, std::string>;

class CCurlDownload;
//单个文件下载
class CCurlDownload
{
public:
    CCurlDownload();

    CCurlDownload(const CCurlDownload& obj);

    CCurlDownload(const std::string& strUrl, const std::string& strPath, int nConcurrency = 6);

    ~CCurlDownload();

    //获取内容数据大小
    size_t GetContentLength(const std::string& strUrl, CURL_HEADER_INFO& headerInfo);
    size_t GetContentLength(const std::string& strUrl);

    //设置下载信息
    void SetDownloadInfo(const std::string& strUrl, const std::string& strPath, int nConcurrency = 6);

    // 同步下载文件
    bool SyncDownload(std::function<void(long long dlNow, long long dlTotal, double lfProgress)> cb = nullptr);

    // 异步下载文件
    void AsyncDownload(std::function<void(long long dlNow, long long dlTotal, double lfProgress)> cb = nullptr);

    // 取消下载
    void CancelDownload();

    // 等待下载结束
    void WaitForDownload() const;

    // 是否下载中
    bool IsDownloading() const;

    // 获取上次下载结果
    bool GetLastDownloadResult() const;

    long long GetDownloadingSize() const;

    long long GetDownloadingTotal() const;

private:

    typedef struct _PackagePart PackagePart;

    bool _IsSupportAcceptRanges(const CURL_HEADER_INFO& headerInfo);

    bool _InitCurlInfo(PackagePart* pPackageInfo, const std::string& strUrl);

    // 分段多线程下载文件
    bool _DownloadByParts(long long nTotalSize, long long packageSize, int nConcurrency);

    // 下载前清空文件
    void _TruncateFile(const std::string& strPath);

    // 进行数据传输
    bool _TransferData(PackagePart* pPackageInfo);

    // 下载前备份文件
    bool _BackupFile(const std::string& strPath);

    // 头部写入回调
    static size_t _HeaderFunc(char* buffer, size_t size, size_t nitems, void* pUserData);

    // 文件写入回调
    static size_t _WriteFunc(void* pData, size_t size, size_t nmemb, void* pUserData);

    // 下载进度回调
    static int _ProgressFunc(void* pUserData, curl_off_t dlTotal, curl_off_t dlNow, curl_off_t ulTotal, curl_off_t ulNow);

    // 文件写入
    size_t _WriteFile(const void* pBuffer, DWORD size, PackagePart* pPackageInfo);

    // 下载进度
    int _ProgressProc(PackagePart* pPackageInfo, long long dlTotal, long long dlNow);

    // 休眠
    void _SleepMillisecond(int millisecond) const;

private:
    std::string m_strUrl;                           //下载链接地址
    std::string m_strPath;                          //文件存放地址
    std::vector<PackagePart*> m_packageParts;       //分包下载信息
    std::function<void(long long dlNow, long long dlTotal, double lfProgress)> m_cbProgress;     //下载进度回调
    std::atomic<int> m_threadCount;                 //下载线程数量
    int m_nConcurrency;                             //下载线程数
    bool m_bCancel;                                 //是否取消
    bool m_bDownloading;                            //是否下载中
    bool m_bDownloadSuccess;                        //是否下载成功
    long long m_lastDownload;                       //下载大小
    long long m_lastTotal;                          //总共大小
};