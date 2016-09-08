#include "audio.h"

#include <SDL.h>
#include <string>

#include "res_path.h"

namespace carousel {

int InitSound(carousel::Carousel& carousel) {
  if (!carousel.click) { return 0; }
  std::string snd = carousel::GetResourcePath() + "click.wav";

  if (SDL_LoadWAV(snd.c_str(), &carousel.wav_spec, &carousel.wav_buffer,
                  &carousel.wav_length) == NULL) {
    return 1;
  }

  carousel.wav_spec.callback = AudioWriteCallback;
  carousel.wav_spec.userdata = &carousel;

  if (SDL_OpenAudio(&carousel.wav_spec, NULL) < 0) {
    fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
    return 1;
  }

  return 0;
}

void PlayClick(carousel::Carousel& carousel) {
  if (!carousel.click) { return; }
  carousel.audio_pos = carousel.wav_buffer;
  carousel.audio_len = carousel.wav_length;
  SDL_PauseAudio(0);
}

void PauseClick(carousel::Carousel& carousel) {
  if (!carousel.click) { return; }
  SDL_PauseAudio(1);
}

void DestroySound(carousel::Carousel& carousel) {
  if (!carousel.click) { return; }
  SDL_CloseAudio();
  SDL_FreeWAV(carousel.wav_buffer);
}

void AudioWriteCallback(void *userdata, Uint8 *stream, int len) {
  carousel::Carousel *carousel = (carousel::Carousel *)userdata;

  if (carousel->audio_len == 0) {
    PauseClick(*carousel);
    return;
  }

  len = ((Uint32)len > carousel->audio_len ? carousel->audio_len : len);
  SDL_memcpy(stream, carousel->audio_pos,
             len); // simply copy from one buffer into the other
  // SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);// mix from one
  // buffer into another

  carousel->audio_pos += len;
  carousel->audio_len -= len;

  if (carousel->audio_len == 0) {
    PauseClick(*carousel);
  }
}

} // namespace carousel
