#pragma once

#include "CCurlDownload.h"

//���ļ����ع���
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

    // ����
    void SleepMillisecond(int millisecond) const;

private:
    std::vector<CCurlDownload> m_downloadTasks;         //�����������
    std::function<void(long long dlNow, long long dlTotal, double lfProgress)> m_cbProgress;     //���ؽ��Ȼص�
    bool m_IsCancel;                                    //�Ƿ�ȡ��
};