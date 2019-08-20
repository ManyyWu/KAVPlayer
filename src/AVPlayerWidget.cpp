#include <cstring>
#include <cstdlib>
#include <new>
#include <cstdint>
#include <QWidget>
#include <QMessageBox>
#include <QKeyEvent>
#include <QApplication>
#include <QDesktopWidget>
#include "AVPlayerWidget.h"
#include "log/log.h"
#include "error/error.h"
#include "utils/utils.h"
#include "decoder/decoder.h"
#include "demux/demux.h"
#include "render/render.h"
#include "clock/clock.h"
#include "vdev/vdev.h"
#include "adev/adev.h"
#if defined(_DEBUG) && defined(_WIN32)
#define CRTDBG_MAP_ALLOC 
#include <crtdbg.h>
#endif /* _DEBUG */

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/time.h"
#include "SDL2/SDL.h"
}

#define FILENAME "AVPlayerWidget.cpp"

void AVPlayerWidget::stop (int err_code)
{
    if (this->stop_req)
        return;

    /* stop */
    stop();

    /* notice GUI */
//    emit player_stopped(err_code);
}

void AVPlayerWidget::mouseDoubleClickEvent (QMouseEvent * e)
{
    /* switch fullscreen */
    if (!inited)
        return;

    QKeyEvent key_event(QEvent::KeyPress, Qt::Key_F, Qt::NoModifier);
    QCoreApplication::sendEvent(parentWidget(), &key_event);
}

void AVPlayerWidget::keyPressEvent (QKeyEvent * e)
{
    int pos;
    int ret;

    if (!inited)
        return;

    QCoreApplication::sendEvent(parentWidget(), e);
}

void AVPlayerWidget::force_refresh ()
{
    if (!inited || !vpaused)
        return;

    force_refresh_req = true;
    SDL_LockMutex(pause_mutex);
    SDL_CondSignal(pause_cond);
    SDL_UnlockMutex(pause_mutex);
    while (!vpaused);

    logger.debug("Force refresh.\n");
}

void AVPlayerWidget::reset_members ()
{
    wanted_vst = wanted_ast = -1;
    frame_drop = false;
    infinite_buf = false;
    max_pktq_size = DEF_PKTQ_SIZE;
    max_pictq_len = DEF_PICTQ_LEN;
    max_sampleq_len = DEF_SAMPLEQ_LEN;
    speed = 1.0;
    paused = true;
    stopped = true;
    delay = 0.0;
    old_volume = SDL_MIX_MAXVOLUME;
    err_code = 0;
    url = NULL;
    avfctx = NULL;
    vst = NULL;
    ast = NULL;
    adev = NULL;
    render = NULL;
    adec = NULL;
    vdec = NULL;
    demux = NULL;
    wait_mutex = NULL;
    continue_read_cond = NULL;
    priv_vf = NULL;
    cur_texture = NULL;
}

double AVPlayerWidget::compute_delay (Frame* priv_vf, Frame* cur_vf)
{
    double tgt_delay;
    double last_duration;

    if (priv_vf) {
        double pts_diff = cur_vf->pts - priv_vf->pts;
        last_duration =  ((isnan(pts_diff)
                   || pts_diff <= 0
                   || pts_diff > (double)max_frame_duration)
                   ? priv_vf->duration
                   : pts_diff);
    } else {
        last_duration = 0.0;
    }
    tgt_delay = last_duration;

    /* set primary clock when no audio stream */
    if (!ast) 
        priclk.set((av_gettime() - spare_clock) / (double)AV_TIME_BASE);

    double primary_clk = priclk.get();
    double video_clk = vclk.get();
    double sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, tgt_delay));
    double clock_diff = video_clk - primary_clk;
    if (!isnan(clock_diff) && fabs(clock_diff) < (double)max_frame_duration) {
        if (clock_diff < -sync_threshold)
            tgt_delay = (frame_drop && clock_diff < -AV_SYNC_FRAMEDROP_THRESHOLD) ? clock_diff : FFMAX(0, tgt_delay + clock_diff);
        else
            tgt_delay = tgt_delay > AV_SYNC_FRAMEDUP_THRESHOLD ? tgt_delay + clock_diff : 2 * tgt_delay /* not clock_diff */;
    }

    logger.verbose("%7.2lfs, fps:%d, A-V: %lf, delay: %lf, buffer: %.2lfKB\n",
                   primary_clk, get_fps(), -clock_diff, tgt_delay,
                   (apktq ? (double)apktq->get_size() / 1024.0 : 0.0) + (vpktq ? (double)vpktq->get_size() / 1024.0 : 0.0));

    return min(tgt_delay, AV_SYNC_DELAY_MAX);
}

