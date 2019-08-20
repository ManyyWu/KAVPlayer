#include "packet_queue.h"
#include "error/error.h"
#include "log/log.h"
#include <new>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "SDL2/SDL.h"
}

#define FILENAME "packet_queue.cpp"

PacketQueue::PacketQueue ()
{ 
    /* init all variables */
    memset(this, 0, sizeof(PacketQueue)); 
}

PacketQueue::~PacketQueue ()
{
    /* clear all nodes */
    if (mutex)
        clear();

    /* destroy condition variables and mutex */
    if (cond)
        SDL_DestroyCond(cond);
    if (mutex)
        SDL_DestroyMutex(mutex);
}

int PacketQueue::init ()
{
    /* create condition variables and mutex */
    mutex = SDL_CreateMutex();
    if (!mutex) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_MUTEX_FAIL), SDL_GetError());
        return KERROR(KECREATE_SDL_MUTEX_FAIL);
    }
    cond = SDL_CreateCond();
    if (!cond) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_COND_FAIL), SDL_GetError());
        SDL_DestroyMutex(mutex);
        return KERROR(KECREATE_SDL_COND_FAIL);
    }

	return 0;
}

int PacketQueue::put (AVPacket *pkt)
{
    Packet *temp;
    int     ret = 0;

    if (!pkt)
        return KERROR(KEINVAL);

    /* enter the critical area */
    SDL_LockMutex(mutex);

    /* get an abort requestion */
    if (abort_req)
        goto fail; // (ret == 0)
    
    /* create a new node and put it to tail of packet queue */
    temp = _New Packet();
    if (!temp) {
        GOTO_FAIL(KENOMEM);
    }
    temp->pkt = pkt; // don't copy 
    temp->next = NULL;
    if (!tail) // equal to (len == 0)
        head = temp;
    else 
        tail->next = temp;
    tail = temp;
    len++;
    size += pkt->size;
    duration += pkt->duration;

fail:
    /* leave the critical area */
    SDL_CondSignal(cond);
    SDL_UnlockMutex(mutex);

    return ret;
}

AVPacket *PacketQueue::get ()
{
    AVPacket *ret = NULL;
    Packet   *temp = tail;

    /* enter the critical area */
    SDL_LockMutex(mutex);

    /* get the queue head node from queue head, blocked */
    while (!abort_req) {
        temp = head;
        if (temp) {
            head = head->next;
            if (!head) 
                tail = NULL;
            ret = temp->pkt;
            delete temp;
            len--;
            size -= ret->size;
            duration -= ret->duration;
            break;
        } else { // (len == 0)
            if (!len && read_eof)
                break;
            
            /* waiting until (len != 0) or aborted */
            SDL_CondWait(cond, mutex);
/*
*           blocking until seeked or aborted when (len == 0 && read_eof == 1)
*           if (!len && read_eof)
*               break;
*/
        }
    }

    /* leave the critical area */
    SDL_CondSignal(cond);
    SDL_UnlockMutex(mutex);

    return ret;
}

void PacketQueue::set_read_eof (bool is_read_eof)
{
    SDL_LockMutex(mutex);
    read_eof = is_read_eof;
    SDL_CondSignal(cond);
    SDL_UnlockMutex(mutex);
}

void PacketQueue::restore ()
{
    SDL_LockMutex(mutex);
    abort_req = false;
    SDL_CondSignal(cond);
    SDL_UnlockMutex(mutex);
}

void PacketQueue::abort ()
{
    SDL_LockMutex(mutex);
    abort_req = true;
    SDL_CondSignal(cond);
    SDL_UnlockMutex(mutex);
}

void PacketQueue::clear ()
{
    Packet *temp = head;

    /* enter the critical area */
    SDL_LockMutex(mutex);

    /* clear all nodes */
    while (head) {
        head = head->next;
        av_packet_free(&temp->pkt);
        delete temp;
        temp = head;
    }
	
	/* reset all infomations */
    head = tail = NULL;
    len = 0;
    size = 0;
    duration = 0;
    abort_req = false;
    read_eof = false;

    /* leave the critical area */
    SDL_UnlockMutex(mutex);
}

int PacketQueue::get_len ()
{
    return len;
}

int64_t PacketQueue::get_size ()
{
    return size;
}

int64_t PacketQueue::get_duration ()
{
    return duration;
}

bool PacketQueue::is_eof()
{
    return (!len && read_eof);
}

