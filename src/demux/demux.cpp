#include "demux.h"
#include "decoder/decoder.h"
#include "error/error.h"
#include "log/log.h"

extern "C"
{
#include "libavutil/time.h"
}

#define FILENAME "demux.cpp"

int SDLCALL Demux::demux_thread (void* args)
{
    Demux *    d = (Demux *)args;
    AVPacket * pkt = NULL;
    int        ret;

    logger.debug("Demux thread started.\n");

    while (!d->abort_req) {
        /* read eof or get a pause requestion */
        if (d->read_eof || d->pause_req)
            goto wait;

        /* if packet queues is full, no need to read more */
        if ((d->vst ? d->vpktq->get_size() : 0) +
            (d->ast ? d->apktq->get_size() : 0) > (d->infinite_buf ? MAX_PKTQ_SIZE : d->max_pktq_size))
            {
            /* wait 10ms */
            SDL_LockMutex(d->wait_mutex);
            //logger.debug("Packet queue is full.\n");
            SDL_CondWait(d->continue_read_cond, d->wait_mutex);
            SDL_UnlockMutex(d->wait_mutex);
            continue;
        }

        /* read a frame */
        pkt = av_packet_alloc();
        if (!pkt) {
            ret = KERROR(KENOMEM);
fail:
            if (d->vpktq)
                d->vpktq->abort();
            if (d->apktq)
                d->apktq->abort();
            av_packet_free(&pkt);
            emit d->err_occured(ret);
            return ret;
        }
        ret = av_read_frame(d->avfctx, pkt);
        if (ret < 0) {
            av_packet_free(&pkt);
            if (ret == AVERROR_EOF || avio_feof(d->avfctx->pb)) {
                d->read_eof = true;
                if (d->vpktq)
                    d->vpktq->set_read_eof(true);
                if (d->apktq)
                    d->apktq->set_read_eof(true);
                logger.debug("Read eof.\n");
            } else {
                if (d->avfctx->pb && d->avfctx->pb->error)
                    logger.error("%s.\n", av_err2str(d->avfctx->pb->error));
                logger.error("%s: %s.\n", kerr2str(KEREAD_PACKET_FAIL), av_err2str(ret));
                ret = KERROR(KEREAD_PACKET_FAIL);
                goto fail;
            }
wait:
            SDL_LockMutex(d->wait_mutex);
            d->paused = true;
            logger.debug("Demux thread paused.\n");
            SDL_CondWait(d->continue_read_cond, d->wait_mutex);
            logger.debug("Demux thread resumed.\n");
            d->paused = false;
            SDL_UnlockMutex(d->wait_mutex);
            continue;
        }

        /* put packet to queue */
        if (d->vst && d->vst_idx == pkt->stream_index) {
            ret = d->vpktq->put(pkt);
            //logger.verbose("+vpktq:%d\n", d->vpktq->get_len());
        } else if (d->ast && d->ast_idx == pkt->stream_index) {
            ret = d->apktq->put(pkt);
            //logger.verbose("+apktq:%d\n", d->vpktq->get_len());
        } else {
            av_packet_free(&pkt);
        }
        if (ret < 0)
            goto fail;
        pkt = NULL;
    }

    logger.debug("Demux thread stopped.\n");
    return KERROR(KEABORTED);
}

int Demux::init ()
{
    int ret;

    if (demux_thr)
        return KERROR(KEREINIT);

    abort_req = false;
    pause_req = false;
    paused = false;
    read_eof = false;

    /* create demux thread */
    demux_thr = SDL_CreateThread(demux_thread, "demux_thread", this);
    if (!demux_thr) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_THREAD_FAIL), SDL_GetError());
        return KERROR(KECREATE_THREAD_FAIL);
    }
    logger.debug("Demux has been inited.\n");

    return 0;;
}

void Demux::close ()
{
    int ret;

    if (!demux_thr)
        return;

    /* stop demux thread */
    abort_req = true;
    SDL_LockMutex(wait_mutex);
    SDL_CondSignal(continue_read_cond);
    SDL_UnlockMutex(wait_mutex);
    SDL_WaitThread(demux_thr, NULL);
    demux_thr = NULL;

    logger.debug("Demux closed.\n");
}

void Demux::start ()
{
    if (!demux_thr)
        return;

    pause_req = false;
    SDL_LockMutex(wait_mutex);
    SDL_CondSignal(continue_read_cond);
    SDL_UnlockMutex(wait_mutex);
    while (paused);

    logger.debug("Demux started.\n");
}

void Demux::pause ()
{
    if (!demux_thr)
        return;

    pause_req = true;
    SDL_LockMutex(wait_mutex);
    SDL_CondSignal(continue_read_cond);
    SDL_UnlockMutex(wait_mutex);
    while (!paused);

    logger.debug("Demux paused.\n"); 
}

int Demux::seek (double pos)
{
    int ret;

    if (!demux_thr)
        return KERROR(KEUNINITED);

    /* seek */
    ret = av_seek_frame(avfctx, -1, 
                        pos * AV_TIME_BASE + (AV_NOPTS_VALUE == avfctx->start_time ? 0 : avfctx->start_time), 
                        AVSEEK_FLAG_BACKWARD);
    if (ret < 0) { // ffmpeg unsolved: av_seek_frame() return -1 when the media format is h264 or h265
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KESEEK_FAIL), av_err2str(ret));
        return KERROR(KESEEK_FAIL);     }

    /* reset read eof flag */
    if (vpktq)
        vpktq->set_read_eof(false);
    if (apktq)
        apktq->set_read_eof(false);
    read_eof = false;

    logger.debug("Demux seek to %lf.\n", pos);

    return ret;
}

Demux::Demux (AVFormatContext* avfctx, PacketQueue* vpktq, PacketQueue* apktq, 
              SDL_mutex* wait_mutex, SDL_cond* continue_read_cond, 
              int vst_idx, int ast_idx, 
              bool infinite_buf, int max_pktq_size)
{
    this->avfctx = avfctx;
    this->vpktq = vpktq;
    this->apktq = apktq;
    this->wait_mutex = wait_mutex;
    this->continue_read_cond = continue_read_cond;
    this->infinite_buf = infinite_buf;
    this->max_pktq_size = max_pktq_size;
    this->vst_idx = vst_idx;
    this->ast_idx = ast_idx;
    if (vst_idx >= 0)
        vst = avfctx->streams[vst_idx];
    else 
        vst = NULL;
    if (ast_idx >= 0)
        ast = avfctx->streams[ast_idx];
    else 
        ast = NULL;
    demux_thr = NULL;
}

Demux::~Demux ()
{
    if (demux_thr)
        close();
}
