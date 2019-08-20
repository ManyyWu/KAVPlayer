#include <QCoreApplication>
#include "msger.h"
#include "error/error.h"
#include "log/log.h"

extern "C" {
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
}

#define FILENAME "msger.h"

int Msger::init (const char * font_file, SDL_Renderer *renderer)
{
#if defined(__MACOSX__) || defined(__LINUX__)
    return 0;
#endif

    if (sdl_renderer)
        return KERROR(KEREINIT);

    if (!font_file || !renderer)
        return KERROR(KENOMEM);

    sdl_renderer = renderer;

    /* init SDL TTF */
    int ret = TTF_Init();
    if (ret < 0) {
        logger.FATALN("[%s: %d]%s: %s.\n", kerr2str(KETTF_INIT_FAIL), SDL_GetError());
        GOTO_FAIL(KETTF_INIT_FAIL);
    }

    /* open font */
    font = TTF_OpenFont((QCoreApplication::applicationDirPath() + "/fonts/stkaiti.ttf").toStdString().c_str(), 20);
    if (!font) { 
        logger.FATALN("[%s: %d]%s: %s", kerr2str(KEOPEN_FONT_FAIL), TTF_GetError());
        GOTO_FAIL(KEOPEN_FONT_FAIL);
    }

    logger.debug("Mesger has been inited.\n");

fail:
    if (ret < 0)
        close();

    return ret;
}

int Msger::render_msg (const char *msg, SDL_Rect *rect)
{
#if defined(__MACOSX__) || defined(__LINUX__)
    return 0;
#endif

    if (!sdl_renderer)
        return KERROR(KEUNINITED);

    if (!msg || !rect)
        return KERROR(KENOMEM);


    SDL_Color msg_color = {255, 0, 0};
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, msg, msg_color);
    if (!surf) {
        logger.FATALN("[%s: %d]%s: %s", kerr2str(KECREATE_SDL_SURFACE_FAIL), SDL_GetError());
        return KERROR(KECREATE_SDL_SURFACE_FAIL);
    }
    TTF_SizeUTF8(font, msg, &rect->w, &rect->h);
    if (texture)
        SDL_DestroyTexture(texture);
    texture = SDL_CreateTextureFromSurface(sdl_renderer, surf);
    SDL_FreeSurface(surf);
    if (!texture) {
        logger.FATALN("[%s: %d]%s: %s", kerr2str(KECREATE_TEXTURE_FAIL), SDL_GetError());
        return KERROR(KECREATE_TEXTURE_FAIL);
    }
     
    return 0;
}

SDL_Texture * Msger::get_texture () const
{
    return texture;
}

void Msger::close ()
{
#if defined(__MACOSX__) || defined(__LINUX__)
    return;
#endif

    if (!sdl_renderer)
        return;

    /* close font */
    if (font) 
        TTF_CloseFont(font);
    font = NULL;

    /* free texture */
    if (texture)
        SDL_DestroyTexture(texture);
    texture = NULL;

    /* deinit TTF */
    TTF_Quit();

    sdl_renderer = NULL;

    logger.debug("Mesger closed.\n");
}

Msger::Msger ()
    : QObject()
{
    font = NULL;
    sdl_renderer = NULL;
    texture = NULL;
}

Msger::~Msger ()
{
    close();
}
