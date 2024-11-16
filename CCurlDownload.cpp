#include "CCurlDownload.h"
#include "ThreadPool.h"
#include <tchar.h>
#include <future>
#include <thread>

#define MINIMUM_PACKAGE_SIZE        (16384) // ��С����С
#define BACKUP_MAX_CONUT            (0)     // ��౸������

//#pragma comment(lib, "curl/lib/libcurl.lib")

typedef struct _PackagePart
{
    bool bSuccess;          //�����Ƿ�ɹ�
    CCurlDownload* pThis;   //����ָ��
    int partId;             //�ֶ�ID
    HANDLE hFile;           //�ļ����
    long long beginPos;     //��ʼƫ��
    long long endPos;       //����ƫ��
    long long downloadSize; //�����ش�С
    long long totalSize;    //�������ܴ�С
    void* curl;             //

    _PackagePart() : hFile(INVALID_HANDLE_VALUE)
    {
        curl = NULL;
        partId = 0;
        beginPos = 0;
        endPos = 0;
        downloadSize = 0;
        totalSize = 0;
        bSuccess = false;
        pThis = nullptr;
    }
}PackagePart;

CCurlDownload::CCurlDownload()
    : m_threadCount(0)
{
    m_nConcurrency = 1;
    m_bCancel = false;
    m_bDownloading = false;
    m_lastDownload = 0;
    m_lastTotal = 0;
    m_bDownloadSuccess = false;
}

CCurlDownload::CCurlDownload(const CCurlDownload& obj)
    : m_strUrl(obj.m_strUrl), m_strPath(obj.m_strPath), m_threadCount(0)
{
    m_bCancel = false;
    m_bDownloading = false;
    m_lastDownload = 0;
    m_lastTotal = 0;
    m_bDownloadSuccess = false;
    m_nConcurrency = obj.m_nConcurrency;
}

CCurlDownload::CCurlDownload(const std::string& strUrl, const std::string& strPath, int nConcurrency)
    : m_strUrl(strUrl), m_strPath(strPath), m_threadCount(0)
{
    m_bCancel = false;
    m_bDownloading = false;
    m_lastDownload = 0;
    m_lastTotal = 0;
    m_bDownloadSuccess = false;
    m_nConcurrency = nConcurrency <= 0 ? 1 : nConcurrency;
}

CCurlDownload::~CCurlDownload()
{
    (void)CancelDownload();
}

void CCurlDownload::_SleepMillisecond(int millisecond) const
{
    const int span = 20;
    if (millisecond < span)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(millisecond));
        return;
    }

    clock_t tmBegin = clock();
    while (true)
    {
        if (clock() - tmBegin > millisecond)
        {
            break;
        }

        if (m_bCancel)
        {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(span));
    }
}

size_t CCurlDownload::GetContentLength(const std::string& strUrl, CURL_HEADER_INFO& headerInfo)
{
    CURL* handle = curl_easy_init();
    if (NULL == handle)
    {
        return (size_t)(0);
    }

    headerInfo.clear();

    curl_easy_setopt(handle, CURLOPT_URL, strUrl.c_str());
    curl_easy_setopt(handle, CURLOPT_HEADER, 1L);    // ��Ҫ����ͷ���ݵ������� 

    // ����ͷ��д��ص�
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, _HeaderFunc);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, (void*)&headerInfo);

    curl_easy_setopt(handle, CURLOPT_NOBODY, 1L);    // ����Ҫbody  
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);

    curl_off_t downloadFileLenth = 0;
    CURLcode curlCode = CURLE_OK;

    // ���Լ���, ���ܻ���ֳ�ʱCURLE_OPERATION_TIMEDOUT
    for (int i = 0; i < 10; i++)
    {
        curlCode = curl_easy_perform(handle);
        if (CURLE_OK == curlCode)
        {
            curlCode = curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &downloadFileLenth);
            long response_code = 0;
            curlCode = curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code < 200 || response_code >= 300)
            {
                downloadFileLenth = 0;
            }

            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    (void)curl_easy_cleanup(handle);

    return (size_t)downloadFileLenth;
}

size_t CCurlDownload::GetContentLength(const std::string& strUrl)
{
    CURL_HEADER_INFO headerInfo;
    return GetContentLength(strUrl, headerInfo);
}