void AVPlayerWidget::calculate_display_rect (AVFrame* vf, SDL_Rect* rect)
{
    float      aspect_ratio;
    int        width, height, x, y;
    AVRational pic_sar = vf->sample_aspect_ratio;

    if (pic_sar.num == 0)
        aspect_ratio = 0;
    else
        aspect_ratio = av_q2d(pic_sar);

    if (aspect_ratio <= 0.0)
        aspect_ratio = 1.0;
    aspect_ratio *= (float)vf->width / (float)vf->height;

    /* XXX: we suppose the screen has a 1.0 pixel ratio */
    QRect first_scr_rect = QApplication::desktop()->screenGeometry(0);
    int w = vdev->width();
    int h = vdev->height();
    height = h;
    width = lrint(height * aspect_ratio) & ~1;
    if (width > w) {
        width = w;
        height = lrint(width / aspect_ratio) & ~1;
    }
    x = (w - width) / 2;
    y = (h - height) / 2;
    rect->x = x;
    rect->y = y;
    rect->w = FFMAX(width,  1);
    rect->h = FFMAX(height, 1);
}

int AVPlayerWidget::video_refresh ()
{
    Frame *  vf = NULL;
    SDL_Rect rect;
    int      ret;

    /* update message or video */
    if (!vst || force_refresh_req) { 
        force_refresh_req = false;
        vdev->lock();
        if (priv_vf) {
            calculate_display_rect(priv_vf->frame, &rect);
            ret = vdev->upload_texture(cur_texture, rect);
        } else { // no video frame, black background
            SDL_Rect cur_rect = SDL_Rect{0, 0, vdev->width(), vdev->height()};
            ret = vdev->upload_texture(cur_texture, cur_rect);
        }
        if (ret < 0) {
            logger.FATALN("[%s: %d]%s.\n", kerr2str(KEUPLOAD_TEXTURE_FAIL));
            GOTO_FAIL(KEUPLOAD_TEXTURE_FAIL);
        }
        ret = vdev->upload_texture(msg_texture, msg_rect);
        if (ret < 0) {
            logger.FATALN("[%s: %d]%s.\n", kerr2str(KEUPLOAD_TEXTURE_FAIL));
            GOTO_FAIL(KEUPLOAD_TEXTURE_FAIL);
        }
        vdev->unlock();
    } else {
        step_req = false;
    
        /* destroy current texture */
        SDL_DestroyTexture(cur_texture);
        cur_texture = NULL;

        /* update GUI play progress */
        if (!adev && !close_req)
            emit pos_changed(get_pos());

        /* get next video frame */
        if (vfq->get_len() <= 2) {
            SDL_LockMutex(wait_mutex);
            SDL_CondSignal(continue_read_cond);
            SDL_UnlockMutex(wait_mutex);
        }
        vf = vfq->get();
        if (!vf) { // aborted or play over
            if (vfq->is_eof())
                ret = KERROR(KEPLAY_OVER);
            else
                ret = KERROR(KEABORTED);
            goto fail;
        }

        /* compute delay */
        delay = compute_delay(priv_vf, vf);
        if (delay < 0.0) { // drop a frame
            delay = 0.0;

            /* set video clock */
            vclk.set(vf->pts);

            /* save current frame */
            av_frame_free(&priv_vf->frame);
            delete priv_vf;
            priv_vf = vf;
            vf = NULL;

            logger.verbose("frame drop.\n");
        } else {
            /* calculate display rect */
            calculate_display_rect(vf->frame, &rect);

            /* render a frame */
            ret = render->render_video_frame(vf, &cur_texture);
            if (ret < 0) {
                logger.FATALN("[%s: %d]%s.\n", kerr2str(KERENDER_FRAME_FAIL));
                GOTO_FAIL(KERENDER_FRAME_FAIL);
            }

            /* display a frame */
            vdev->lock();
            ret = vdev->upload_texture(cur_texture, rect);
            if (ret < 0) {
                logger.FATALN("[%s: %d]%s.\n", kerr2str(KEUPLOAD_TEXTURE_FAIL));
                GOTO_FAIL(KEUPLOAD_TEXTURE_FAIL);
            }
            ret = vdev->upload_texture(msg_texture, msg_rect);
            if (ret < 0) {
                logger.FATALN("[%s: %d]%s.\n", kerr2str(KEUPLOAD_TEXTURE_FAIL));
                GOTO_FAIL(KEUPLOAD_TEXTURE_FAIL);
            }
            vdev->unlock();

            /* update video clock */
            vclk.set(vf->pts);

            /* save current frame */
            av_frame_free(&priv_vf->frame);
            delete priv_vf;
            priv_vf = vf;
            vf = NULL;
        }
    }

    ret = 0;
fail:
    if (ret < 0) {
        av_frame_free(&vf->frame);
        delete vf;
    }
    return ret;
}

