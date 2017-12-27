#ifndef VIDEO_CORE_H
#define VIDEO_CORE_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
}

#include <QObject>
#include <QTimer>
#include <QThread>
#include <vector>
#include "utils/video_bufqueue.h"

class video_core : public QObject
{
    Q_OBJECT
public:
    explicit video_core(int num=0,video_bufQueue** bufQueue=NULL,QObject *parent = nullptr);
    typedef enum
    {
        VideoCrtl_PLAY=0,
        VideoCrtl_STOP
    }VideoCtrlCmd;
    typedef enum
    {
        VideoStatus_OPEN=0,
        VideoStatus_TERMINATE
    }VideoStatusCmd;
private:
    void videoInit();
    bool getVideoInfo(QString url);
    void videoRun();
    void videoControl(VideoCtrlCmd cmd);

    std::vector<video_bufQueue*> m_vecBufQueue;
    QTimer m_tStopTimer;

    AVFormatContext* m_pFormatContext=NULL;
    AVCodecContext* m_pCodecContext=NULL;
    SwsContext* m_pSwsContext=NULL;
    AVPacket* m_pPacket = NULL;
    AVCodec* m_pCodec = NULL;
    AVFrame* m_pFrame=NULL;
    AVFrame* m_pFrameGrey=NULL;
    uint8_t* m_pFrameBuffer=NULL;
    int m_nVideoStram=-1;
    QString m_strUrl = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov";

    VideoStatusCmd m_bState=VideoStatus_TERMINATE;
    VideoCtrlCmd m_bContrl=VideoCrtl_STOP;

signals:
    void sendToLog(QString);
    void getImage();
    void readyToClose();
public slots:
    void videoOpen(QString url);
private slots:
    void videoClose();

};

#endif // VIDEO_CORE_H