void CCurlDownload::SetDownloadInfo(const std::string& strUrl, const std::string& strPath, int nConcurrency)
{
    (void)WaitForDownload();

    m_bCancel = false;
    m_strUrl = strUrl;
    m_strPath = strPath;
    m_bDownloading = false;
    m_lastDownload = 0;
    m_lastTotal = 0;

    if (nConcurrency <= 0)
    {
        nConcurrency = 1;
    }

    m_nConcurrency = nConcurrency;
}

bool CCurlDownload::_IsSupportAcceptRanges(const CURL_HEADER_INFO& headerInfo)
{
    auto itKey = headerInfo.find("Accept-Ranges");
    if (itKey != headerInfo.end())
    {
        if (itKey->second == "none")
        {
            return false;
        }

        // ����Ƿ�֧�ֶַ�����(��ϵ�����)
        if (itKey->second == "bytes")
        {
            return true;
        }
    }

    return false;
}

bool CCurlDownload::_InitCurlInfo(PackagePart* pPackageInfo, const std::string& strUrl)
{
    if (nullptr == pPackageInfo)
    {
        return false;
    }

    CURL* curl = curl_easy_init();
    if (NULL == curl)
    {
        return false;
    }

    pPackageInfo->curl = curl;

    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());                            // ��������URL
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _WriteFunc);                      // �����ļ�д��ص�
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(pPackageInfo));    // �����ļ�д��ص�����
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);                                 // �������ؽ��Ȼص�
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, _ProgressFunc);                // �������ؽ��Ȼص�
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, static_cast<void*>(pPackageInfo)); // �������ؽ��Ȼص�����
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);                                   // ָʾ libcurl ��Ҫʹ���κ��ź�/����������򣬼�ʹ��ʹ�ó�ʱʱʱҲ�����
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);                            // �� �ֽ�/�� Ϊ��λ�ĵ��ٶ�����
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 1L);                             // ��������ʱ���
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 60);                             // �������ӳ�ʱ����
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60000);                                 // ������������ɵ��ʱ��
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);                             // ����֤��
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);                             // ���������Ƿ�Ӧ�ô� ssl �еĶԵ�֤������֤������
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);                             // �����ض���
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);                                    // �����Ƿ��ӡ libcurl ִ�й���

    // �����������ݷ�Χ
    char range[MAX_PATH] = { 0 };

    // δ֪��С���ɶϵ�����, ʧ�ܾ�ֻ�ܴ�ͷ��ʼ
    if ((size_t)(-1) == pPackageInfo->endPos)
    {
        pPackageInfo->beginPos = 0;
        curl_easy_setopt(curl, CURLOPT_RANGE, 0L);
        ::SetFilePointer(pPackageInfo->hFile, 0, 0, FILE_BEGIN);
        ::SetEndOfFile(pPackageInfo->hFile);
    }
    else
    {
        snprintf(range, sizeof(range), "%lld-%lld", pPackageInfo->beginPos, pPackageInfo->endPos);
        curl_easy_setopt(curl, CURLOPT_RANGE, range);
    }

    return true;
}

void CCurlDownload::_TruncateFile(const std::string& strPath)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    hFile = ::CreateFileA(strPath.c_str(),
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
        NULL,
        TRUNCATE_EXISTING,
        FILE_ATTRIBUTE_ARCHIVE,
        NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        ::CloseHandle(hFile);
    }
}

bool CCurlDownload::_BackupFile(const std::string& strPath)
{
    char szBuf[MAX_PATH] = { 0 };
    std::string strBakFile;
    bool isBackupOk = false;

    if (0 == BACKUP_MAX_CONUT)
    {
        return true;
    }

    for (int i = 1; i <= BACKUP_MAX_CONUT; i++)
    {
        sprintf_s(szBuf, _countof(szBuf), "%02d", i);
        strBakFile = strPath + "." + szBuf + ".bak";
        if (::MoveFileA(strPath.c_str(), strBakFile.c_str()))
        {
            isBackupOk = true;
            break;
        }

        if (ERROR_FILE_NOT_FOUND == ::GetLastError())
        {
            isBackupOk = true;
            break;
        }
    }

    //����ʧ��, ��ɾ������һ�������ļ�
    if (!isBackupOk)
    {
        std::string strOld;
        std::string strNew;

        sprintf_s(szBuf, _countof(szBuf), "%02d", 1);
        strOld = strPath + "." + szBuf + ".bak";

        // �޷�ɾ��
        if (!::DeleteFileA(strOld.c_str()))
        {
            return false;
        }

        for (int i = 1; i < BACKUP_MAX_CONUT; i++)
        {
            sprintf_s(szBuf, _countof(szBuf), "%02d", i);
            strNew = strPath + "." + szBuf + ".bak";;
            sprintf_s(szBuf, _countof(szBuf), "%02d", i + 1);
            strOld = strPath + "." + szBuf + ".bak";;
            if (!::MoveFileA(strOld.c_str(), strNew.c_str()))
            {
                break;
            }
        }

        sprintf_s(szBuf, _countof(szBuf), "%02d", BACKUP_MAX_CONUT);
        strBakFile = strPath + "." + szBuf + ".bak";
        if (::MoveFileA(strPath.c_str(), strBakFile.c_str()))
        {
            isBackupOk = true;
        }
    }

    return isBackupOk;
}

