#ifndef AUDIO_H
#define AUDIO_H

#include "carousel.h"

namespace carousel {

int InitSound(carousel::Carousel& carousel);
void DestroySound(carousel::Carousel& carousel);
void PlayClick(carousel::Carousel& carousel);
void PlayBlip(carousel::Carousel& carousel);
void PauseClick(carousel::Carousel& carousel);

void AudioWriteCallback(void* userdata, Uint8* stream, int len);

}  // namespace carousel

#endif
