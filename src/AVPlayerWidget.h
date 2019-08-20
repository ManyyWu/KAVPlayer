#ifndef _AVPLAYERWIDGET_H_
#define _AVPLAYERWIDGET_H_

#include <QEvent>
#include <QWidget>
#include <QRect>
#include <QTimer>
#include "avplayerwidget_global.h"
#include "decoder/decoder.h"
#include "demux/demux.h"
#include "render/render.h"
#include "msger/msger.h"
#include "clock/clock.h"
#include "vdev/vdev.h"
#include "adev/adev.h"
#include "log/log.h"

extern "C" 
{
#include "libavformat/avformat.h"
#include "SDL2/SDL.h"
}

/* media info */
class AVPLAYERWIDGET_EXPORT MediaInfo {
private:
    
public:
    MediaInfo ();
    ~MediaInfo ();
};

class AVPLAYERWIDGET_EXPORT AVPlayerWidget : public QWidget {
    Q_OBJECT

private:
    /* player params */
    bool             inited;
    int              wanted_vst;
    int              wanted_ast;
    bool             frame_drop;
    bool             hw_acce;
    bool             infinite_buf;
    int              max_pktq_size;
    int              max_pictq_len;
    int              max_sampleq_len;
    bool             realtime;
    double           speed;
    bool             close_req;
    bool             paused;
    bool             vstop_req;
    bool             vstopped;
    bool             stop_req;
    bool             stopped;
    bool             vpause_req;
    bool             vpaused;
    double           start_time;
    double           duration;
    double           delay;
    QSize            old_size;
    int              old_volume;
    int              err_code;

    /* url */
    const char *     url;

    /* contex */
    AVFormatContext *avfctx;

    /* media streams */
    int              vst_idx;
    int              ast_idx;
    AVStream *       vst;
    AVStream *       ast;

    /* queues */
    PacketQueue *    vpktq;
    PacketQueue *    apktq;
    FrameQueue *     vfq;
    FrameQueue *     afq;

    /* dev */
    Vdev *           vdev;
    Adev *           adev;

    /* msger */
    Msger *          msger;

    /* render */
    Render *         render;

    /* decoders */
    Decoder *        vdec;
    Decoder *        adec;

    /* demux */
    Demux *          demux;

    /* threads */
    SDL_Thread *     vrefresh_thr;

    /* clock */
    Clock            priclk;
    Clock            vclk;
    double           spare_clock;

    /* mutex and condition variable */
    SDL_mutex *      wait_mutex;
    SDL_cond *       continue_read_cond;
    SDL_mutex *      pause_mutex;
    SDL_cond *       pause_cond;

    /* video state */
    Frame *          priv_vf;

    /* max video frame duration */
    double           max_frame_duration;

    /* message texture */
    SDL_Texture *    msg_texture;
    SDL_Rect         msg_rect;
    QTimer           timer;

    /* force refresh */
    bool             force_refresh_req;
    bool             step_req;
    SDL_Texture *    cur_texture;

signals:
    void               player_playing         ();
    void               player_paused          ();
    void               player_stopped         (int);
    void               player_seeked          ();
    void               pos_changed            (double);

signals:
    void               err_occured            (int);

private slots:
    void               stop                   (int err_code);

private:
    void               mouseDoubleClickEvent  (QMouseEvent *e);
    void               keyPressEvent          (QKeyEvent *e);

private:
    void               force_refresh          ();
    void               reset_members          ();
    double             compute_delay          (Frame *priv_vf, Frame *cur_vf);
    void               calculate_display_rect (AVFrame *vf, SDL_Rect *rect);
    int                video_refresh          ();
    bool               is_realtime            ();
    int                open_media_file        (const char *url);
    int                init_queues            (int max_pictq_len, int max_sampleq_len);
    void               vplay                  ();
    void               vpause                 ();
    void               aplay                  ();
    void               apause                 ();

private:
    static int         audio_fill_proc        (void *data, SampleBuf *sample_buf);

public:
    int                get_media_info         (const char *url, MediaInfo *info);

private:
    static int SDLCALL vrefresh_thread        (void *args);

public:
    int                init                   (bool en_hw_acce);
    int                open                   (const char *url);
    void               stop                   ();
    void               play                   ();
    void               pause                  ();
    void               close                  ();
    int                seek                   (double pos);
    double             get_pos                ();
    int                get_fps                ();
    double             get_duration           ();
    void               set_volume             (int vol);
    int                get_volume             () const;
    void               set_frame_drop         (bool drop);
    bool               is_paused              () const;
    bool               is_stopped             () const;
    void               set_size               (int w, int h);
    int                show_msg               (const char *msg, int ms);
    void               update_video           ();
    void               step                   ();
    void               switch_fullscreen      (bool fullscr);

public slots:
    int                clear_msg              ();

private:
    AVPlayerWidget                             ();
    AVPlayerWidget                             (AVPlayerWidget &widget);

public:
    AVPlayerWidget                             (QWidget *parent);
    ~AVPlayerWidget                            ();
};

#endif /* _AVPLAYERWIDGET_H_ */