bool CCurlDownload::_DownloadByParts(long long nTotalSize, long long packageSize, int nConcurrency)
{
    std::string strTmpFile = m_strPath + ".tmp";
    bool bDownloadFinish = true;

    do
    {
        ThreadPool pool(nConcurrency);
        std::string strBakFile;

        // �����ļ�
        if (!_BackupFile(m_strPath))
        {
            bDownloadFinish = false;
            break;
        }

        // ɾ���ļ�, ��ֹ���غ������ͻ
        if (!::DeleteFileA(m_strPath.c_str()))
        {
            if (ERROR_FILE_NOT_FOUND != ::GetLastError())
            {
                bDownloadFinish = false;
                break;
            }
        }

        // ���һ����ʱ�ļ�(������ڵĻ�)
        (void)_TruncateFile(strTmpFile);

        // �ְ�����
        for (int i = 0; i < nConcurrency; i++)
        {
            HANDLE hFile = INVALID_HANDLE_VALUE;
            hFile = ::CreateFileA(strTmpFile.c_str(),
                GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_ARCHIVE,
                NULL);

            if (INVALID_HANDLE_VALUE == hFile)
            {
                m_bCancel = true;
                break;
            }

            PackagePart* pPackageInfo = new (std::nothrow) PackagePart();
            if (NULL == pPackageInfo)
            {
                m_bCancel = true;
                break;
            }

            pPackageInfo->partId = i;
            pPackageInfo->hFile = hFile;
            pPackageInfo->pThis = this;

            if (1 == nConcurrency)
            {
                pPackageInfo->beginPos = 0;
                pPackageInfo->endPos = nTotalSize - 1;
                pPackageInfo->totalSize = nTotalSize;

                if ((size_t)(-1) == nTotalSize)
                {
                    pPackageInfo->endPos = nTotalSize;
                }
            }
            else
            {
                pPackageInfo->beginPos = i * packageSize;
                pPackageInfo->endPos = (i < m_nConcurrency - 1) ? ((i + 1) * packageSize - 1) : (nTotalSize - 1);
                pPackageInfo->totalSize = (pPackageInfo->endPos - pPackageInfo->beginPos) + 1;
            }

            LARGE_INTEGER liDistanceToMove = { 0 };
            liDistanceToMove.QuadPart = pPackageInfo->beginPos;

            // �����ļ�д��ָ��λ��
            ::SetFilePointerEx(pPackageInfo->hFile, liDistanceToMove, NULL, FILE_BEGIN);

            m_packageParts.push_back(pPackageInfo);

            // ��������
            auto workFunc = [this, pPackageInfo]() -> void {
                _TransferData(pPackageInfo);
                m_threadCount--;
                };

            try
            {
                pool.enqueue(workFunc);
            }
            catch (...)
            {
                _tprintf(_T("download excetion"));
            }

            m_threadCount++;
        }
    } while (false);

    // �ȴ������߳̽���
    while (m_threadCount)
    {
        _SleepMillisecond(100);
    }

    // ���ֶ����ؽ��, ����һ��ʧ������Ϊȫ����ʧ��
    for (auto& item : m_packageParts)
    {
        if (!item->bSuccess)
        {
            bDownloadFinish = false;
        }
        delete item;
    }

    m_packageParts.clear();

    m_bDownloadSuccess = bDownloadFinish;
    m_bCancel = false;

    // ����������޸��ļ���
    if (bDownloadFinish)
    {
        ::MoveFileA(strTmpFile.c_str(), m_strPath.c_str());
    }

    return bDownloadFinish;
}

