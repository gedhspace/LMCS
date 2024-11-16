#pragma once

#include "CCurlDownload.h"

//多文件下载管理
class CCurlDownloadMgr
{
public:

    CCurlDownloadMgr();

    ~CCurlDownloadMgr();

    void PushBack(const CCurlDownload& task);

    void Clear();

    bool StartDownload(std::function<void(long long dlNow, long long dlTotal, double lfProgress)> cb = nullptr);

    void CancelDownload();

private:

    // 休眠
    void SleepMillisecond(int millisecond) const;

private:
    std::vector<CCurlDownload> m_downloadTasks;         //下载任务队列
    std::function<void(long long dlNow, long long dlTotal, double lfProgress)> m_cbProgress;     //下载进度回调
    bool m_IsCancel;                                    //是否取消
};