#ifndef _AVPLAYERWIDGET_VDEV_H_
#define _AVPLAYERWIDGET_VDEV_H_

#include <QWidget>

extern "C" {
#include "libavformat/avformat.h"
#include "SDL2/SDL.h"
}

class Vdev {
private:
    /* parent */
    QWidget *        parent;

    /* SDL window */
    SDL_Window *     window;

    /* SDL renderer */
    SDL_Renderer *   renderer;
    SDL_RendererInfo renderer_info;

    /* video device locker */
    SDL_mutex *      vdev_locker;
    bool             locked;

    /* window state */
    int              w;
    int              h;

    /* fps */
    int64_t          cur_time;
    int64_t          priv_time;
    int              fps;
    int              _fps;

public:
    int            init              (QWidget *parent, bool hw_acce);
    void           close             ();
    int            lock              ();
    int            unlock            ();
    int            upload_texture    (SDL_Texture *texture, SDL_Rect rect);
    void           resize            (int w, int h);
    int            width             () const;
    int            height            () const;
    int            get_fps           () const;
    SDL_Renderer * get_sdl_renderer  () const;
    void           switch_fullscreen (bool fullscr);

public:
    Vdev                             ();
    ~Vdev                            ();
};

#endif /* _AVPLAYERWIDGET_VDEV_H_ */