bool AVPlayerWidget::is_realtime ()
{
    const char *name = avfctx->iformat->name;
    const char *url = avfctx->url;

    if (!strcmp(name, "rtp")
        || !strcmp(name, "rtsp")
        || !strcmp(name, "sdp"))
        return true;

    if (avfctx->pb
        && (!strncmp(url, "rtp:", 4)
           || !strncmp(url, "udp:", 4)))
        return true;

    return false;
}

int AVPlayerWidget::open_media_file (const char* url)
{
    int ret;

    /* copy url */
    this->url = av_strdup(url);
    if (!this->url)
        return KERROR(KENOMEM);

    /* open input file */
    avfctx = avformat_alloc_context();
    if (!avfctx)
        return KERROR(KENOMEM);
    ret = avformat_open_input(&avfctx, url, NULL, NULL);
    if (ret < 0) {
        logger.error("%s %s %s: \n", kerr2str(KEOPEN_INPUT_FAIL), url, av_err2str(ret));
        return KERROR(KEOPEN_INPUT_FAIL);
    }

    /* find stream info */
    ret = avformat_find_stream_info(avfctx, NULL);
    if (ret < 0) {
        logger.error("%s: %s.\n", kerr2str(KEFIND_STREAM_INFO_FAIL), av_err2str(ret));
        return KERROR(KEFIND_STREAM_INFO_FAIL);
    }

    /* set duration */
    duration = avfctx->duration / (double)AV_TIME_BASE;
    duration = duration <= 0 ? 0 : duration;

    /* check whether the media stream is realtime */
    realtime = is_realtime();

    /* find streams */
    vst_idx = ast_idx = -1;
    AVStream *st = NULL;
    if (wanted_vst >= 0 && wanted_vst < avfctx->nb_streams) {
        st = avfctx->streams[wanted_vst];
        if (AVMEDIA_TYPE_VIDEO == st->codecpar->codec_type) 
            vst_idx = wanted_vst;
    }
    if (wanted_ast >= 0 && wanted_ast < avfctx->nb_streams) {
        st = avfctx->streams[wanted_ast];
        if (AVMEDIA_TYPE_AUDIO == st->codecpar->codec_type)
            ast_idx = wanted_ast;
    }
    vst_idx = av_find_best_stream(avfctx, AVMEDIA_TYPE_VIDEO, vst_idx, -1, NULL, 0);
    ast_idx = av_find_best_stream(avfctx, AVMEDIA_TYPE_AUDIO, ast_idx, -1, NULL, 0);
    if (vst_idx >= 0)
        vst = avfctx->streams[vst_idx];
    if (ast_idx >= 0)
        ast = avfctx->streams[ast_idx];
    if (!ast && !vst) {
        logger.error("%s.\n", kerr2str(KENOAVST));
        return KERROR(KENOAVST);
    }

    /* dump format */
    logger.dis_label();
    av_dump_format(avfctx, 0, url, 0);
    logger.en_label();

    return 0;
}

int AVPlayerWidget::init_queues (int max_pictq_len, int max_sampleq_len)
{
    int ret = 0;

    vpktq = apktq = NULL;
    vfq = afq = NULL;
    if (vst) {
        vpktq = _New PacketQueue();
        vfq = _New FrameQueue();
        if (!vpktq || !vfq)
            return KERROR(KENOMEM);
        if (vpktq->init() < 0 || vfq->init(vpktq, max_pictq_len) < 0) {
            return KERROR(KEQUEUE_INIT_FAIL);
        }
    }
    if (ast){
        apktq = _New PacketQueue();
        afq = _New FrameQueue();
        if (!apktq || !afq) {
            return KERROR(KENOMEM);
        }
        if (apktq->init() < 0 || afq->init(apktq, max_pictq_len) < 0) {
            return KERROR(KEQUEUE_INIT_FAIL);
        }
    }

    return 0;
}

