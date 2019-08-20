#ifndef _AVPLAYERWIDGET_DECODER_H_
#define _AVPLAYERWIDGET_DECODER_H_

#include <QObject>
#include "queue/packet_queue.h"
#include "queue/frame_queue.h"
#include "error/error.h"
#include "clock/clock.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "SDL2/SDL.h"
}

class Decoder : public QObject {
    Q_OBJECT

private:
    /* thread */
    SDL_Thread *     dec_thr;

    /* mutex and condition variable */
    SDL_mutex *      wait_mutex;
    SDL_cond *       empty_queue_cond;
    SDL_mutex *      pause_mutex;
    SDL_cond *       pause_cond;

    /* context */
    AVFormatContext *avfctx;
    AVCodecContext * avctx;
    AVCodec *        codec;

    /* queues */
    PacketQueue *    pktq;
    FrameQueue *     fq;

    /* stream */
    AVStream *       st;
    int              st_idx;
    
    /* decoder state */
    bool             abort_req;
    bool             pause_req;
    bool             paused;
    Clock            clk;

    /* seek */
    bool             seek_req;
    bool             seeking;
    double           seek_pos;
    double           incr;
 
signals:
    void               err_occured    (int);

private:
    static int SDLCALL vdec_thread    (void *args);
    static int SDLCALL adec_thread    (void *args);

private:
    int                decode_packets (AVFrame *f);

public:
    int                init           (Clock clk);
    void               close          ();
    void               start          ();
    void               pause          ();
    void               seek           (double pos);
    void               flush          ();
    bool               is_seeking     () const;

public:
    Decoder   (AVFormatContext *avfctx, int st_idx, 
               PacketQueue *pktq, FrameQueue *fq, 
               SDL_mutex *wait_mutex, SDL_cond *empty_queue_cond);
    ~Decoder  ();
};

#endif /* _AVPLAYERWIDGET_DECODER_H_ */
