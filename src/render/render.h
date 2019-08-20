#ifndef _AVPLAYERWIDGET_WINDOW_H_ 
#define _AVPLAYERWIDGET_WINDOW_H_ 

#include <QObject>
#include "clock/clock.h"
#include "queue/frame_queue.h"
#include "adev/adev.h"
#include "vdev/vdev.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "SDL2/SDL.h"
}

#define AV_SYNC_THRESHOLD_MIN       0.04 // no AV sync correction is done if below the minimum AV sync threshold
#define AV_SYNC_THRESHOLD_MAX       0.1  // AV sync correction is done if above the maximum AV sync threshold 
#define AV_SYNC_FRAMEDUP_THRESHOLD  0.1  // if a frame duration is longer than this, it will not be duplicated to compensate AV sync
#define AV_SYNC_FRAMEDROP_THRESHOLD 0.5 
#define AV_SYNC_DELAY_MAX           2.0  // max delay in seconds

/* speed */
#define MAX_SPEED 4.0
#define MIN_SPEED 0.1
#define DEF_SPEED 1.0

/* texture format map */
static const struct TextureFormatEntry {
    enum AVPixelFormat format;
    int  texture_fmt;
} sdl_texture_format_map[] = {
    { AV_PIX_FMT_RGB8,           SDL_PIXELFORMAT_RGB332 },
    { AV_PIX_FMT_RGB444,         SDL_PIXELFORMAT_RGB444 },
    { AV_PIX_FMT_RGB555,         SDL_PIXELFORMAT_RGB555 },
    { AV_PIX_FMT_BGR555,         SDL_PIXELFORMAT_BGR555 },
    { AV_PIX_FMT_RGB565,         SDL_PIXELFORMAT_RGB565 },
    { AV_PIX_FMT_BGR565,         SDL_PIXELFORMAT_BGR565 },
    { AV_PIX_FMT_RGB24,          SDL_PIXELFORMAT_RGB24 },
    { AV_PIX_FMT_BGR24,          SDL_PIXELFORMAT_BGR24 },
    { AV_PIX_FMT_0RGB32,         SDL_PIXELFORMAT_RGB888 },
    { AV_PIX_FMT_0BGR32,         SDL_PIXELFORMAT_BGR888 },
    { AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888 },
    { AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888 },
    { AV_PIX_FMT_RGB32,          SDL_PIXELFORMAT_ARGB8888 },
    { AV_PIX_FMT_RGB32_1,        SDL_PIXELFORMAT_RGBA8888 },
    { AV_PIX_FMT_BGR32,          SDL_PIXELFORMAT_ABGR8888 },
    { AV_PIX_FMT_BGR32_1,        SDL_PIXELFORMAT_BGRA8888 },
    { AV_PIX_FMT_YUV420P,        SDL_PIXELFORMAT_IYUV },
    { AV_PIX_FMT_YUYV422,        SDL_PIXELFORMAT_YUY2 },
    { AV_PIX_FMT_UYVY422,        SDL_PIXELFORMAT_UYVY },
    { AV_PIX_FMT_NONE,           SDL_PIXELFORMAT_UNKNOWN }
};

class Render {
private:
    /* speed */
    double         speed;
    
    /* audio params */
    AudioParams    ap_src;
    AudioParams    ap_tgt;

    /* sdl renderer */
    SDL_Renderer * sdl_renderer;

    /* sdl texture */
    SDL_Texture *  vid_texture;

    /* swr context */
    SwrContext *   swr_ctx;

    /* sws context */
    SwsContext *   sws_ctx;

    /* frame queues */
    FrameQueue *   vfq;
    FrameQueue *   afq;

    /* mutex and cond */
    SDL_mutex *    wait_mutex;
    SDL_cond *     empty_queue_cond;
    
private:
    int         init_swr           ();
    int         alloc_texture      (int fmt, int w, int h, SDL_BlendMode blend_mode);
    int         render_video_image (Frame *vf, SDL_Texture **texture);

public:
    void        init_vrender       ();
    void        close_vrender      ();
    int         init_arender       (AudioParams ap_src, AudioParams ap_tgt);
    void        close_arender      ();
    int         resample           (AVFrame *vf, SampleBuf *sample_buf);
    int         render_video_frame (Frame *vf, SDL_Texture **texture);
    AudioParams get_ap_tgt         () const;
    void        set_speed          (double speed);

public:
    Render                         (SDL_Renderer *sdl_renderer, FrameQueue *vfq, FrameQueue *afq,
                                    SDL_mutex *wait_mutex, SDL_cond *empty_queue_cond);
    ~Render                        ();
};

#endif /* _AVPLAYERWIDGET_WINDOW_H_ */