void AVPlayerWidget::vplay ()
{
    vstop_req = false;
    vstopped = false;
    vpause_req = false;
    if (vfq)
        vfq->abort();
    SDL_LockMutex(pause_mutex);
    SDL_CondSignal(pause_cond);
    SDL_UnlockMutex(pause_mutex);
    if (vst)
        while (vpaused);

    /*
    * to fix negative pts, set primary clock if no audio stream 
    */
    if (!ast) {
        spare_clock = av_gettime() - (int64_t)(vclk.get() * (double)AV_TIME_BASE);
        priclk.set(vclk.get());
    }
}

void AVPlayerWidget::vpause ()
{
    if (vst) {
        vpause_req = true;
        if (vfq)
            vfq->abort();
        SDL_LockMutex(pause_mutex);
        SDL_CondSignal(pause_cond);
        SDL_UnlockMutex(pause_mutex);
        while (!vpaused);
    }
}

void AVPlayerWidget::aplay ()
{
    afq->abort();
    adev->play();
}

void AVPlayerWidget::apause ()
{
    afq->abort();
    adev->pause();
}

int AVPlayerWidget::audio_fill_proc (void* data, SampleBuf* sample_buf)
{
    AVPlayerWidget *p = (AVPlayerWidget *)data;
    AudioParams     ap_tgt = p->render->get_ap_tgt();
    int             ret = 0;

    if (!p->stop_req && !p->close_req && !sample_buf->pos && !sample_buf->size) {
        /* get an audio frame, blocked */
        if (p->afq->get_len() <= 2) {
            SDL_LockMutex(p->wait_mutex);
            SDL_CondSignal(p->continue_read_cond);
            SDL_UnlockMutex(p->wait_mutex);
        }
        Frame *af = p->afq->get();
        if (!af) { // aborted or eof
////////////////////////////////////////////////////////
            if (p->afq->is_eof())
                ret = KERROR(KEPLAY_OVER);
            else
                ret = KERROR(KEABORTED);
            goto err;
        }

        /* resample */
        ret = p->render->resample(af->frame, sample_buf);
        if (ret < 0) {
            logger.FATALN("[%s: %d]%s.\n", kerr2str(KERESAMPLE_FAIL));
            ret = KERROR(KERESAMPLE_FAIL);
            av_frame_free(&af->frame);
            delete af;            
err:
            emit p->err_occured(ret);
            return ret;
        }

        double cur_af_pts = af->pts;
        av_frame_free(&af->frame);
        delete af;

        /* update audio clock */
        int byte_per_sec = ap_tgt.channels
                           * ap_tgt.sample_rate
                           * av_get_bytes_per_sample(ap_tgt.sample_fmt);
        p->priclk.set(p->adev->get_cur_af_pts() + (double)((sample_buf->pos - sample_buf->buf) / (double)byte_per_sec));
        p->adev->set_cur_af_pts(cur_af_pts);
        emit p->pos_changed(p->priclk.get());
    }

    return 0;
}

int AVPlayerWidget::get_media_info (const char* url, MediaInfo* info)
{
    if (!inited)
        return KERROR(KEUNINITED);
    if (!url)
        return -1;

    return 0;
}

int SDLCALL AVPlayerWidget::vrefresh_thread (void* args)
{
    AVPlayerWidget *p = (AVPlayerWidget *)args;

    logger.debug("Video refresh thread started.\n");

    p->vstopped = false;   // controlled by vplay() and stop()
    p->vstop_req = false;  // controlled by vplay() and stop()
    p->vpaused = true;     // controlled by vrefresh_thread()
    p->vpause_req = false; // controlled by vplay() and vpause()
    p->step_req = false;   // controlled by step() and vrefresh_thread()

    p->delay = 0.0;
    while (!p->close_req) {
        if (p->vst && !p->vstop_req && !p->vstopped) { // playing or paused
            p->vpaused = false;

            /* pause */
            if (p->vpause_req && !p->step_req) {
                SDL_LockMutex(p->pause_mutex);
                p->vpaused = true;
                logger.debug("Video refresh thread paused.\n");
                SDL_CondWait(p->pause_cond, p->pause_mutex);
                p->vpaused = false;
                logger.debug("Video refresh thread resumed.\n");
                SDL_UnlockMutex(p->pause_mutex);
            }

            /* delay */
            if (!p->close_req && !p->vpause_req 
                && !p->vstop_req && !p->step_req 
                && !p->force_refresh_req && p->delay > 0.0) 
                av_usleep((int64_t)(p->delay * AV_TIME_BASE));

            /* video refresh */
            if ((!p->close_req && !p->vpause_req && !p->vstop_req) 
                || p->force_refresh_req || p->step_req) 
                {
                int ret = emit p->video_refresh();
                if (ret < 0) {
                    emit p->err_occured(ret);
                    p->vstop_req = true;
                }
            }
        } else { // stopped
            p->vstop_req = false;
            p->step_req = false;

            /* pause */
            SDL_LockMutex(p->pause_mutex);
            p->vst = NULL;
            p->vstopped = true;
            p->vpaused = true;
            SDL_CondWait(p->pause_cond, p->pause_mutex);
            SDL_UnlockMutex(p->pause_mutex);

            /* force refresh */
            if (p->force_refresh_req)
                emit p->video_refresh();
        }
    }
    
    logger.debug("Video refresh thread closed.\n");
    return KERROR(KEABORTED);
}

