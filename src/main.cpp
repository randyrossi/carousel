#include <SDL.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "audio.h"
#include "carousel.h"
#include "res_path.h"

int rendering_loop(carousel::Carousel&, SDL_Renderer*);

SDL_Texture* LoadTexture(SDL_Renderer* ren, std::string file) {
  std::string imagePath = carousel::GetResourcePath() + file;
  SDL_Surface* bmp = SDL_LoadBMP(imagePath.c_str());
  if (bmp == NULL) {
    std::cerr << "SDL_LoadBMP Error: " << file << "," << SDL_GetError()
              << std::endl;
    return NULL;
  }

  SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, bmp);
  SDL_FreeSurface(bmp);
  if (tex == NULL) {
    std::cerr << "SDL_CreateTextureFromSurface Error: " << file << ","
              << SDL_GetError() << std::endl;
    return NULL;
  }

  return tex;
}

int main(int, char**) {
  carousel::Carousel carousel;
  if (!carousel.ParseConfig()) {
    std::cerr << "Could not parse config file" << std::endl;
    return 1;
  }

  int sdl_init_mode = SDL_INIT_VIDEO;
  if (carousel.click) {
    sdl_init_mode |= SDL_INIT_AUDIO;
  }

  if (SDL_Init(sdl_init_mode) != 0) {
    std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  SDL_DisplayMode current;
  for (int i = 0; i < SDL_GetNumVideoDisplays(); ++i) {
    int r = SDL_GetCurrentDisplayMode(i, &current);

    if (r != 0) {
      // In case of error...
      std::cerr << "Could not get display mode for video display " << i << ","
                << SDL_GetError();
      return 1;
    } else {
      // On success, print the current display mode.
      carousel.width = current.w;
      carousel.height = current.h;
    }
  }

  if (carousel.width == -1 || carousel.height == -1) {
    std::cerr << "Could not find console resolution" << std::endl;
    return 1;
  }

  carousel::InitSound(carousel);

#ifdef ALSA_FOUND
  const char *card = "default";

  if (carousel.mixer != "None" && carousel.mixer != "none") {
    snd_mixer_open(&carousel.handle, 0);
    snd_mixer_attach(carousel.handle, card);
    snd_mixer_selem_register(carousel.handle, NULL, NULL);
    snd_mixer_load(carousel.handle);

    snd_mixer_selem_id_alloca(&carousel.sid);
    snd_mixer_selem_id_set_index(carousel.sid, 0);
    snd_mixer_selem_id_set_name(carousel.sid, carousel.mixer.c_str());

    carousel.elem = snd_mixer_find_selem(carousel.handle, carousel.sid);
    if (carousel.elem == NULL) {
      std::cerr << "Mixer not found: " << carousel.mixer << ", check config" << std::endl;
      carousel::DestroySound(carousel);
      SDL_Quit();
      return 1;
    }
    carousel.mixer_opened = true;
  }
#endif

  SDL_Window* win = SDL_CreateWindow("Arcade Menu", 0, 0, carousel.width,
                                     carousel.height, SDL_WINDOW_SHOWN);
  if (win == NULL) {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
#ifdef ALSA_FOUND
  if (carousel.mixer_opened) {
    snd_mixer_close(carousel.handle);
  }
#endif
    carousel::DestroySound(carousel);
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* ren = SDL_CreateRenderer(
      win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (ren == NULL) {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
#ifdef ALSA_FOUND
  if (carousel.mixer_opened) {
    snd_mixer_close(carousel.handle);
  }
#endif
    carousel::DestroySound(carousel);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 1;
  }

  carousel.background_texture = LoadTexture(ren, "background.bmp");
  if (carousel.background_texture == NULL) {
#ifdef ALSA_FOUND
  if (carousel.mixer_opened) {
    snd_mixer_close(carousel.handle);
  }
#endif
    carousel::DestroySound(carousel);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 1;
  }

  carousel.screensaver_texture = LoadTexture(ren, "scr_saver.bmp");
  if (carousel.screensaver_texture == NULL) {
#ifdef ALSA_FOUND
  if (carousel.mixer_opened) {
    snd_mixer_close(carousel.handle);
  }
#endif
    SDL_DestroyTexture(carousel.background_texture);
    carousel::DestroySound(carousel);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 1;
  }

  carousel.volume_texture = LoadTexture(ren, "volume.bmp");
  if (carousel.volume_texture == NULL) {
#ifdef ALSA_FOUND
  if (carousel.mixer_opened) {
    snd_mixer_close(carousel.handle);
  }
#endif
    SDL_DestroyTexture(carousel.screensaver_texture);
    SDL_DestroyTexture(carousel.background_texture);
    carousel::DestroySound(carousel);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 1;
  }

  int start_index = 0;
  std::ifstream last_index_file;
  last_index_file.open("/tmp/carousel.idx");
  if (!last_index_file.fail()) {
    last_index_file >> start_index;
  }
  last_index_file.close();

  carousel.low_index = start_index - carousel.num_slots / 2;
  if (carousel.low_index < 0) {
    carousel.low_index += carousel.all_cards.size();
  }
  carousel.high_index = start_index + carousel.num_slots / 2;
  if (carousel.high_index >= (int)carousel.all_cards.size()) {
    carousel.high_index -= (int)carousel.all_cards.size();
  }

  // Load the first carousel cards.
  int card_index = carousel.low_index;
  for (int i = 0; i < carousel.num_slots; i++) {
    carousel.carousel_image[i] =
        LoadTexture(ren, carousel.all_cards.at(card_index).image_filename);
    card_index++;
    if (card_index >= (int)carousel.all_cards.size()) {
      card_index -= carousel.all_cards.size();
    }
  }

  int rc = rendering_loop(carousel, ren);

  // Cleanup
  SDL_DestroyTexture(carousel.background_texture);
  for (int i = 0; i < carousel.num_slots; i++) {
    if (carousel.carousel_image[i] != NULL) {
      SDL_DestroyTexture(carousel.carousel_image[i]);
    }
  }

#ifdef ALSA_FOUND
  if (carousel.mixer_opened) {
    snd_mixer_close(carousel.handle);
  }
#endif
  carousel::DestroySound(carousel);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return rc;
}

bool move_left(carousel::Carousel& carousel, SDL_Renderer* ren) {
  carousel.low_index++;
  if (carousel.low_index >= (int)carousel.all_cards.size()) {
    carousel.low_index = 0;
  }
  carousel.high_index++;
  if (carousel.high_index >= (int)carousel.all_cards.size()) {
    carousel.high_index = 0;
  }

  SDL_DestroyTexture(carousel.carousel_image[0]);
  for (int i = 0; i < carousel.num_slots - 1; i++) {
    carousel.carousel_image[i] = carousel.carousel_image[i + 1];
  }
  carousel.carousel_image[carousel.num_slots - 1] = LoadTexture(
      ren, carousel.all_cards.at(carousel.high_index).image_filename);
  if (carousel.carousel_image[carousel.num_slots - 1] == NULL) {
    return true;
  }
  return false;
}

bool move_right(carousel::Carousel& carousel, SDL_Renderer* ren) {
  carousel.low_index--;
  if (carousel.low_index < 0) {
    carousel.low_index = carousel.all_cards.size() - 1;
  }
  carousel.high_index--;
  if (carousel.high_index < 0) {
    carousel.high_index = carousel.all_cards.size() - 1;
  }

  SDL_DestroyTexture(carousel.carousel_image[carousel.num_slots - 1]);
  for (int i = carousel.num_slots - 1; i >= 1; i--) {
    carousel.carousel_image[i] = carousel.carousel_image[i - 1];
  }
  carousel.carousel_image[0] = LoadTexture(
      ren, carousel.all_cards.at(carousel.low_index).image_filename);
  if (carousel.carousel_image[0] == NULL) {
    return true;
  }
  return false;
}

bool select_game(carousel::Carousel& carousel, bool screensaver) {
  // Don't process select if we are waking up from saver
  if (!screensaver) {
    int selected = std::abs(carousel.low_index + carousel.num_slots / 2) %
                   carousel.all_cards.size();

    std::ofstream last_index_file;
    last_index_file.open("/tmp/carousel.idx", std::ofstream::out);
    if (!last_index_file.fail()) {
      last_index_file << selected;
    }
    last_index_file.close();

    carousel::Emulator emu =
        carousel.all_emulators[carousel.all_cards[selected].emu];
    char cmd[512];
    snprintf(cmd, 512, emu.cmd.c_str(),
             carousel.all_cards[selected].rom.c_str());
    std::cout << cmd << std::endl;
    return true;
  }
  return false;
}

int rendering_loop(carousel::Carousel& carousel, SDL_Renderer* ren) {
  int spin_pos = 0;
  int dir = DIR_NONE;
  int speed = 0;
  int dirty = true;
  bool ended = false;
  int rc = 0;

  bool left_down = false;
  bool right_down = false;
  uint32_t left_down_repeat = 0;
  uint32_t right_down_repeat = 0;

  std::vector<carousel::CarouselCard> render_order;
  uint32_t frame_delay = 1000 / carousel.fps;

  uint32_t last_tick = -1;
  int sp = carousel.width / carousel.num_slots;

  uint32_t next_saver = SDL_GetTicks() + carousel.timeout * 1000;
  bool screensaver = false;

  uint32_t next_volume = SDL_GetTicks();
  bool show_volume = false;


#ifdef ALSA_FOUND
  long min_vol;
  long max_vol;
  long volume = 0;
  long vol_step = 0;
  if (carousel.mixer_opened) {
    snd_mixer_selem_get_playback_volume_range(carousel.elem, &min_vol, &max_vol);
    snd_mixer_selem_get_playback_volume(
        carousel.elem,
        SND_MIXER_SCHN_FRONT_LEFT,
        &volume);
    if (volume > max_vol) { volume = max_vol; }
    if (volume < min_vol) { volume = min_vol; }
    vol_step = (max_vol - min_vol) / 12;
  }
#endif

  while (!ended) {
    // Handle carousel spin.
    if (dir != DIR_NONE) {
      spin_pos = spin_pos +
                 dir * (sp / carousel.fps * (carousel.initial_speed + speed));
      if (spin_pos >= sp || spin_pos <= -sp) {
        if (dir == DIR_LEFT) {
          ended = move_left(carousel, ren);
        } else if (dir == DIR_RIGHT) {
          ended = move_right(carousel, ren);
        }
        spin_pos = 0;
        if (speed > 0) {
          speed--;
        } else {
          dir = DIR_NONE;
        }
        carousel::PlayClick(carousel);
      }
      dirty = true;
    }

    if (dirty) {
      SDL_RenderClear(ren);

      carousel.SetCarouselPositions(spin_pos);

      // For rendering order, sort by their top left y position which
      // should always draw the biggest card last. As the card
      // that's coming to the front reaches the half way
      // point, it's drawn last.
      render_order.clear();

      for (int k = 0; k < carousel.num_slots; k++) {
        carousel::CarouselCard card;
        card.index = k;
        card.y = carousel.carousel_pos[k].y;
        render_order.push_back(card);
      }
      std::sort(render_order.begin(), render_order.end(), carousel::SortByY);

      if (!screensaver) {
        SDL_RenderCopy(ren, carousel.background_texture, NULL, NULL);
        for (size_t i = 0; i < render_order.size(); i++) {
          SDL_RenderCopy(ren, carousel.carousel_image[render_order.at(i).index],
                         NULL,
                         &carousel.carousel_pos[render_order.at(i).index]);
        }
      } else {
        // pick a random location for our screen saver img
        SDL_Rect dest;
        dest.x = std::max(0, std::rand() % carousel.width - 260);
        dest.y = std::max(0, std::rand() % carousel.height - 260);
        dest.w = 260;
        dest.h = 260;
        SDL_RenderCopy(ren, carousel.screensaver_texture, NULL, &dest);
      }

      if (show_volume) {
#ifdef ALSA_FOUND
        SDL_Rect dest;
        for (int i = 0; i < (volume - min_vol) / vol_step; i++) {
          dest.x = i * 40;
          dest.y = 0;
          dest.w = 32;
          dest.h = 32;
          SDL_RenderCopy(ren, carousel.volume_texture, NULL, &dest);
        }
#endif
      }

      // Update the screen
      SDL_RenderPresent(ren);
      dirty = false;
    }

    uint32_t now = SDL_GetTicks();
    uint32_t this_delay = frame_delay - std::min(now - last_tick, frame_delay);

    if (now >= next_saver) {
      next_saver = now + 5000;
      screensaver = true;
      dirty = true;
    }

    if (now >= next_volume) {
      // Cancel volume controls
      show_volume = false;
      dirty = true;
    }

    SDL_Event event;
    SDL_KeyboardEvent* ke = (SDL_KeyboardEvent*)&event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_KEYDOWN:
          if (screensaver) {
            break;
          }
          if (!carousel.reverse_keys) {
            switch (ke->keysym.sym) {
              case SDLK_RIGHT:
              case SDLK_g:
                left_down_repeat = now;
                left_down = true;
                break;
              case SDLK_LEFT:
              case SDLK_d:
                right_down_repeat = now;
                right_down = true;
                break;
              default:
                break;
            }
          } else {
            switch (ke->keysym.sym) {
              case SDLK_LEFT:
              case SDLK_d:
                left_down_repeat = now;
                left_down = true;
                break;
              case SDLK_RIGHT:
              case SDLK_g:
                right_down_repeat = now;
                right_down = true;
                break;
              default:
                break;
            }
          }
          break;
        case SDL_MOUSEBUTTONUP:
          ended = select_game(carousel, screensaver);
          rc = 0;
          break;
        case SDL_KEYUP:
          switch (ke->keysym.sym) {
            case SDLK_ESCAPE:
              ended = true;
              rc = 1;
              break;
            case SDLK_UP:
#ifdef ALSA_FOUND
              if (carousel.mixer_opened) {
                volume+=vol_step; if (volume > max_vol) { volume = max_vol; }
                snd_mixer_selem_set_playback_volume_all(carousel.elem, volume);
                carousel::PlayClick(carousel);
                next_volume = SDL_GetTicks() + 5 * 1000;
                show_volume = true;
                dirty = true;
              }
#endif
              break;
            case SDLK_DOWN:
#ifdef ALSA_FOUND
              if (carousel.mixer_opened) {
                volume-=vol_step; if (volume < min_vol) { volume = min_vol; }
                snd_mixer_selem_set_playback_volume_all(carousel.elem, volume);
                carousel::PlayClick(carousel);
                next_volume = SDL_GetTicks() + 5 * 1000;
                show_volume = true;
                dirty = true;
              }
#endif
              break;
            case SDLK_RETURN:
            // PLayer 1 or 2
            case SDLK_1:
            case SDLK_2:
            // Coin p1 or p2
            case SDLK_5:
            case SDLK_6:
            // P1 fire
            case SDLK_LCTRL:
            // P2 fire
            case SDLK_a:
              ended = select_game(carousel, screensaver);
              rc = 0;
              break;
            default:
              break;
          }

          if (!carousel.reverse_keys) {
            switch (ke->keysym.sym) {
              case SDLK_RIGHT:
              case SDLK_g:
                left_down = false;
                break;
              case SDLK_LEFT:
              case SDLK_d:
                right_down = false;
                break;
              default:
                break;
            }
          } else {
            switch (ke->keysym.sym) {
              case SDLK_LEFT:
              case SDLK_d:
                left_down = false;
                break;
              case SDLK_RIGHT:
              case SDLK_g:
                right_down = false;
                break;
              default:
                break;
            }
          }

          // Not in screen saver any more.
          next_saver = last_tick + carousel.timeout * 1000;
          if (screensaver) {
            dirty = true;
          }
          screensaver = false;

          break;
        default:
          break;
      }
    }

    if (ended) {
      break;
    }

    // If left is down and it's time to repeat, do it now.
    if (left_down && now >= left_down_repeat) {
      if (left_down) {
        left_down_repeat = left_down_repeat + 1000;
      }
      dirty = true;
      if (dir == DIR_LEFT) {
        // Already moving in that dir. Increase speed.
        speed = std::min(speed + 1, MAX_SPEED);
      } else if (dir == DIR_RIGHT) {
        // Opposite direction will slow down active transition to slowest
        // speed and let it finish.
        speed = 0;
      } else {
        dir = DIR_LEFT;
      }
    } else if (right_down && now >= right_down_repeat) {
      if (right_down) {
        right_down_repeat = right_down_repeat + 1000;
      }
      dirty = true;
      if (dir == DIR_RIGHT) {
        // Already moving in that dir. Increase speed.
        speed = std::min(speed + 1, MAX_SPEED);
      } else if (dir == DIR_LEFT) {
        // Opposite direction will slow down active transition to slowest
        // speed and let it finish.
        speed = 0;
      } else {
        dir = DIR_RIGHT;
      }
    }

    SDL_Delay(this_delay);
    last_tick = now;
  }

  return rc;
}
