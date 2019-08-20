#include <QtWidgets/QApplication>
#include "adev.h"
#include "error/error.h"
#include "log/log.h"

extern "C" {
#include "libavformat/avformat.h"
#include "SDL2/SDL.h"
}

#define FILENAME "adev.cpp"

void SDLCALL Adev::sdl_audio_callback (void * userdata, Uint8 * stream, int len)
{
    Adev *p = (Adev *)userdata;
    int   ret;

    while (!p->err_code &&!p->abort_req && len > 0) {
        /* fill buffer */
        if (!p->sample_buf.size) {
            p->sample_buf.pos = NULL;
            ret = (p->audio_fill_proc)(p->data, &p->sample_buf);
            if (ret < 0) {
                p->abort_req = true;
                if (KERROR(KEABORTED) != ret && KERROR(KEEOF) != ret)
                    emit p->err_occured(KERROR(KEAUDIO_FILL_FAIL));
                return;
            }
        }

        int len1 = len > p->sample_buf.size ? p->sample_buf.size : len;
        if (!p->muted && p->volume == SDL_MIX_MAXVOLUME)
            memcpy(stream, p->sample_buf.pos, len1);
        else {
            memset(stream, 0, len1);
            SDL_MixAudioFormat(stream, p->sample_buf.pos, AUDIO_S16SYS, len1, p->volume);
        }
        len -= len1;
        stream += len1;
        p->sample_buf.size -= len1;
        p->sample_buf.pos += len1;
    }
    return;
}

Adev::Adev (AudioFillProc audio_fill_proc, void *data)
{
    this->audio_fill_proc = audio_fill_proc;
    this->data = data;
    volume = SDL_MIX_MAXVOLUME;
    paused = true;
    muted = !volume ? true : false;
    abort_req = false;
    cur_af_pts = 0.0;
    sample_buf.buf = sample_buf.pos = NULL;
    sample_buf.size = 0;
    err_code = 0;
}

Adev::~Adev ()
{
}

int Adev::init (AudioParams wanted_params, AudioParams *tgt_params)
{
    SDL_AudioSpec wanted_spec;
    int           next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;
    int           ret;

    /* fill audio params */
    wanted_spec.channels = (Uint8)wanted_params.channels;
    wanted_spec.freq = wanted_params.sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.samples = (Uint16)wanted_params.nb_samples;
    wanted_spec.silence = 0;
    wanted_spec.userdata = this;
    wanted_spec.callback = sdl_audio_callback;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        logger.ERRORN("[%s: %d]%s.\n", kerr2str(KEWRONG_AUDIO_PARAMS));
        return KERROR(KEWRONG_AUDIO_PARAMS);
    }

    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
        next_sample_rate_idx--;

    /* open audio devide */
    while (!(adev_id = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE ))) {
        av_log(NULL, AV_LOG_WARNING, "Audio Device[%d channels, %d Hz]: %s\n",
               wanted_spec.channels, wanted_spec.freq, SDL_GetError());
        wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
        if (!wanted_spec.channels) {
            wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
            wanted_spec.channels = wanted_params.channels;
            if (!wanted_spec.freq) {
                logger.ERRORN("[%s: %d]%s %d.\n", kerr2str(KEUNSUPPORTED_AUDIO_FREQ), wanted_params.sample_rate);
                return KERROR(KEUNSUPPORTED_AUDIO_FREQ);
            }
        }
    }
    if (spec.format != AUDIO_S16SYS) {
        logger.FATALN("[%s: %d]%s %d\n", kerr2str(KEUNSUPPORTED_AUDIO_DATA_FORMAT), spec.format);
        return KERROR(KEUNSUPPORTED_AUDIO_DATA_FORMAT);
    }
    if (spec.channels != wanted_spec.channels) {
        logger.FATALN( "[%s: %d]%s %d", kerr2str(KEUNSUPPORTED_AUDIO_CHANNEL), spec.channels);
        return KERROR(KEUNSUPPORTED_AUDIO_CHANNEL);
    }

    /* init output params */
    tgt_params->channel_layout = av_get_default_channel_layout(wanted_spec.channels);
    tgt_params->channels = spec.channels;
    tgt_params->sample_rate = spec.freq;
    tgt_params->sample_fmt = AV_SAMPLE_FMT_S16;
    tgt_params->nb_samples = wanted_params.nb_samples;

    /* allocate sample buffer */
    int sample_buf_size = (Uint32)av_samples_get_buffer_size(NULL,
                                                     tgt_params->channels,
                                                     tgt_params->nb_samples,
                                                     tgt_params->sample_fmt,
                                                     1);
    sample_buf.buf = (uint8_t *)av_mallocz((size_t)sample_buf_size);
    if (!sample_buf.buf)
        return KERROR(KENOMEM);

    /* pause */
    SDL_PauseAudioDevice(adev_id, 1);

    logger.debug("Audio device has been inited.\n");

    return 0;
}

void Adev::pause ()
{
    if (!adev_id)
        return;

    abort_req = true;
    SDL_PauseAudioDevice(adev_id, 1);
    paused = true;

    logger.debug("Audio device paused.\n");
}

void Adev::play ()
{
    if (!adev_id)
        return;

    abort_req = false;
    SDL_PauseAudioDevice(adev_id, 0);
    paused = false;

    logger.debug("Audio device started.\n");
}

void Adev::close ()
{
    if (!adev_id)
        return;

    abort_req = true;
    SDL_CloseAudioDevice(adev_id);
    av_freep(&sample_buf.buf);
    
    logger.debug("Audio device closed.\n");
}

void Adev::flush ()
{
    if (paused && adev_id)
        SDL_ClearQueuedAudio(adev_id);

    logger.debug("Audio device flushed.\n");
}

void Adev::set_volume (int vol)
{
    volume = vol;
    muted = !vol ? true : false;
}

int Adev::get_volume ()
{
    return volume;
}

void Adev::set_cur_af_pts (double pts)
{
    cur_af_pts = pts;
}

double Adev::get_cur_af_pts () const
{
    return cur_af_pts;
}

int Adev::get_err_code() const
{
    return err_code;
}
