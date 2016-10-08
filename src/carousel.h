#ifndef CAROUSEL_H
#define CAROUSEL_H

#include <SDL.h>
#include <map>
#include <string>
#include <vector>

#ifdef ALSA_FOUND
#include <alsa/asoundlib.h>
#include <alsa/mixer.h>
#endif

// The percentage of the screen height a full sized card should be.
#define HOME_HEIGHT_FACTOR 0.75

// Used to determine card width given a height.
#define CARD_ASPECT 1.372

// Max rotation speed.
#define MAX_SPEED 10

#define DIR_LEFT -1
#define DIR_RIGHT 1
#define DIR_NONE 0

namespace carousel {

struct CarouselCard {
  int index;
  int y;

  std::string image_filename;
  std::string emu;
  std::string rom;
  bool patience;
};

struct Emulator {
  std::string cmd;
};

bool SortByY(const carousel::CarouselCard& lhs,
             const carousel::CarouselCard& rhs);

class Carousel {
 public:
  int fps;
  int num_slots;
  int initial_speed;
  bool reverse_keys;
  bool click;
  int timeout;
  std::string mixer;
  bool mixer_opened;

  SDL_Texture* background_texture;
  SDL_Texture* screensaver_texture;
  SDL_Texture* volume_texture;
  SDL_Texture* patience_texture;
  std::vector<SDL_Texture*> carousel_image;
  std::vector<SDL_Rect> carousel_pos;

  int width;
  int height;

  int low_index;
  int high_index;

#ifdef ALSA_FOUND
  snd_mixer_t* handle;
  snd_mixer_selem_id_t* sid;
  snd_mixer_elem_t* elem;
#endif

  // Audio
  Uint8* audio_pos;  // global pointer to the audio buffer to be played
  Uint32 audio_len;  // remaining length of the sample we have to play
  Uint32 click_wav_length;
  Uint8* click_wav_buffer;
  Uint32 blip_wav_length;
  Uint8* blip_wav_buffer;
  SDL_AudioSpec click_wav_spec;
  SDL_AudioSpec blip_wav_spec;

  std::map<std::string, Emulator> all_emulators;
  std::vector<CarouselCard> all_cards;

  Carousel();
  ~Carousel();

  // Move all visible carousel cards to their home position + xoffset.
  // Where -width / num_slots < xoffset < width / num_slots
  void SetCarouselPositions(int xoffset);
  bool ParseConfig();
};

}  // namespace carousel

#endif
