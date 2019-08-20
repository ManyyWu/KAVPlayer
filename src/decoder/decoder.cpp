#include "decoder.h"
#include "error/error.h"
#include "log/log.h"
#include <cstring>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/time.h"
#include "SDL2/SDL.h"
}

#define FILENAME "decoder.cpp"

int SDLCALL Decoder::vdec_thread (void* args)
{
    Decoder    *d = (Decoder *)args;
    AVFrame    *f = NULL;
    AVRational  frame_rate = av_guess_frame_rate(d->avfctx, d->st, NULL);
    int         got_frame;
    int         ret;

    logger.debug("Video decoder thread started.\n");

    while (!d->abort_req) {
        /* pause */
        if (d->pause_req) {
            got_frame = 0;
            goto pause;
        }

        /* alloc a frame */
        f = av_frame_alloc();
        if (!f)
            GOTO_FAIL(KENOMEM);

        /* get a packet and decode it */
        got_frame = d->decode_packets(f);
        ret = 0;
put_frame:
        if (d->abort_req)
            break;
        if (d->pause_req) {
pause:            
            SDL_LockMutex(d->pause_mutex);
            d->pause_req = false;
            d->paused = true;
            logger.debug("Video decoder thread paused.\n");
            SDL_CondWait(d->pause_cond, d->pause_mutex);
            logger.debug("Video decoder thread resumed.\n");
            d->paused = false;
            SDL_UnlockMutex(d->pause_mutex);
            if (d->seek_req) {
                d->seek_req = false;
                av_frame_free(&f);
                continue;
            }
            if (1 == got_frame)
                goto put_frame;
        }
        if (1 == got_frame) {
            double pts = f->pts == AV_NOPTS_VALUE ? d->clk.get() : f->pts * av_q2d(d->st->time_base);
            double duration = (frame_rate.num && frame_rate.den) ?
                               av_q2d((AVRational){frame_rate.den, frame_rate.num}) :
                               0;
            if (!ret)
                d->clk.set(f->pts == AV_NOPTS_VALUE ? d->clk.get() + duration : pts);

            /* seeking */
            if (d->seeking) {
                // logger.verbose("Video decoder seeking vpts: %lf\n", pts);
                if (pts < d->seek_pos) {
                    av_frame_free(&f);
                    continue;
                } else {
                    d->seeking = false;
                }
            }

            /* put frame to queue, unblocked */
            ret = d->fq->put(f, f->pkt_pos, pts, duration);
            if (ret < 0) {
                if (KERROR(KEAGAIN) == ret) { // frame_queue is full
                    //logger.debug("Video frame queue is full.\n");
                    av_usleep(10000); // wait 10ms
////////////////////////////////////////////
                    goto put_frame;
                } else { // KERROR(KENOMEM)
                    GOTO_FAIL(KENOMEM);
                }
            } else {
                //logger.verbose("+vfq:%d\n", d->fq->get_len());
            }
        } else if (!got_frame) {
            av_frame_free(&f);
            continue;
        } else {
            if (d->abort_req)
                continue;
            if (KERROR(KEABORTED) == got_frame)
                continue;
            else if (KERROR(KEEOF) == got_frame)
                goto pause;
            logger.FATALN("[%s: %d]%s.\n", kerr2str(KEDECODE_PACKETS_FAIL));
            GOTO_FAIL(KEDECODE_PACKETS_FAIL);
        }
    }

    ret = 0;
fail:
    av_frame_free(&f);
    if (ret < 0)
        emit d->err_occured(ret);

    logger.debug("Video decoder thread stopped.\n");
    return ret;
}

