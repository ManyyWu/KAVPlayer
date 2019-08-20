#include <QWidget>
#include <stdio.h>
#include "vdev.h"
#include "error/error.h"
#include "log/log.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/time.h"
#include "SDL2/SDL.h"
}

#define FILENAME "vdev.cpp"

int Vdev::init (QWidget * parent, bool hw_acce)
{
    bool error = false;

    if (!parent)
        return KERROR(KEINVAL);
    if (vdev_locker)
        return KERROR(KEREINIT);

    this->parent = parent;

    /* create SDL window */
#if defined(__MACOSX__) || defined(__LINUX__)
    window = SDL_CreateWindow("KAVPlayer", 400, 400, 600, 400, SDL_WINDOW_SHOWN);
#else
    window = SDL_CreateWindowFrom((void *)parent->winId());
#endif
    if (!window) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_WINDOW_FAIL), SDL_GetError());
        return KERROR(KECREATE_SDL_WINDOW_FAIL);
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_ShowWindow(window);
    w = parent->width();
    h = parent->height();

    /* create SDL renderer */
create_renderer:
    renderer = SDL_CreateRenderer(window, -1, hw_acce ? SDL_RENDERER_ACCELERATED : SDL_RENDERER_SOFTWARE | SDL_TEXTUREACCESS_STREAMING); // SDL_TEXTUREACCESS_STREAMING 不限制帧率
    if (renderer) {
        /* get renderer info */
        if (!SDL_GetRendererInfo(renderer, &renderer_info)) {
            logger.info("Initialized %s renderer.\n", renderer_info.name);
        } else {
err:
            logger.warning("[%s: %d]%s: %s.\n", kerr2str(KEGET_SDL_RENDERER_INFO_FAIL), SDL_GetError());
            return KERROR(KEGET_SDL_RENDERER_INFO_FAIL);
        }
        if (!renderer_info.num_texture_formats)
            goto err;
    } else {
        /* try to use software renderer */
        if (hw_acce)
            logger.warning("Failed to create hardware renderer.\n");
        else 
            logger.warning("Failed to create software renderer.\n");
        if (!error) {
            error = true;
            hw_acce = !hw_acce;
            goto create_renderer;
        }

        logger.fatal("[%s: %d]%s: %s.\n\n", kerr2str(KECREATE_SDL_RENDERER_FAIL), SDL_GetError());
        return KERROR(KECREATE_SDL_RENDERER_FAIL);
    }

    /* clear renderer */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, NULL);
    SDL_RenderPresent(renderer);

    /* create video device locker */
    vdev_locker = SDL_CreateMutex();
    if (!vdev_locker) {
        logger.fatal("[%s: %d]%s: %s.\n", kerr2str(KECREATE_SDL_MUTEX_FAIL), SDL_GetError());
        return KERROR(KECREATE_SDL_MUTEX_FAIL);
    }
    locked = false;

    /* reset fps */
    fps = _fps = 0;

    logger.debug("Vdev has been inited.\n");
    return 0;
}

void Vdev::close ()
{
    if (!vdev_locker)
        return;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (locked)
        SDL_UnlockMutex(vdev_locker);
    locked = false;
    SDL_DestroyMutex(vdev_locker);
    parent = NULL;
    renderer = NULL;
    window = NULL;
    vdev_locker = NULL;
    fps = _fps = 0;

    logger.debug("Vdev closed.\n");
}

int Vdev::lock ()
{
    if (!vdev_locker)
        return KERROR(KEUNINITED);

    SDL_LockMutex(vdev_locker);
    locked = true;

    /* clear renderer */  
    SDL_RenderClear(renderer);

    /* set background color of renderer */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    return 0;
}

int Vdev::unlock ()
{
    if (!vdev_locker)
        return KERROR(KEUNINITED);

    /* present */
    SDL_RenderPresent(renderer);

    /* compute fps */
    cur_time = av_gettime();
    _fps++;
    if (cur_time - priv_time > AV_TIME_BASE) {
        priv_time = cur_time;
        fps = _fps;
        _fps = 0;
    }

    SDL_UnlockMutex(vdev_locker);
    locked = false;

    return 0;
}

int Vdev::upload_texture (SDL_Texture * texture, SDL_Rect rect)
{
    if (!vdev_locker)
        return KERROR(KEUNINITED);

    if (!texture)
        return 0;

    int ret = SDL_RenderCopy(renderer, texture, NULL, &rect);
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KEUPDATE_VIDEO_FAIL), SDL_GetError());
        return KERROR(KEUPDATE_VIDEO_FAIL);
    }

    return 0;
}

void Vdev::resize (int w, int h)
{
    SDL_LockMutex(vdev_locker);

    this->w = w;
    this->h = h;

    if (window)
        SDL_SetWindowSize(window, w, h);

    SDL_UnlockMutex(vdev_locker);
}

int Vdev::width () const
{
    if (!window)
        return KERROR(KEUNINITED);
    else
        return w;
}

int Vdev::height () const
{
    if (!window)
        return KERROR(KEUNINITED);
    else
        return h;
}

int Vdev::get_fps () const
{
    return vdev_locker ? fps : 0;
}

SDL_Renderer * Vdev::get_sdl_renderer () const
{
    return renderer;
}

void Vdev::switch_fullscreen (bool fullscr)
{
    if (!vdev_locker)
        return;
    SDL_LockMutex(vdev_locker);
    if (!fullscr) {
        parent->setWindowFlags(Qt::SubWindow);
        parent->showNormal();
    } else {
        parent->setWindowFlags(Qt::Window);
        parent->showFullScreen();
    }
    SDL_UnlockMutex(vdev_locker);
}

Vdev::Vdev ()
{
    parent = NULL;
    renderer = NULL;
    window = NULL;
    vdev_locker = NULL;
}

Vdev::~Vdev ()
{
    if (vdev_locker)
        close();
}
