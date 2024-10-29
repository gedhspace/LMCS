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

//����ͷ��Ϣ
using CURL_HEADER_INFO = std::map<std::string, std::string>;

class CCurlDownload;
//�����ļ�����
class CCurlDownload
{
public:
    CCurlDownload();

    CCurlDownload(const CCurlDownload& obj);

    CCurlDownload(const std::string& strUrl, const std::string& strPath, int nConcurrency = 6);

    ~CCurlDownload();

    //��ȡ�������ݴ�С
    size_t GetContentLength(const std::string& strUrl, CURL_HEADER_INFO& headerInfo);
    size_t GetContentLength(const std::string& strUrl);

    //����������Ϣ
    void SetDownloadInfo(const std::string& strUrl, const std::string& strPath, int nConcurrency = 6);

    // ͬ�������ļ�
    bool SyncDownload(std::function<void(long long dlNow, long long dlTotal, double lfProgress)> cb = nullptr);

    // �첽�����ļ�
    void AsyncDownload(std::function<void(long long dlNow, long long dlTotal, double lfProgress)> cb = nullptr);

    // ȡ������
    void CancelDownload();

    // �ȴ����ؽ���
    void WaitForDownload() const;

    // �Ƿ�������
    bool IsDownloading() const;

    // ��ȡ�ϴ����ؽ��
    bool GetLastDownloadResult() const;

    long long GetDownloadingSize() const;

    long long GetDownloadingTotal() const;

private:

    typedef struct _PackagePart PackagePart;

    bool _IsSupportAcceptRanges(const CURL_HEADER_INFO& headerInfo);

    bool _InitCurlInfo(PackagePart* pPackageInfo, const std::string& strUrl);

    // �ֶζ��߳������ļ�
    bool _DownloadByParts(long long nTotalSize, long long packageSize, int nConcurrency);

    // ����ǰ����ļ�
    void _TruncateFile(const std::string& strPath);

    // �������ݴ���
    bool _TransferData(PackagePart* pPackageInfo);

    // ����ǰ�����ļ�
    bool _BackupFile(const std::string& strPath);

    // ͷ��д��ص�
    static size_t _HeaderFunc(char* buffer, size_t size, size_t nitems, void* pUserData);

    // �ļ�д��ص�
    static size_t _WriteFunc(void* pData, size_t size, size_t nmemb, void* pUserData);

    // ���ؽ��Ȼص�
    static int _ProgressFunc(void* pUserData, curl_off_t dlTotal, curl_off_t dlNow, curl_off_t ulTotal, curl_off_t ulNow);

    // �ļ�д��
    size_t _WriteFile(const void* pBuffer, DWORD size, PackagePart* pPackageInfo);

    // ���ؽ���
    int _ProgressProc(PackagePart* pPackageInfo, long long dlTotal, long long dlNow);

    // ����
    void _SleepMillisecond(int millisecond) const;

private:
    std::string m_strUrl;                           //�������ӵ�ַ
    std::string m_strPath;                          //�ļ���ŵ�ַ
    std::vector<PackagePart*> m_packageParts;       //�ְ�������Ϣ
    std::function<void(long long dlNow, long long dlTotal, double lfProgress)> m_cbProgress;     //���ؽ��Ȼص�
    std::atomic<int> m_threadCount;                 //�����߳�����
    int m_nConcurrency;                             //�����߳���
    bool m_bCancel;                                 //�Ƿ�ȡ��
    bool m_bDownloading;                            //�Ƿ�������
    bool m_bDownloadSuccess;                        //�Ƿ����سɹ�
    long long m_lastDownload;                       //���ش�С
    long long m_lastTotal;                          //�ܹ���С
};