int SDLCALL Decoder::adec_thread (void* args)
{
    Decoder    *d = (Decoder *)args;
    AVFrame    *f = NULL;
    int         got_frame;
    int         ret;

    logger.debug("Audio decoder thread started.\n");

    while (!d->abort_req) {
        /* pause */
        if (d->pause_req) {
            got_frame = 0;
            goto pause;
        }

        /* alloc a frame */
        f = av_frame_alloc();
        if (!f)
            GOTO_FAIL(KENOMEM);

        /* get a packet and decode it */
        got_frame = d->decode_packets(f);
        ret = 0;
put_frame:
        if (d->abort_req)
            break;
        if (d->pause_req) {
pause:            
            SDL_LockMutex(d->pause_mutex);
            d->pause_req = false;
            d->paused = true;
            logger.debug("Audio decoder thread paused.\n");
            SDL_CondWait(d->pause_cond, d->pause_mutex);
            logger.debug("Audio decoder thread resumed.\n");
            d->paused = false;
            SDL_UnlockMutex(d->pause_mutex);
            if (d->seek_req) {
                d->seek_req = false;
                av_frame_free(&f);
                continue;
            }
            if (1 == got_frame)
                goto put_frame;
        }
        if (1 == got_frame) {
            double pts = f->pts == AV_NOPTS_VALUE ? d->clk.get() : f->pts * av_q2d(d->st->time_base);
            double duration = av_q2d((AVRational){f->nb_samples, f->sample_rate});
            if (!ret)
                d->clk.set(f->pts == AV_NOPTS_VALUE ? d->clk.get() + duration : pts);

            /* seeking */
            if (d->seeking) {
                // logger.verbose("Audio decoder seeking apts: %lf\n", pts);
                if (pts < d->seek_pos) {
                    av_frame_free(&f);
                    continue;
                } else {
                    d->seeking = false;
                }
            }

            /* put frame to queue, unblocked */
            ret = d->fq->put(f, f->pkt_pos, pts, duration);
            if (ret < 0) {
                if (KERROR(KEAGAIN) == ret) { // frame_queue is full
                    //logger.debug("Audio frame queue is full.\n");
                    av_usleep(10000); // wait 10ms
////////////////////////////////////////////
                    goto put_frame;
                } else { // KERROR(KENOMEM)
                    GOTO_FAIL(KENOMEM);
                }
            } else {
                //logger.verbose("+afq:%d\n", d->fq->get_len());
            }
        } else if (!got_frame) {
            av_frame_free(&f);
            continue;
        } else {
            if (d->abort_req)
                continue;
            if (KERROR(KEABORTED) == got_frame)
                continue;
            else if (KERROR(KEEOF) == got_frame)
                goto pause;
            logger.FATALN("[%s: %d]%s.\n", kerr2str(KEDECODE_PACKETS_FAIL));
            GOTO_FAIL(KEDECODE_PACKETS_FAIL);
        }
    }

    ret = 0;
fail:
    av_frame_free(&f);
    if (ret < 0)
        emit d->err_occured(ret);

    logger.debug("Audio decoder thread stopped.\n");
    return ret;
}

int Decoder::decode_packets (AVFrame* f)
{
    AVPacket *pkt = NULL;
    int       ret;

    while (!abort_req) {
        switch (avctx->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            ret = avcodec_receive_frame(avctx, f);
            break;
        case AVMEDIA_TYPE_AUDIO:
            ret = avcodec_receive_frame(avctx, f);
            break;
        default:
            return KERROR(KEUNSUPPORTED_MEDIA_STREAM_TYPE);
        }
        if (AVERROR(EAGAIN) != ret) {
            if (AVERROR_EOF == ret) {
                return 0;
            } else if (ret >= 0) { // success
                return 1;
            } else { // get an error
                logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KERECEIVE_FRAME_FAIL), av_err2str(ret));
                GOTO_FAIL(KERECEIVE_FRAME_FAIL);
            }
        } else {
            if (pktq->is_eof())
                return KERROR(KEEOF);
        }

        /* get a packet from queue, blocked */
get_pkt:
        if (pktq->get_len() <= 2) {
            SDL_LockMutex(wait_mutex);
            SDL_CondSignal(empty_queue_cond);
            SDL_UnlockMutex(wait_mutex);
        }
        pkt = pktq->get();
        //logger.verbose("-vpktq:%d\n", pktq->get_len());
        if (!pkt) // aborted or eof
            return pktq->is_eof() ?  KERROR(EOF): KERROR(KEABORTED);

        /* if get a common packet, send it to decoder */
        ret = avcodec_send_packet(avctx, pkt);
        if (ret < 0) {
             if (AVERROR(EAGAIN) == ret) {
                 logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KESEND_PACKET_FAIL), av_err2str(ret));
                 /*
                 * input is not accepted in the current state - user
                 * must read output with avcodec_receive_frame() (once
                 * all output is read, the packet should be resent, and
                 * the call will not fail with EAGAIN).
                 * */
             } else if (AVERROR(ENOMEM) == ret) {
                 return AVERROR(KENOMEM);
             } else {
                  logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KESEND_PACKET_FAIL), av_err2str(ret));
                  GOTO_FAIL(KESEND_PACKET_FAIL);
             }
        }
        av_packet_free(&pkt);
    }

    ret = 0;
fail:
    if (pkt)
        av_packet_free(&pkt);
    return ret;
}

