#include "CCurlDownload.h"
#include "ThreadPool.h"
#include <tchar.h>
#include <future>
#include <thread>

#define MINIMUM_PACKAGE_SIZE        (16384) // 最小包大小
#define BACKUP_MAX_CONUT            (0)     // 最多备份数量

//#pragma comment(lib, "curl/lib/libcurl.lib")

typedef struct _PackagePart
{
    bool bSuccess;          //下载是否成功
    CCurlDownload* pThis;   //对象指针
    int partId;             //分段ID
    HANDLE hFile;           //文件句柄
    long long beginPos;     //起始偏移
    long long endPos;       //结束偏移
    long long downloadSize; //已下载大小
    long long totalSize;    //包数据总大小
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
    curl_easy_setopt(handle, CURLOPT_HEADER, 1L);    // 需要将标头传递到数据流 

    // 设置头部写入回调
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, _HeaderFunc);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, (void*)&headerInfo);

    curl_easy_setopt(handle, CURLOPT_NOBODY, 1L);    // 不需要body  
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);

    curl_off_t downloadFileLenth = 0;
    CURLcode curlCode = CURLE_OK;

    // 重试几次, 可能会出现超时CURLE_OPERATION_TIMEDOUT
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

        // 检查是否支持分段下载(或断点续传)
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

    curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());                            // 设置链接URL
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _WriteFunc);                      // 设置文件写入回调
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(pPackageInfo));    // 设置文件写入回调参数
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);                                 // 启用下载进度回调
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, _ProgressFunc);                // 设置下载进度回调
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, static_cast<void*>(pPackageInfo)); // 设置下载进度回调参数
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);                                   // 指示 libcurl 不要使用任何信号/警报处理程序，即使在使用超时时时也是如此
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);                            // 以 字节/秒 为单位的低速度限制
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 1L);                             // 低速限制时间段
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 60);                             // 设置连接超时秒数
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60000);                                 // 设置允许传输完成的最长时间
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);                             // 忽略证书
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);                             // 设置我们是否应该从 ssl 中的对等证书中验证公用名
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);                             // 设置重定向
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);                                    // 设置是否打印 libcurl 执行过程

    // 设置请求数据范围
    char range[MAX_PATH] = { 0 };

    // 未知大小不可断点续传, 失败就只能从头开始
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

    //备份失败, 则删除最早一个备份文件
    if (!isBackupOk)
    {
        std::string strOld;
        std::string strNew;

        sprintf_s(szBuf, _countof(szBuf), "%02d", 1);
        strOld = strPath + "." + szBuf + ".bak";

        // 无法删除
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

        // 备份文件
        if (!_BackupFile(m_strPath))
        {
            bDownloadFinish = false;
            break;
        }

        // 删除文件, 防止下载后改名冲突
        if (!::DeleteFileA(m_strPath.c_str()))
        {
            if (ERROR_FILE_NOT_FOUND != ::GetLastError())
            {
                bDownloadFinish = false;
                break;
            }
        }

        // 清空一下临时文件(如果存在的话)
        (void)_TruncateFile(strTmpFile);

        // 分包下载
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

            // 设置文件写入指针位置
            ::SetFilePointerEx(pPackageInfo->hFile, liDistanceToMove, NULL, FILE_BEGIN);

            m_packageParts.push_back(pPackageInfo);

            // 工作函数
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

    // 等待下载线程结束
    while (m_threadCount)
    {
        _SleepMillisecond(100);
    }

    // 检查分段下载结果, 存在一个失败则认为全部是失败
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

    // 下载完成则修改文件名
    if (bDownloadFinish)
    {
        ::MoveFileA(strTmpFile.c_str(), m_strPath.c_str());
    }

    return bDownloadFinish;
}

bool CCurlDownload::_TransferData(PackagePart* pPackageInfo)
{
    // 重试30次
    for (int i = 0; i < 30 && !m_bCancel; i++)
    {
        if (!_InitCurlInfo(pPackageInfo, m_strUrl))
        {
            break;
        }

        CURLcode res = curl_easy_perform(pPackageInfo->curl);

        // 主动取消则不再尝试
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

    // 等待下载线程结束
    (void)WaitForDownload();

    CURL_HEADER_INFO headerInfo;

    // 获取内容大小
    m_lastTotal = GetContentLength(m_strUrl, headerInfo);

    // 返回0表示失败
    if (((size_t)0) == m_lastTotal)
    {
        return false;
    }

    int nConcurrency = m_nConcurrency;

    //不支持断点续传则仅单线程下载
    if (!_IsSupportAcceptRanges(headerInfo))
    {
        nConcurrency = 1;
    }

    //文件大小未知则仅单线程下载
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

        // 获取分包大小
        nPagePackageSize = m_lastTotal / nConcurrency;
    }

    m_cbProgress = cb;

    m_bDownloading = true;
    isSuccess = _DownloadByParts(m_lastTotal, nPagePackageSize, nConcurrency);
    m_bDownloading = false;

    return isSuccess;
}

// 异步下载文件
void CCurlDownload::AsyncDownload(std::function<void(long long dlNow, long long dlTotal, double lfProgress)> cb)
{
    // 先等待下载状态结束
    WaitForDownload();

    std::promise<bool> promiseReady;
    std::future<bool> futureReady(promiseReady.get_future());

    std::thread([this, cb](std::promise<bool>& fReady) -> void {

        size_t nPagePackageSize = 0;
        bool isSuccess = false;

        // 等待下载线程结束
        (void)WaitForDownload();

        // 获取内容大小
        CURL_HEADER_INFO headerInfo;
        m_lastTotal = GetContentLength(m_strUrl, headerInfo);
        if (((size_t)-1) == m_lastTotal)
        {
            fReady.set_value(true);
            return;
        }

        int nConcurrency = m_nConcurrency;
        //不支持断点续传则仅单线程下载
        if (!_IsSupportAcceptRanges(headerInfo))
        {
            nConcurrency = 1;
        }

        //文件大小未知则仅单线程下载
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

            // 获取分包大小
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

// 等待到空闲状态
void CCurlDownload::WaitForDownload() const
{
    // 等待下载线程结束
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

    // 统计已下载数据和总文件大小
    for (auto& item : m_packageParts)
    {
        download += item->downloadSize;
    }

    // 下载进度回调
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