int AVPlayerWidget::init (bool en_hw_acce)
{
    int flags = SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS;
    int ret;

    if (inited)
        return KERROR(KEREINIT);

    hw_acce = en_hw_acce;
    step_req = false;
    force_refresh_req = false;
    stop_req = false;
    close_req = false;
    msg_texture = NULL;

    /* init FFmpeg components */
    av_register_all();
    avformat_network_init();

    /* init SDL */
    ret = SDL_Init(flags);
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KESDL_INIT_FAIL), SDL_GetError());
        GOTO_FAIL(KESDL_INIT_FAIL);
    }

    /* init vdev */
    vdev = _New Vdev;
    if (!vdev)
        GOTO_FAIL(KENOMEM);
    vdev->init(this, hw_acce);
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s.\n", kerr2str(KEDEV_INIT_FAIL));
        GOTO_FAIL(KEDEV_INIT_FAIL);
    }
    QObject::connect(this, SIGNAL(err_occured(int)), this, SLOT(stop(int)));

    /* init msger */
    msger = _New Msger();
    if (!msger)
        GOTO_FAIL(KENOMEM);
    ret = msger->init("fonts/stkaiti.ttf", vdev->get_sdl_renderer());
    if (ret < 0) {
        logger.fatal("[%s: %d]%s.\n", kerr2str(KEMSGER_INIT_FAIL));
        GOTO_FAIL(KEMSGER_INIT_FAIL);
    }

    /* create mutex and cond */
    pause_mutex = SDL_CreateMutex();
    if (!pause_mutex) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_MUTEX_FAIL), SDL_GetError());
        GOTO_FAIL(KECREATE_SDL_MUTEX_FAIL);
    }
    pause_cond = SDL_CreateCond();
    if (!pause_cond) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_COND_FAIL), SDL_GetError());
        GOTO_FAIL(KECREATE_SDL_COND_FAIL);
    }

    /* create video refresh thread */
    vrefresh_thr = SDL_CreateThread(vrefresh_thread, "vrefresh_thread", this);
    if (!vrefresh_thr) {
        logger.fatal("[%s: %d]%s: %s.\n", kerr2str(KECREATE_THREAD_FAIL), SDL_GetError());
        GOTO_FAIL(KECREATE_THREAD_FAIL);
    }

    /* init timer */
    QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(clear_msg()));

    logger.debug("Player widget has been inited.\n");

    inited = true;
    ret = 0;
fail:
    if (ret < 0)
        close();

    return ret;
}