int Decoder::init (Clock clk)
{
    int ret;

    if (dec_thr)
        return KERROR(KEREINIT);

    abort_req = false;
    pause_req = false;
    paused = false;
    seeking = false;
    seek_req = false;

    /* create mutex and cond */
    pause_mutex = SDL_CreateMutex();
    if (!pause_mutex) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_MUTEX_FAIL), SDL_GetError());
        return KERROR(KECREATE_SDL_MUTEX_FAIL);
    }
    pause_cond = SDL_CreateCond();
    if (!pause_cond) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_COND_FAIL), SDL_GetError());
        return KERROR(KECREATE_SDL_COND_FAIL);
    }

    /* find decoder */
    avctx = avcodec_alloc_context3(NULL);
    if (!avctx)
        return KERROR(KENOMEM);
    ret = avcodec_parameters_to_context(avctx, st->codecpar);
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECOPY_CODEC_PARAMS_FAIL), av_err2str(ret));
        return KERROR(KECOPY_CODEC_PARAMS_FAIL);
    }
    codec = avcodec_find_decoder(avctx->codec_id);
    if (!codec) {
        logger.FATALN("[%s: %d]%s.\n", kerr2str(KEAVCODEC_FIND_DECODER_FAIL));
        return KERROR(KEAVCODEC_FIND_DECODER_FAIL);
    }

    /* open decoder */
    ret = avcodec_open2(avctx, codec, NULL);
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KEOPEN_DECODER_FAIL), av_err2str(ret));
        return KERROR(KEOPEN_DECODER_FAIL);
    }

    /* set clock */
    this->clk.set(clk.get());

    /* create video decoder thread */
    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
        dec_thr = SDL_CreateThread(vdec_thread, "vdec_thread", this);
        break;
    case AVMEDIA_TYPE_AUDIO:
        dec_thr = SDL_CreateThread(adec_thread, "adec_thread", this);
        break;
    default:
        logger.FATALN("[%s: %d]%s.\n", kerr2str(KEUNSUPPORTED_MEDIA_STREAM_TYPE));
        return KERROR(KEUNSUPPORTED_MEDIA_STREAM_TYPE);
    }
    if (!dec_thr) {
        logger.FATALN("[%s: %d]%s.\n", kerr2str(KECREATE_THREAD_FAIL));
        return KERROR(KECREATE_THREAD_FAIL);
    }

    if (AVMEDIA_TYPE_VIDEO == avctx->codec_type)
        logger.debug("Video decoder has been inited.\n");
    else if (AVMEDIA_TYPE_VIDEO == avctx->codec_type)
        logger.debug("Audio decoder has been inited.\n");
    return 0;
}

void Decoder::close ()
{
    int         ret;
    AVMediaType type = avctx->codec_type;

    if (!dec_thr)
        return;

    /* stop decoder thread */
    abort_req = true;
    SDL_LockMutex(pause_mutex);
    SDL_CondSignal(pause_cond);
    SDL_UnlockMutex(pause_mutex);
    SDL_WaitThread(dec_thr, NULL);

    /* clear all */
    SDL_DestroyMutex(pause_mutex);
    SDL_DestroyCond(pause_cond);
    avcodec_close(avctx);
    avcodec_free_context(&avctx);

    dec_thr = NULL;
    paused = true;
    seeking = false;

    if (AVMEDIA_TYPE_VIDEO == type)
        logger.debug("Video decoder closed.\n");
    else if (AVMEDIA_TYPE_VIDEO == type)
        logger.debug("Audio decoder closed.\n");
}

void Decoder::start ()
{
    if (!dec_thr)
        return;

    pause_req = false;
    SDL_LockMutex(pause_mutex);
    SDL_CondSignal(pause_cond);
    SDL_UnlockMutex(pause_mutex);
    while (paused);

    if (AVMEDIA_TYPE_VIDEO == avctx->codec_type)
        logger.debug("Video decoder started.\n");
    else if (AVMEDIA_TYPE_VIDEO == avctx->codec_type)
        logger.debug("Audio decoder started.\n");
}

void Decoder::pause ()
{
    if (!dec_thr)
        return;

    pause_req = true;
    pktq->abort();
    while (!paused);

    if (AVMEDIA_TYPE_VIDEO == avctx->codec_type)
        logger.debug("Video decoder paused.\n");
    else if (AVMEDIA_TYPE_VIDEO == avctx->codec_type)
        logger.debug("Audio decoder paused.\n");
}

void Decoder::seek (double pos)
{
    if (!dec_thr)
        return;

    if (!seeking) {
        if (AVMEDIA_TYPE_VIDEO == avctx->codec_type)
            logger.debug("Video decoder seek to %lf.\n", pos);
        else if (AVMEDIA_TYPE_VIDEO == avctx->codec_type)
            logger.debug("Audio decoder seek to %lf.\n", pos);

        clk.set(pos);
        seek_req = true;
        seek_pos = pos;
        seeking = true;
    }
}

void Decoder::flush ()
{
    if (!dec_thr)
        return;

    if (paused) {
        pktq->clear();
        fq->clear();
        avcodec_flush_buffers(avctx);

        if (AVMEDIA_TYPE_VIDEO == avctx->codec_type)
            logger.debug("Video decoder flushed.\n");
        else if (AVMEDIA_TYPE_AUDIO == avctx->codec_type)
            logger.debug("Audio decoder flushed.\n");
    }
}

bool Decoder::is_seeking () const
{
    return seeking;
}

Decoder::Decoder (AVFormatContext* avfctx, int st_idx, 
                  PacketQueue* pktq, FrameQueue* fq, 
                  SDL_mutex* wait_mutex, SDL_cond* empty_queue_cond)
{
    this->avfctx = avfctx;
    this->st_idx = st_idx;
    this->st = avfctx->streams[st_idx];
    this->pktq = pktq;
    this->fq = fq;
    this->wait_mutex = wait_mutex;
    this->empty_queue_cond = empty_queue_cond;
    dec_thr = NULL;
}

Decoder::~Decoder ()
{
    if (dec_thr)
        close();
}