bool CCurlDownload::_TransferData(PackagePart* pPackageInfo)
{
    // ����30��
    for (int i = 0; i < 30 && !m_bCancel; i++)
    {
        if (!_InitCurlInfo(pPackageInfo, m_strUrl))
        {
            break;
        }

        CURLcode res = curl_easy_perform(pPackageInfo->curl);

        // ����ȡ�����ٳ���
        if (CURLE_ABORTED_BY_CALLBACK == res)
        {
            (void)curl_easy_cleanup(pPackageInfo->curl);
            break;
        }

        if (res == CURLE_OK)
        {
            long response_code = 0;
            curl_easy_getinfo(pPackageInfo->curl, CURLINFO_RESPONSE_CODE, &response_code);

            if (404 == response_code)
            {
                _tprintf(_T("curl_easy_getinfo failed rescode: %ld"), response_code);
                break;
            }
            else if (response_code < 200 || response_code > 300)
            {
                _tprintf(_T("curl_easy_getinfo failed rescode: %ld"), response_code);
            }
            else
            {
                (void)curl_easy_cleanup(pPackageInfo->curl);
                pPackageInfo->bSuccess = true;
                break;
            }
        }
        else
        {
            _tprintf(_T("curl_easy_perform failed res: %d: %S"), res, curl_easy_strerror(res));
        }

        (void)curl_easy_cleanup(pPackageInfo->curl);
        _SleepMillisecond(500);
        _tprintf(_T("retry: %d"), i + 1);
    }

    ::CloseHandle(pPackageInfo->hFile);

    return true;
}

bool CCurlDownload::SyncDownload(std::function<void(long long dlNow, long long dlTotal, double lfProgress)> cb)
{
    long long nPagePackageSize = 0;
    bool isSuccess = false;

    // �ȴ������߳̽���
    (void)WaitForDownload();

    CURL_HEADER_INFO headerInfo;

    // ��ȡ���ݴ�С
    m_lastTotal = GetContentLength(m_strUrl, headerInfo);

    // ����0��ʾʧ��
    if (((size_t)0) == m_lastTotal)
    {
        return false;
    }

    int nConcurrency = m_nConcurrency;

    //��֧�ֶϵ�����������߳�����
    if (!_IsSupportAcceptRanges(headerInfo))
    {
        nConcurrency = 1;
    }

    //�ļ���Сδ֪������߳�����
    if ((size_t)(-1) == m_lastTotal)
    {
        nConcurrency = 1;
        nPagePackageSize = (size_t)(-1);
    }
    else
    {
        if (m_lastTotal <= MINIMUM_PACKAGE_SIZE)
        {
            nConcurrency = 1;
        }

        // ��ȡ�ְ���С
        nPagePackageSize = m_lastTotal / nConcurrency;
    }

    m_cbProgress = cb;

    m_bDownloading = true;
    isSuccess = _DownloadByParts(m_lastTotal, nPagePackageSize, nConcurrency);
    m_bDownloading = false;

    return isSuccess;
}

// �첽�����ļ�
void CCurlDownload::AsyncDownload(std::function<void(long long dlNow, long long dlTotal, double lfProgress)> cb)
{
    // �ȵȴ�����״̬����
    WaitForDownload();

    std::promise<bool> promiseReady;
    std::future<bool> futureReady(promiseReady.get_future());

    std::thread([this, cb](std::promise<bool>& fReady) -> void {

        size_t nPagePackageSize = 0;
        bool isSuccess = false;

        // �ȴ������߳̽���
        (void)WaitForDownload();

        // ��ȡ���ݴ�С
        CURL_HEADER_INFO headerInfo;
        m_lastTotal = GetContentLength(m_strUrl, headerInfo);
        if (((size_t)-1) == m_lastTotal)
        {
            fReady.set_value(true);
            return;
        }

        int nConcurrency = m_nConcurrency;
        //��֧�ֶϵ�����������߳�����
        if (!_IsSupportAcceptRanges(headerInfo))
        {
            nConcurrency = 1;
        }

        //�ļ���Сδ֪������߳�����
        if ((size_t)(-1) == m_lastTotal)
        {
            nConcurrency = 1;
            nPagePackageSize = (size_t)(-1);
        }
        else
        {
            if (m_lastTotal <= MINIMUM_PACKAGE_SIZE)
            {
                nConcurrency = 1;
            }

            // ��ȡ�ְ���С
            nPagePackageSize = (size_t)(m_lastTotal / nConcurrency);
        }

        m_cbProgress = cb;

        m_bDownloading = true;
        fReady.set_value(true);
        isSuccess = _DownloadByParts(m_lastTotal, nPagePackageSize, nConcurrency);
        m_bDownloading = false;

        }, ref(promiseReady)).detach();

    futureReady.get();
}

