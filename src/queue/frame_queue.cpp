#include "frame_queue.h"
#include "packet_queue.h"
#include "error/error.h"
#include "log/log.h"
#include <new>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "SDL2/SDL.h"
}

#define FILENAME "frame_queue.cpp"

FrameQueue::FrameQueue ()
{
    /* init all variables */
    memset(this, 0, sizeof(FrameQueue));
}

FrameQueue::~FrameQueue ()
{
    /* clear all nodes */
    if (!this->len)
        this->clear();

    if (this->fq)
        delete[] fq;

    /* destroy condition variables and mutex */
    if (this->cond)
        SDL_DestroyCond(this->cond);
    if (this->mutex)
        SDL_DestroyMutex(this->mutex);
}

int FrameQueue::init (PacketQueue *pktq, int max_len)
{
    /* create condition variables and mutex */
    this->mutex = SDL_CreateMutex();
    if (!this->mutex) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_MUTEX_FAIL), SDL_GetError());
        return KERROR(KECREATE_SDL_MUTEX_FAIL);
    }
    this->cond = SDL_CreateCond();
    if (!this->cond) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_COND_FAIL), SDL_GetError());
        SDL_DestroyMutex(this->mutex);
        return KERROR(KECREATE_SDL_COND_FAIL);
    }

    /* set associated packet queue */
    this->pktq = pktq;

    /* init queue */
    this->max_len = max_len;
    this->fq = _New Frame *[max_len];
    if (!this->fq) {
        return AVERROR(ENOMEM);
    }
    memset(fq, 0, max_len * sizeof(Frame *));

    return 0;
}

int FrameQueue::put (AVFrame *f, int64_t pos,
                     double pts, double duration)
{
    Frame *temp = NULL;
    int    ret = 0;

    if (!f)
        return KERROR(KEINVAL);

    /* enter the critical aera */
    SDL_LockMutex(this->mutex);

    /* get an abort requestion */
    if (this->pktq->abort_req)
        goto fail; // (ret == 0)

    /* create a new node and put it to tail of frame queue */
    if (this->len < this->max_len) {
        temp = _New Frame();
        if (!temp) {
            GOTO_FAIL(KENOMEM);
        }
        temp->frame = f; // don't copy
        temp->pts = pts;
        temp->duration = duration;
        this->fq[this->windex] = temp;
        if (++this->windex == this->max_len)
            this->windex = 0;
        this->len++;
    } else {
        GOTO_FAIL(KEAGAIN);
    }

fail:
    /* leave the critical aera */
    SDL_CondSignal(this->cond);
    SDL_UnlockMutex(this->mutex);

    return ret;
}

Frame *FrameQueue::get ()
{
    Frame *ret = NULL;

    /* enter the critical aera */
    SDL_LockMutex(this->mutex);

    /* get the queue head node from queue head, blocked */
    while (!this->pktq->abort_req) {
        if (this->len) {
            ret = this->fq[this->rindex];
            this->fq[this->rindex] = NULL;
            if (++this->rindex == this->max_len)
                this->rindex = 0;
            this->len--;
            break;
        } else { // (len == 0)
            /* return NULL when the play is over */
            if (!this->len && !this->pktq->len && this->pktq->read_eof)
                break;

            /* waiting until (len != 0) or aborted */
            SDL_CondWait(this->cond, this->mutex);
        }
    }

    /* leave the critical aera */
    SDL_CondSignal(this->cond);
    SDL_UnlockMutex(this->mutex);

    return ret;
}

Frame * FrameQueue::peek ()
{
    Frame *ret = NULL;

    /* enter the critical aera */
    SDL_LockMutex(this->mutex);

    /* get the queue head node from queue head, blocked */
    while (!this->pktq->abort_req) {
        if (this->len) {
            ret = this->fq[this->rindex];
            break;
        } else { // (len == 0)
            /* return NULL when the play is over */
            if (!this->len && !this->pktq->len && this->pktq->read_eof)
                break;

            /* waiting until (len != 0) or aborted */
            SDL_CondWait(this->cond, this->mutex);
        }
    }

    /* leave the critical aera */
    SDL_CondSignal(this->cond);
    SDL_UnlockMutex(this->mutex);

    return ret;
}

void FrameQueue::abort ()
{
    SDL_LockMutex(this->mutex);
    SDL_CondSignal(this->cond);
    SDL_UnlockMutex(this->mutex);
}

void FrameQueue::clear ()
{
    int i = 0; 

    /* enter the critical aera */
    SDL_LockMutex(this->mutex);

    /* clear all nodes */
    while (i < max_len) {
        if (this->fq[i]) // equal to "if (this->fq[i] && this->fq[i]->frame)" in this application
            av_frame_free(&this->fq[i]->frame);
        delete this->fq[i];
        this->fq[i] = NULL;
        i++;
    }

    /* reset all information */
    this->len = 0;
    this->windex = 0;
    this->rindex = 0;

    /* leave the critical aera */
    SDL_UnlockMutex(this->mutex);
}

int FrameQueue::get_len ()
{
    return this->len;
}

bool FrameQueue::is_eof()
{
    return (!this->len && !this->pktq->len && this->pktq->read_eof);
}
