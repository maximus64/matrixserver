#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/future.hpp>
#include <cstdint>
#include "SoundEffect.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

namespace {

#include "SoundSamples.inc"

static void playSoundBuffer(const int16_t *buffer, size_t frames) {
    int err;
    snd_pcm_t *handle;

    if (!frames)
        return;

    if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            printf("Playback open error: %s\n", snd_strerror(err));
            return;
    }
    if ((err = snd_pcm_set_params(handle,
                                SND_PCM_FORMAT_S16_LE,
                                SND_PCM_ACCESS_RW_INTERLEAVED,
                                1,
                                44100,
                                1,
                                20000)) < 0) {
            printf("Playback open error: %s\n", snd_strerror(err));
            snd_pcm_close(handle);
            return;
    }
    err = snd_pcm_writei(handle, buffer, frames);
    if (err < 0)
        err = snd_pcm_recover(handle, err, 0);
    if (err < 0) {
        printf("snd_pcm_writei error: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
        return;
    }

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
}

}

SoundEffect::SoundEffect() {

}

void SoundEffect::playSound(Sound sid){
    const int16_t *ptr;
    size_t frames = 0;

    switch (sid) {
    case charging:
        ptr = charging_sound;
        frames = ARRAY_SIZE(charging_sound);
        break;
    case select:
        ptr = select_sound;
        frames = ARRAY_SIZE(select_sound);
        break;
    default:
        printf("Unknow sound id: %d\n", sid);
        return;
    }

    boost::async( [=]{ return playSoundBuffer(ptr, frames); } );
}
