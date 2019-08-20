#ifndef _AVPLAYERWIDGET_READER_H_
#define _AVPLAYERWIDGET_READER_H_

#include <QObject>
#include "queue/packet_queue.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "SDL2/SDL.h"
}

class Demux : public QObject {
    Q_OBJECT

private:
    /* threads */
    SDL_Thread *     demux_thr;

    /* format context */
    AVFormatContext *avfctx;

    /* state */
    bool             read_eof;
    bool             paused;
    bool             abort_req;
    bool             pause_req;
    bool             infinite_buf;
    int              max_pktq_size;

    /* mutex and condition variable */
    SDL_mutex *      wait_mutex;
    SDL_cond *       continue_read_cond;

    /* queues */     
    PacketQueue *    vpktq;
    PacketQueue *    apktq;

    /* stream index */
    int              vst_idx;
    int              ast_idx;
    AVStream *       vst;
    AVStream *       ast;

signals:
    void               err_occured  (int);

private:
    static int SDLCALL demux_thread (void *args);

public:
    int                init         ();
    void               close        ();
    void               start        ();
    void               pause        ();
    int                seek         (double pos);

public:
    Demux                           (AVFormatContext *avfctx, 
                                     PacketQueue *vpktq, PacketQueue *apktq,
                                     SDL_mutex *wait_mutex, SDL_cond *continue_read_cond,
                                     int vst_idx, int ast_idx,
                                     bool infinite_buf, int max_pktq_size);
    ~Demux                          ();
};

#endif /* _AVPLAYERWIDGET_READER_H_ */