int AVPlayerWidget::open (const char* url)
{
    int ret;

    if (!inited)
        return KERROR(KEUNINITED);

    if (this->url)
        stop();

    /* open media file */
    ret = open_media_file(url);
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s.\n", kerr2str(KEOPEN_MEDIA_FILE_FAIL));
        GOTO_FAIL(KEOPEN_MEDIA_FILE_FAIL);
    }

    /* 
    * set max duration allowed between two nearby frame, 
    * Protect against some wrong pts video. 
    * copy from ffplay. 
    */
    max_frame_duration = (avfctx->iformat->flags & AVFMT_TS_DISCONT) ? 
                         10.0 :
                         3600.0;

    /* set start time */
    start_time = (double)(AV_NOPTS_VALUE == avfctx->start_time ? 
                          0.0 : 
                          (double)avfctx->start_time / AV_TIME_BASE);

    /* init clock */
    priclk.set(start_time);
    vclk.set(start_time);

    /* create mutex and cond */
    wait_mutex = SDL_CreateMutex();
    if (!wait_mutex) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_MUTEX_FAIL), SDL_GetError());
        GOTO_FAIL(KECREATE_SDL_MUTEX_FAIL);
    }
    continue_read_cond = SDL_CreateCond();
    if (!wait_mutex) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_COND_FAIL), SDL_GetError());
        GOTO_FAIL(KECREATE_SDL_COND_FAIL);
    }

    /* init queues */
    ret = init_queues(max_pictq_len, max_sampleq_len);
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s.\n", kerr2str(KEQUEUE_INIT_FAIL));
        GOTO_FAIL(KEQUEUE_INIT_FAIL);
    }
    
    /* init demux */
    demux = _New Demux(avfctx, 
                       vpktq, apktq, 
                       wait_mutex, continue_read_cond,
                       vst_idx, ast_idx, 
                       infinite_buf, max_pktq_size);
    if (!demux)
        GOTO_FAIL(KENOMEM);
    QObject::connect(demux, SIGNAL(err_occured(int)), this, SLOT(stop(int)));
    ret = demux->init();   
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s.\n", kerr2str(KEDEMUX_INIT_FAIL));
        GOTO_FAIL(KEDEMUX_INIT_FAIL);
    }

    /* init decoder */
    ret = 0;
    if (vst) {
        vdec = _New Decoder(avfctx, vst_idx, vpktq, vfq, wait_mutex, continue_read_cond);
        if (!vdec)
            GOTO_FAIL(KENOMEM);
        QObject::connect(vdec, SIGNAL(err_occured(int)), this, SLOT(stop(int)));
        ret = vdec->init(priclk);
    }
    if (!ret && ast) {
        adec = _New Decoder(avfctx, ast_idx, apktq, afq, wait_mutex, continue_read_cond);
        if (!adec)
            GOTO_FAIL(KENOMEM);
        QObject::connect(adec, SIGNAL(err_occured(int)), this, SLOT(stop(int)));
        ret = adec->init(priclk);
    }
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s.\n", kerr2str(KEDECODER_INIT_FAIL));
        GOTO_FAIL(KEDECODER_INIT_FAIL);
    }

    /* init adev */
    AudioParams ap_src, ap_tgt;
    if (ast) {
        adev = _New Adev(audio_fill_proc, this);
        if (!adev)
            GOTO_FAIL(KENOMEM);
        QObject::connect(adev, SIGNAL(err_occured(int)), this, SLOT(stop(int)));

        /* wait for the first audio frame to be decode */
        Frame *af = afq->peek();
        if (!af)
            GOTO_FAIL(KENO_FIRST_FRAME);

        ap_src.channels = af->frame->channels;
        ap_src.channel_layout = af->frame->channel_layout <= 0 ?
                                av_get_default_channel_layout(ap_src.channels) :
                                af->frame->channel_layout; // to fix *.wma
        ap_src.nb_samples = af->frame->nb_samples;
        ap_src.sample_fmt = (AVSampleFormat)af->frame->format;
        ap_src.sample_rate = af->frame->sample_rate;
        ret = adev->init(ap_src, &ap_tgt);
        if (ret < 0) {
            logger.FATALN("[%s: %d]%s.\n", kerr2str(KEDEV_INIT_FAIL));
            GOTO_FAIL(KEDEV_INIT_FAIL);
        }
    }

    /* init render */
    render = _New Render(vdev ? vdev->get_sdl_renderer() : NULL, vfq, afq, wait_mutex, continue_read_cond);
    if (!render)
        GOTO_FAIL(KENOMEM);
    if (vdev)
        render->init_vrender();
    if (adev) {
        ret = render->init_arender(ap_src, ap_tgt);
        if (ret < 0) {
            logger.FATALN("[%s: %d]%s.\n", kerr2str(KERENDER_INIT_FAIL));
            GOTO_FAIL(KERENDER_INIT_FAIL); 
        }
    }
    render->set_speed(speed);

    stopped = false;
    stop_req = false;

    logger.info("File %s is open.\n", url);
    ret = 0;
fail:
    if (ret < 0)
        stop();
    return ret;
}