bool CCurlDownload::IsDownloading() const
{
    return m_bDownloading;
}

bool CCurlDownload::GetLastDownloadResult() const
{
    return m_bDownloadSuccess;
}

void CCurlDownload::CancelDownload()
{
    m_bCancel = true;
    (void)WaitForDownload();
}

// �ȴ�������״̬
void CCurlDownload::WaitForDownload() const
{
    // �ȴ������߳̽���
    while (m_bDownloading)
    {
        _SleepMillisecond(100);
    }
}

long long CCurlDownload::GetDownloadingSize() const
{
    return m_lastDownload;
}

long long CCurlDownload::GetDownloadingTotal() const
{
    return m_lastTotal;
}

size_t CCurlDownload::_HeaderFunc(char* buffer, size_t size, size_t nitems, void* pUserData)
{
    size_t realSize = size * nitems;

    std::map<std::string, std::string>* pHeaderContent = (std::map<std::string, std::string>*)pUserData;

    std::string strContent = buffer;

    size_t nPos = strContent.find(':');

    if (std::string::npos != nPos)
    {
        std::string strKey = strContent.substr(0, nPos);

        std::string strValue = strContent.substr(nPos + 2);
        size_t nReturnPos = strValue.rfind('\r');
        if (std::string::npos != nReturnPos)
        {
            strValue.resize(nReturnPos);
        }

        pHeaderContent->insert(std::make_pair(strKey, strValue));
    }

    return realSize;
}

size_t CCurlDownload::_WriteFunc(void* pBuffer, size_t size, size_t nmemb, void* pUserData)
{
    size_t realSize = size * nmemb;
    if (NULL == pBuffer || NULL == pUserData)
    {
        return realSize;
    }

    PackagePart* pPackageInfo = static_cast<PackagePart*>(pUserData);
    CCurlDownload* pThis = pPackageInfo->pThis;

    return pThis->_WriteFile(pBuffer, (DWORD)realSize, pPackageInfo);
}

size_t CCurlDownload::_WriteFile(const void* pBuffer, DWORD size, PackagePart* pPackageInfo)
{
    DWORD dwWritten = 0;
    ::WriteFile(pPackageInfo->hFile, pBuffer, (DWORD)size, &dwWritten, NULL);
    pPackageInfo->downloadSize += dwWritten;
    pPackageInfo->beginPos += dwWritten;
    return dwWritten;
}

int CCurlDownload::_ProgressFunc(void* pUserData, curl_off_t dlTotal, curl_off_t dlNow, curl_off_t ulTotal, curl_off_t ulNow)
{
    UNREFERENCED_PARAMETER(ulTotal);
    UNREFERENCED_PARAMETER(ulNow);

    if (NULL == pUserData || 0 == dlTotal)
    {
        return CURLE_OK;
    }

    PackagePart* pPackageInfo = static_cast<PackagePart*>(pUserData);
    CCurlDownload* pThis = pPackageInfo->pThis;

    return pThis->_ProgressProc(pPackageInfo, dlTotal, dlNow);
}

int CCurlDownload::_ProgressProc(PackagePart* pPackageInfo, long long dlTotal, long long dlNow)
{
    UNREFERENCED_PARAMETER(pPackageInfo);
    UNREFERENCED_PARAMETER(dlTotal);
    UNREFERENCED_PARAMETER(dlNow);

    long long download = 0;

    // ͳ�����������ݺ����ļ���С
    for (auto& item : m_packageParts)
    {
        download += item->downloadSize;
    }

    // ���ؽ��Ȼص�
    if (m_lastTotal > 0 && m_lastDownload != download && m_cbProgress)
    {
        m_cbProgress(download, m_lastTotal, (double)download / (double)m_lastTotal);
    }

    m_lastDownload = download;

    if (m_bCancel)
    {
        return -1;
    }

    return CURLE_OK;
}