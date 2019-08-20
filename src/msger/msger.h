#ifndef _AVPLAYERWIDGET_MSGER_
#define _AVPLAYERWIDGET_MSGER_

#include <QObject>

extern "C" {
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
}

class Msger : public QObject {
    Q_OBJECT
    
private:
    TTF_Font *     font;
    SDL_Renderer * sdl_renderer;
    SDL_Texture *  texture; 

public:
    int          init        (const char *font_file, SDL_Renderer *renderer);
    int          render_msg  (const char *msg, SDL_Rect *rect);
    SDL_Texture *get_texture () const;
    void         close       ();
 
public:
    Msger           ();
    ~Msger          ();
};

#endif /* _AVPLAYERWIDGET_MSGER_ */