#include "CCurlDownloadMgr.h"
#include <time.h>
#include <algorithm>
#include <thread>

CCurlDownloadMgr::CCurlDownloadMgr()
{
    m_IsCancel = false;
}

CCurlDownloadMgr::~CCurlDownloadMgr()
{
    CancelDownload();
}

void CCurlDownloadMgr::PushBack(const CCurlDownload& task)
{
    m_downloadTasks.push_back(task);
}

void CCurlDownloadMgr::Clear()
{
    m_downloadTasks.clear();
}

bool CCurlDownloadMgr::StartDownload(std::function<void(long long dlNow, long long dlTotal, double lfProgress)> cb)
{
    for (auto& item : m_downloadTasks)
    {
        item.AsyncDownload();
    }

    while (true)
    {
        //检查是否有线程还在下载
        bool isDownloading = std::any_of(m_downloadTasks.begin(), m_downloadTasks.end(), [&isDownloading](const CCurlDownload& item)
            {
                return item.IsDownloading();
            });

        //统计下载任务下载进度
        long long nDownload = 0;
        long long nTotal = 0;

        for (const auto& item : m_downloadTasks)
        {
            nDownload += item.GetDownloadingSize();
            nTotal += item.GetDownloadingTotal();
        }

        if (cb && nTotal > 0)
        {
            cb(nDownload, nTotal, (double)nDownload / (double)nTotal);
        }

        //非下载状态及时退出
        if (!isDownloading)
        {
            break;
        }

        SleepMillisecond(100);
    }

    //检查是否每个任务都下载完成
    bool isDownloadFinish = std::all_of(m_downloadTasks.begin(), m_downloadTasks.end(), [](const CCurlDownload& item)
        {
            return item.GetLastDownloadResult();
        });

    return isDownloadFinish;
}

void CCurlDownloadMgr::CancelDownload()
{
    m_IsCancel = true;
    for (auto& item : m_downloadTasks)
    {
        item.CancelDownload();
    }
}

void CCurlDownloadMgr::SleepMillisecond(int millisecond) const
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

        if (m_IsCancel)
        {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(span));
    }
}