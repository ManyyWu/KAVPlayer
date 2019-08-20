#ifndef _AVPLAYERWIDGET_FRAME_QUEUE_H_
#define _AVPLAYERWIDGET_FRAME_QUEUE_H_

#include "packet_queue.h"

extern "C"
{
#include "libavcodec/avcodec.h"
#include "SDL2/SDL.h"
}

/* queue length */
#define DEF_SAMPLEQ_LEN        10
#define DEF_PICTQ_LEN          10
#define DEF_SUBPICTQ_LEN       10
#define MAX_SAMPLEQ_LEN        100
#define MAX_PICTQ_LEN          100
#define MAX_SUBPICTQ_LEN       100

/* frame node */
typedef struct Frame {
    AVFrame *           frame;   // pointer of frame
    double              pts;      // pts of frame (unit: second)
    double              duration; // duration of frame (unit: second)
}Frame;

/* frame queue */
class FrameQueue {
private:
    Frame **            fq;     // pointer of queue
    int                 len;      // length of queue
    int                 rindex;   // read index
    int                 windex;   // write index
    int                 max_len;  // max length of queue
    SDL_mutex *         mutex;   // mutex
    SDL_cond *          cond;    // cond
    PacketQueue *       pktq;    // pointer of associated packet queue 

public:
    int    init    (PacketQueue *pktq, int max_len);
    int    put     (AVFrame *f, int64_t pos, double pts, double duration);
    Frame *get     ();
    Frame *peek    ();
    void   abort   ();
    void   clear   ();
    int    get_len ();
    bool   is_eof  ();

public:
    FrameQueue     ();
    ~FrameQueue    ();
};

#endif /* _AVPLAYERWIDGET_FRAME_QUEUE_H_ */
