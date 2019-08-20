#ifndef _AVPLAYERWIDGET_ADEV_H_
#define _AVPLAYERWIDGET_ADEV_H_

#include <QObject>

extern "C" {
#include "libavformat/avformat.h"
#include "SDL2/SDL.h"
}

/* audio parameters */
typedef struct AudioParams {
    int                 sample_rate;
    int                 channels;
    int64_t             channel_layout; 
    enum AVSampleFormat sample_fmt;
    int                 nb_samples;
}AudioParams;

/* sample buffer */
typedef struct SampleBuf {
    Uint8 *             buf;
    Uint8 *             pos;
    Uint32              size;
}SampleBuf;

typedef int (*AudioFillProc) (void *, SampleBuf *);

class Adev : public QObject {
    Q_OBJECT

private:
    /* audio params */
    SDL_AudioSpec spec;

    /* audio device */
    int           adev_id;

    /* audio buffer */
    SampleBuf     sample_buf;

    /* fill proc */
    AudioFillProc audio_fill_proc; // audio fill process function 

    /* render */   
    void *        data;

    /* audio device status */
    bool          abort_req;
    bool          muted;
    int           volume;
    bool          paused;

    /* pts */
    double        cur_af_pts;

    /* error code */
    int           err_code;

public:
    const int     next_nb_channels[8] = {0, 0, 1, 6, 2, 6, 4, 6};
    const int     next_sample_rates[5] = {0, 44100, 48000, 96000, 192000};

signals:
    void                err_occured        (int err_code);

private:
    static void SDLCALL sdl_audio_callback (void *userdata, Uint8 * stream, int len);

public:
    int                 init               (AudioParams wanted_params, AudioParams *tgt_params);
    void                pause              ();
    void                play               ();
    void                close              ();
    void                flush              ();
    void                set_volume         (int vol);
    int                 get_volume         ();
    void                set_cur_af_pts     (double pts);
    double              get_cur_af_pts     () const;
    int                 get_err_code       () const;

public:
    Adev                                   (AudioFillProc audio_fill_proc, void *data);
    ~Adev                                  ();
};

#endif /* _AVPLAYERWIDGET_ADEV_H_ */
