#include "error/error.h"
#include "log/log.h"
#include "render.h"
#include "vdev/vdev.h"
#include "adev/adev.h"
#include <cstring>
#include <new>

extern "C"
{
#include "libavutil/imgutils.h"
#include "libavutil/avutil.h"
#include "libavutil/time.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "SDL2/SDL.h"
}

#define FILENAME "render.cpp"

int Render::init_swr ()
{
    int ret;

    /* init swr */
    swr_ctx = swr_alloc();
    if (!swr_ctx)
        return KERROR(KENOMEM);
    swr_ctx = swr_alloc_set_opts(swr_ctx,
                                 ap_tgt.channel_layout,
                                 ap_tgt.sample_fmt,
                                 ap_tgt.sample_rate,
                                 ap_src.channel_layout,
                                 ap_src.sample_fmt,
                                 ap_src.sample_rate,
                                 0, NULL);
    ret = swr_init(swr_ctx);
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KESWR_INIT_FAIL), av_err2str(ret));
        return KERROR(KESWR_INIT_FAIL);
    }

    return 0;
}

int Render::alloc_texture (int fmt, int w, int h, SDL_BlendMode blend_mode)
{
    if (!(vid_texture = SDL_CreateTexture(sdl_renderer,
                                          fmt, 
                                          SDL_TEXTUREACCESS_STREAMING,
                                          w, h)))
        {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_TEXTURE_FAIL), SDL_GetError());
        return KERROR(KECREATE_TEXTURE_FAIL); 
    }
    if (SDL_SetTextureBlendMode(vid_texture, blend_mode) < 0) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KESET_TEXTURE_BLEND_MODE_FAIL), SDL_GetError());
        return KERROR(KESET_TEXTURE_BLEND_MODE_FAIL);
    }
//    logger.info("Created %dx%d texture with %s.\n", 
//                w, h, SDL_GetPixelFormatName(fmt));

    return 0;
}

int Render::render_video_image (Frame * vf, SDL_Texture ** texture)
{
    int           sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
    SDL_BlendMode sdl_blend_mode = SDL_BLENDMODE_NONE;
    uint8_t *     yuv_buf;
//    AVFrame *    yuv_vf = av_frame_alloc();
    uint8_t **    data = vf->frame->data;
    int *         linesize = vf->frame->linesize;
    int           ret;

//    if (!yuv_vf)
//        return KERROR(KENOMEM);
    
//    /* fill yuv frame buffer */
//    yuv_buf = (uint8_t *)av_malloc((size_t)av_image_get_buffer_size(
//                                   (AVPixelFormat)vf->format, rect.w, rect.h, 1));
//    if (!yuv_buf)
//        GOTO_FAIL(KENOMEM);
//    yuv_vf->width = rect.w;
//    yuv_vf->height = rect.h;
//    yuv_vf->pts = vf->pts;
//    ret = av_image_fill_arrays(yuv_vf->data,
//                               yuv_vf->linesize,
//                               yuv_buf,
//                               (AVPixelFormat)vf->format,
//                               rect.w, rect.h, 1);
//    if (ret < 0) {
//        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KEAV_IMAGE_FILL_ARRAY_FAIL), av_err2str(ret));
//        GOTO_FAIL(KEAV_IMAGE_FILL_ARRAY_FAIL);
//    }

    /* set pixel format and blend mode */
    sdl_pix_fmt = SDL_PIXELFORMAT_UNKNOWN;
    sdl_blend_mode = SDL_BLENDMODE_NONE;
    if (vf->frame->format == AV_PIX_FMT_RGB32   ||
        vf->frame->format == AV_PIX_FMT_RGB32_1 ||
        vf->frame->format == AV_PIX_FMT_BGR32   ||
        vf->frame->format == AV_PIX_FMT_BGR32_1)
        sdl_blend_mode = SDL_BLENDMODE_BLEND;
    for (int i = 0; i < FF_ARRAY_ELEMS(sdl_texture_format_map) - 1; i++) {
        if (vf->frame->format == sdl_texture_format_map[i].format) {
            sdl_pix_fmt = sdl_texture_format_map[i].texture_fmt;
            break;
        }
    }

//    /* realloc sws */
//    ret = realloc_sws(vf, rect.w, rect.h, vf->format);
//    if (ret < 0) {
//        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KEREALLOC_SWS_FAIL), av_err2str(ret));
//        GOTO_FAIL(KEREALLOC_SWS_FAIL);
//    }
//
//    /* scale */
//    ret = sws_scale(sws_ctx, vf->data, vf->linesize, 0, vf->height, 
//                    yuv_vf->data, yuv_vf->linesize);
//    if (ret < 0) {
//        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KESWS_SCALE_FAIL), av_err2str(ret));
//        GOTO_FAIL(KESWS_SCALE_FAIL);
//    }

    /* update texture */

    /* realloc texture */
//    ret = realloc_texture(texture, sdl_pix_fmt, rect->w, rect->h, sdl_blend_mode);
    ret = alloc_texture(sdl_pix_fmt, vf->frame->width, vf->frame->height, sdl_blend_mode);
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s.\n", kerr2str(KEREALLOC_TEXTURE_FAIL));
        return KERROR(KEREALLOC_TEXTURE_FAIL);
    }

    /* update texture */
    switch (sdl_pix_fmt) {
    case SDL_PIXELFORMAT_UNKNOWN:
        break;
    case SDL_PIXELFORMAT_IYUV:
        if (linesize[0] > 0 && linesize[1] > 0 && linesize[2] > 0) {
            ret = SDL_UpdateYUVTexture(vid_texture, NULL,
                                       data[0], linesize[0],
                                       data[1], linesize[1],
                                       data[2], linesize[2]);
        } else if (linesize[0] < 0 && linesize[1] < 0 && linesize[2] < 0) {
            ret = SDL_UpdateYUVTexture(vid_texture, NULL,
                                       data[0] + linesize[0] * (vf->frame->height - 1), -linesize[0],
                                       data[1] + linesize[1] * (AV_CEIL_RSHIFT(vf->frame->height, 1) - 1), -linesize[1],
                                       data[2] + linesize[2] * (AV_CEIL_RSHIFT(vf->frame->height, 1) - 1), -linesize[2]);
        } else {
            logger.ERRORN("[%s: %d]%s.\n", kerr2str(KEUNSUPPORTED_PIXFORMAT));
            return KERROR(KEUNSUPPORTED_PIXFORMAT);
        }
        break;
    default:
        if (linesize[0] < 0) {
            ret = SDL_UpdateTexture(vid_texture, NULL, data[0] + linesize[0] * (vf->frame->height - 1), -linesize[0]);
        } else {
            ret = SDL_UpdateTexture(vid_texture, NULL, data[0], linesize[0]);
        }
        break;
    }
    if (ret < 0) {
        logger.ERRORN("[%s: %d]%s: %s.\n", kerr2str(KEUPDATE_TEXTURE_FAIL), SDL_GetError());
        return KERROR(KEUPDATE_TEXTURE_FAIL);
    }

    *texture = vid_texture;
    vid_texture = NULL;
    ret = 0;
