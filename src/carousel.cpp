#include "carousel.h"

#include <iostream>
#include <libconfig.h++>

namespace carousel {

// Custom comparator for sorting CarouselCard records.
bool SortByY(const carousel::CarouselCard& lhs,
             const carousel::CarouselCard& rhs) {
  return lhs.y > rhs.y;
}

Carousel::Carousel()
    : fps(30),
      num_slots(5),
      initial_speed(2),
      reverse_keys(false),
      click(true),
      timeout(1800),
      mixer("PCM"),
      mixer_opened(false),
      background_texture(NULL),
      screensaver_texture(NULL),
      volume_texture(NULL),
      patience_texture(NULL),
      width(-1),
      height(-1),
      low_index(0),
      high_index(num_slots - 1),
#ifdef ALSA_FOUND
      handle(NULL),
      sid(NULL),
      elem(NULL),
#endif
      audio_pos(NULL),
      audio_len(0),
      click_wav_length(0),
      click_wav_buffer(NULL),
      blip_wav_length(0),
      blip_wav_buffer(NULL) {
  carousel_image.resize(num_slots);
  carousel_pos.resize(num_slots);

  for (int i = 0; i < num_slots; i++) {
    carousel_image[i] = NULL;
  }
}

Carousel::~Carousel() {}

void Carousel::SetCarouselPositions(int xoffset) {
  // Largest card is the one in the middle (representing the current selection)
  int largest_h = (int)((double)height * HOME_HEIGHT_FACTOR);
  int largest_w = (int)((double)largest_h / CARD_ASPECT);

  // sp is the space between cards (by center x/y position)
  int sp = width / num_slots;
  SDL_assert(abs(xoffset) <= sp);

  for (int i = 0; i < num_slots; i++) {
    // X positions are simply index (i) * spacing + provided xoffset
    int sx = sp * i + xoffset;
    // 180 degrees is divided evenly by slots - 1
    double angle = (double)(i)*180 / (num_slots - 1);
    // Calc what angle adjustment we need based on xoffset
    double angle_adjust =
        (double)xoffset / (double)sp * ((180.0 / (num_slots - 1)));
    // Size factor (sf) is sin(angle + angle_adjust) so we go from 0 to 1.0 back
    // to 0 across all
    // slots.
    double sf = sin((angle + angle_adjust) / 57.29);
    // cx,cy are center positions for the cards, we center our x's horizontally
    // since they started
    // on the very left edge of the screen
    int cx = sx + sp / 2;
    int cy = height / 2;
    carousel_pos[i].w = largest_w * sf;
    carousel_pos[i].h = largest_h * sf;
    carousel_pos[i].x = cx - carousel_pos[i].w / 2;
    carousel_pos[i].y = cy - carousel_pos[i].h / 2;
  }
}

bool Carousel::ParseConfig() {
  libconfig::Config cfg;

  // Read the file. If there is an error, report it and exit.
  try {
    cfg.readFile("carousel.cfg");
  } catch (const libconfig::FileIOException& fioex) {
    std::cerr << "I/O error while reading file." << std::endl;
    return false;
  } catch (const libconfig::ParseException& pex) {
    std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
              << " - " << pex.getError() << std::endl;
    return false;
  }

  // fps
  try {
    int cfg_fps = cfg.lookup("fps");
    if (cfg_fps < 12 || cfg_fps > 48) {
      std::cerr << "Ignoring out of range fps " << cfg_fps << std::endl;
    } else {
      fps = cfg_fps;
    }
  } catch (const libconfig::SettingNotFoundException& nfex) {
    // ignore
  }

  // numslots
  try {
    int cfg_numslots = cfg.lookup("numslots");
    if (cfg_numslots < 3 || cfg_numslots > 9) {
      std::cerr << "Ignoring out of range numslots " << cfg_numslots
                << std::endl;
    } else {
      // Always add 2 for the invisible end cards.
      num_slots = cfg_numslots + 2;
    }
  } catch (const libconfig::SettingNotFoundException& nfex) {
    // ignore
  }

  // Number of slots must be odd.
  if (num_slots % 2 == 0) {
    std::cerr << "Number of carousel slots must be odd" << std::endl;
    return false;
  }

  high_index = num_slots - 1;
  carousel_pos.resize(num_slots);
  carousel_image.resize(num_slots);
  for (int i = 0; i < num_slots; i++) {
    carousel_image[i] = NULL;
  }

  // speed
  try {
    int cfg_speed = cfg.lookup("speed");
    if (cfg_speed < 1 || cfg_speed > 3) {
      std::cerr << "Ignoring out of range speed " << cfg_speed << std::endl;
    } else {
      initial_speed = cfg_speed;
    }
  } catch (const libconfig::SettingNotFoundException& nfex) {
    // ignore
  }

  // reverse
  try {
    reverse_keys = cfg.lookup("reverse_keys");
  } catch (const libconfig::SettingNotFoundException& nfex) {
    // ignore
  }

  // click
  try {
    click = cfg.lookup("click");
  } catch (const libconfig::SettingNotFoundException& nfex) {
    // ignore
  }

  // timeout
  try {
    int cfg_timeout = cfg.lookup("timeout");
    if (cfg_timeout < 1) {
      std::cerr << "Ignoring bad timeout " << cfg_timeout << std::endl;
    } else {
      timeout = cfg_timeout;
    }
  } catch (const libconfig::SettingNotFoundException& nfex) {
    // ignore
  }

  // mixer
  try {
    cfg.lookupValue("mixer", mixer);
  } catch (const libconfig::SettingNotFoundException& nfex) {
    // ignore
  }

  const libconfig::Setting& root = cfg.getRoot();

  // Register emulators.
  // Add cards defined by config.
  try {
    Emulator carousel_emulator;
    const libconfig::Setting& emulators = root["emulators"];
    int count = emulators.getLength();

    for (int i = 0; i < count; ++i) {
      const libconfig::Setting& emulator = emulators[i];

      std::string name, cmd;

      if (!(emulator.lookupValue("name", name) &&
            emulator.lookupValue("cmd", cmd))) {
        std::cerr << "Config file contains invalid emulator entry :" << i
                  << std::endl;
        return false;
      }

      carousel_emulator.cmd = cmd;

      all_emulators[name] = carousel_emulator;
    }
  } catch (const libconfig::SettingNotFoundException& nfex) {
    std::cerr << "Config file is missing cards definition" << std::endl;
    return false;
  }

  root_genre.name = "root";
  root_genre.image_filename = "";
  all_genres["root"] = root_genre;
  all_genre_names.push_back("root");

  // Read genres
  try {
    Genre carousel_genre;
    const libconfig::Setting& genres = root["genres"];
    int count = genres.getLength();

    for (int i = 0; i < count; ++i) {
      const libconfig::Setting& genre = genres[i];

      std::string name, img;
      if (!(genre.lookupValue("name", name) &&
            genre.lookupValue("image", img))) {
        std::cerr << "Config file contains invalid genre entry :" << i
                  << std::endl;
        return false;
      }

      carousel_genre.name = name;
      carousel_genre.image_filename = img;

      all_genres[name] = carousel_genre;


      all_genre_names.push_back(name);

      CarouselCard carousel_card;
      carousel_card.image_filename = img;
      carousel_card.genre = name;
      carousel_card.back = false;
      all_genres["root"].all_cards.push_back(carousel_card);
    }
  } catch (const libconfig::SettingNotFoundException& nfex) {
    std::cerr << "Config file is missing cards definition" << std::endl;
    return false;
  }

  // Add cards defined by config.
  try {
    CarouselCard carousel_card;
    const libconfig::Setting& cards = root["cards"];
    int count = cards.getLength();

    for (int i = 0; i < count; ++i) {
      const libconfig::Setting& card = cards[i];

      // Only output the record if all of the expected fields are present.
      std::string image, emu, rom, genre;
      bool patience = false;

      if (!(card.lookupValue("image", image) && card.lookupValue("emu", emu) &&
            card.lookupValue("rom", rom) && card.lookupValue("genre", genre))) {
        std::cerr << "Config file contains invalid card index " << i
                  << std::endl;
        return false;
      }

      // patience
      card.lookupValue("patience", patience);

      if (all_emulators.find(emu) == all_emulators.end()) {
        std::cerr << "Unknown emulator " << emu << " for card index " << i
                  << std::endl;
        return false;
      }

      if (all_genres.find(genre) == all_genres.end()) {
        std::cerr << "Unknown genre " << genre << " for card index " << i
                  << std::endl;
        return false;
      }

      carousel_card.image_filename = image;
      carousel_card.emu = emu;
      carousel_card.rom = rom;
      carousel_card.genre = genre;
      carousel_card.patience = patience;
      carousel_card.back = false;
      carousel_card.index = all_genres[genre].all_cards.size();

      all_genres[genre].all_cards.push_back(carousel_card);
    }
  } catch (const libconfig::SettingNotFoundException& nfex) {
    std::cerr << "Config file is missing cards definition" << std::endl;
    return false;
  }


  // Duplicate members of the each list until we have at least num_slots
  // entries.

  for (unsigned int i = 0; i < all_genre_names.size(); ++i) {
     std::string gn = all_genre_names.at(i);

     if (all_genres[gn].all_cards.empty()) {
       std::cerr << "No carousel cards configured for " << gn << std::endl;
       return false;
     }

     int repeat = 0;
     while ((int)all_genres[gn].all_cards.size() < num_slots) {
       carousel::CarouselCard card = all_genres[gn].all_cards.at(repeat);
       card.index = all_genres[gn].all_cards.size();
       all_genres[gn].all_cards.push_back(card);
       repeat++;
     }

     // Append a back card to each genre except root
     if (gn != "root") {
       CarouselCard back_card;
       back_card.image_filename = "back.bmp";
       back_card.genre = "";
       back_card.back = true;
       all_genres[gn].all_cards.push_back(back_card);
     }

  }

  return true;
}

}  // namespace carousel