void AVPlayerWidget::stop ()
{
    if (!url || stopped)
        return;
    stop_req = true;

    /* stop video refresh thread */
    if (!vstopped) {
        vstop_req = true;
        if (vfq)
            vfq->abort();
        SDL_LockMutex(pause_mutex);
        SDL_CondSignal(pause_cond);
        SDL_UnlockMutex(pause_mutex);
        while (!vstopped);
    }
        
    /* destroy texture */
    SDL_DestroyTexture(cur_texture);
    cur_texture = NULL;

    /* clear frames */
    av_frame_free(&priv_vf->frame);
    delete priv_vf;

    /* close audio device */
    if (adev) {
        adev->close();
        delete adev;
    }

    /* close render */
    if (vst && render)
        render->close_vrender();
    if (ast && render)
        render->close_arender();
    delete render;

    /* close decoders */
    if (vdec) {
        vdec->close();
        delete vdec;
    }
    if (adec) {
        adec->close();
        delete adec;
    }

    /* close demux */
    if (demux)
        demux->close();
    delete demux;

    /* clear queues */
    delete vfq;
    delete afq;
    delete vpktq;
    delete apktq;

    /* close format context */
    avformat_close_input(&avfctx);

    /* free other memebers */
    av_freep(&url);

    /* reset members */
    reset_members();

    /* force refresh */
    force_refresh();

    /* update GUI */
    emit pos_changed(0.0); // to fix bug

    stop_req = false;
    logger.debug("Player widget stopped.\n");
}

void AVPlayerWidget::play ()
{
    if (!url || stopped)
        return;

    /* play audio */
    if (adev)
        aplay();

    /* play video */
    if (vdev)
        vplay();

    /* set delay */
    delay = 0.0;

    paused = false;

    logger.debug("Player widget Playing.\n");
}

void AVPlayerWidget::pause ()
{
    if (!url || stopped)
        return;

    /* pause audio */
    if (adev)
        apause();

    /* pause video refresh */
    if (vdev)
        vpause();

    paused = true;

    logger.debug("Player widget paused.\n");
}

void AVPlayerWidget::close ()
{
    int ret;

    if (!inited)
        return;
    inited = false;

    close_req = true;

    /* stop */
    if (!stopped)
        stop();

    /* stop timer */
    if (timer.isActive())
        timer.stop();

    /* destroy video refresh thread */
    if (vrefresh_thr) {
        SDL_LockMutex(pause_mutex);
        SDL_CondSignal(pause_cond);
        SDL_UnlockMutex(pause_mutex);
        SDL_WaitThread(vrefresh_thr, NULL);
    }

    /* destroy mutex and cond */
    if (pause_mutex)
        SDL_DestroyMutex(pause_mutex);
    if (pause_cond)
        SDL_DestroyCond(pause_cond);

    /* clear msger */
    if (msger)
        msger->close();
    delete msger;

    /* clear vdev */
    if (vdev)
        vdev->close();
    delete vdev;

    /* deinit SDL */
    SDL_Quit();

    /* deinit FFmpeg components */
    avformat_network_deinit();

    /* reset members */
    reset_members();

    pause_mutex = NULL;
    pause_cond = NULL;
    vdev = NULL;
    vrefresh_thr = NULL;
    msger = NULL;

    logger.debug("Player widget closed.\n");
}

int AVPlayerWidget::seek (double pos)
{
    bool _paused = paused;

    if (!url)
        return KERROR(KEUNINITED);

    if (pos > duration)
        return KERROR(KEINVAL);
    pos += start_time;
    if (pos - start_time < 0.001)
        pos = 0.0;

    logger.info("Seeking to %lfs.\n", pos);

    /* pause video refresh */
    if (vst) // not "if (vdev)"
        vpause();

    /* pause audio */
    if (adev)
        adev->pause();

    /* pause decoder */
    if (adec)
        adec->pause();
    if (vdec)
        vdec->pause();

    /* pause demux */
    demux->pause();

    /* clear queues and avcodec buffer */
    if (vdec)
        vdec->flush();
    if (adec)
        adec->flush();

    /* clear audio buffer */
    if (adev)
        adev->flush();

    /* set clocks */
    priclk.set(pos);
    vclk.set(pos);
    delay = 0.0;

    /* seek */
    int ret = demux->seek(pos);
    if (ret < 0)
        return ret;
    if (vdec)
        vdec->seek(pos);
    if (adec)
        adec->seek(pos);

    /* reset abort flag */
    if (vpktq)
        vpktq->restore();
    if (apktq)
        apktq->restore();

    /* start demux */
    demux->start();

    /* start decoder */
    if (adec)
        adec->start();
    if (vdec)
        vdec->start();

    /* wait for seeking */
    while (adec && adec->is_seeking())
    while (vdec && vdec->is_seeking())

    /* free privious frame */
    av_frame_free(&priv_vf->frame);
    delete priv_vf;
    priv_vf = NULL;

    /* start audio and video refresh */
    if (!_paused) {
        if (vst)
            vplay();

        if (adev)
            adev->play();
    } else {
        force_refresh();
///////////////////////////
    }

    return ret;
}