fail:
//    av_frame_free(&yuv_vf);
//    av_freep(&yuv_buf);/////////////////////////////////////
    return ret;
}

int Render::resample (AVFrame *vf, SampleBuf *sample_buf)
{
    int ret;

    if (!swr_ctx)
        return KERROR(KEUNINITED);

    /* swr conversion */
    int sample_buf_size = (Uint32)av_samples_get_buffer_size(NULL,
                                                     ap_tgt.channels,
                                                     ap_tgt.nb_samples,
                                                     ap_tgt.sample_fmt,
                                                     1);
    ret = swr_convert(swr_ctx,
                      &sample_buf->buf,
                      sample_buf_size / ap_tgt.channels,
                      (const uint8_t **)vf->data,
                      vf->nb_samples);
    if (ret < 0) {
        logger.FATALN( "[%s: %d]%s: %s.\n", kerr2str(KESWR_CONVERT_FAIL), av_err2str(ret));
        return KERROR(KESWR_CONVERT_FAIL);
    }
    sample_buf->pos = sample_buf->buf;
    sample_buf->size = (Uint32)ret * ap_tgt.channels * av_get_bytes_per_sample(ap_tgt.sample_fmt);

    return ret;
}

void Render::init_vrender ()
{
    vid_texture = NULL;
}

void Render::close_vrender ()
{
    if (!sws_ctx)
        return;

    if (vid_texture)
        SDL_DestroyTexture(vid_texture);
    vid_texture = NULL;
    sws_freeContext(sws_ctx);
    sws_ctx = NULL;
}

int Render::init_arender (AudioParams ap_src, AudioParams ap_tgt)
{
    this->ap_src = ap_src;
    this->ap_tgt = ap_tgt;

    return init_swr();
}

void Render::close_arender ()
{
    if (!swr_ctx)
        return;
    
    swr_free(&swr_ctx);
}

int Render::render_video_frame (Frame * vf, SDL_Texture ** texture)
{
//    if (!sws_ctx)
//        return KERROR(KEUNINITED);

    if (!vf->frame->width || !vf->frame->height) // fix bad frame
        return 0;

    int ret = render_video_image(vf, texture);
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s.\n", kerr2str(KEVIDEO_IMAGE_DISPLAY_FAIL));
        return KERROR(KEVIDEO_IMAGE_DISPLAY_FAIL);
    }

    return 0;
}

AudioParams Render::get_ap_tgt () const
{
    return ap_tgt;
}

void Render::set_speed (double speed)
{
    this->speed = speed;
}

Render::Render (SDL_Renderer * sdl_renderer, FrameQueue * vfq, FrameQueue * afq, SDL_mutex * wait_mutex, SDL_cond * empty_queue_cond)
{
    this->sdl_renderer = sdl_renderer;
    this->vfq = vfq;
    this->afq = afq;
    this->wait_mutex = wait_mutex;
    this->empty_queue_cond = empty_queue_cond;
    swr_ctx = NULL;
    sws_ctx = NULL;
    speed = 1.0;
}

Render::~Render ()
{
    if (sws_ctx)
        close_vrender();
    if (swr_ctx)
        close_arender();
}