double AVPlayerWidget::get_pos ()
{
    return (url ? priclk.get() : 0.0);
}

int AVPlayerWidget::get_fps ()
{
    if (!inited)
        return KERROR(KEUNINITED);

    return (vdev && (!paused && !stopped)? vdev->get_fps() : 0);
}

double AVPlayerWidget::get_duration ()
{
    return (url ? duration : 0.0);
}

void AVPlayerWidget::set_volume (int vol)
{
    vol = max(0, min(SDL_MIX_MAXVOLUME, vol));
    old_volume = vol;
    if (adev)
        adev->set_volume(vol);
}

int AVPlayerWidget::get_volume () const
{
    return (adev ? adev->get_volume() : 0);
}

void AVPlayerWidget::set_frame_drop (bool drop)
{
    frame_drop = drop;
}

bool AVPlayerWidget::is_paused () const
{
    return paused;
}

bool AVPlayerWidget::is_stopped () const
{
    return stopped;
}

void AVPlayerWidget::set_size (int w, int h)
{
    if (!inited)
        return;

    vdev->resize(w, h);

    /* force refresh */
    if (vpaused || !vst)
       force_refresh();

    resize(w, h);
}

int AVPlayerWidget::show_msg (const char* msg, int ms)
{
    if (!inited)
        return KERROR(KEUNINITED);
    if (ms < 0 || !msg)
        return KERROR(KEINVAL);

    /* stop timer */
    if (timer.isActive())
        timer.stop();

    /* render message */
    msg_rect.x = 20;
    msg_rect.y = 20;
    int ret = msger->render_msg(msg, &msg_rect);
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s.", kerr2str(KERENDER_MSG_FAIL));
        return KERROR(KERENDER_MSG_FAIL);
    }
    msg_texture = msger->get_texture();
    if (ms) {
        timer.setInterval(ms);
        timer.setSingleShot(true);
        timer.start();
    }

    /* force refresh */
    force_refresh();

    logger.debug("Show message: \"%s\" for %d ms.\n", msg, ms);
    return 0;
}

void AVPlayerWidget::update_video ()
{
    if (inited)
        force_refresh();
}

void AVPlayerWidget::step ()
{
    if (!inited || stopped || !paused)
        return;

    step_req = true;
    SDL_LockMutex(pause_mutex);
    SDL_CondSignal(pause_cond);
    SDL_UnlockMutex(pause_mutex);

    /* sync audio */
    if (adec)
        adec->seek(vclk.get());

    logger.debug("step.\n");

}

void AVPlayerWidget::switch_fullscreen (bool fullscr)
{
    bool vpaused_old = vpaused;

    if (!inited || !vdev->height() || !vdev->width())
        return;

    vpause();
    if (fullscr) { // switch to fullscreen
        old_size = size();
        vdev->switch_fullscreen(fullscr);
        QRect first_scr_rect = QApplication::desktop()->screenGeometry(0);
        vdev->resize(first_scr_rect.width(), first_scr_rect.height());
    } else { // cancel fullcsreen
        vdev->switch_fullscreen(fullscr);
        resize(old_size);
        vdev->resize(old_size.width(), old_size.height());
    }
    if (vpaused_old)
        force_refresh();
    else
        vplay();
}

int AVPlayerWidget::clear_msg ()
{
    if (!inited)
        return KERROR(KEUNINITED);
  
    /* stop timer */
    if (timer.isActive())
        timer.stop();

    /* free texture */
    msg_texture = NULL;

    /* force_refresh */
    force_refresh();

    logger.debug("Message cleared.\n");
    return 0;
}

AVPlayerWidget::AVPlayerWidget (QWidget* parent)
    : QWidget(parent)
{
    inited = false;
    pause_mutex = NULL;
    pause_cond = NULL;
    vdev = NULL;
    vrefresh_thr = NULL;
    msger = NULL;
    reset_members();

    /* does not refresh when the window changed */
    setUpdatesEnabled(false);
}

AVPlayerWidget::~AVPlayerWidget ()
{
    if (inited)
        close();
}
