#include "res_path.h"

#include <SDL.h>
#include <iostream>

namespace carousel {

std::string GetResourcePath(const std::string &subDir) {
#ifdef _WIN32
  const char PATH_SEP = '\\';
#else
  const char PATH_SEP = '/';
#endif
  static std::string baseRes;
  if (baseRes.empty()) {
    // SDL_GetBasePath will return NULL if something went wrong
    char *basePath = SDL_GetBasePath();
    if (basePath) {
      baseRes = basePath;
      SDL_free(basePath);
    } else {
      std::cerr << "Error getting resource path: " << SDL_GetError()
                << std::endl;
      return "";
    }
    // We replace the last bin/ with res/ to get the the resource path
    size_t pos = baseRes.rfind("bin");
    baseRes = baseRes.substr(0, pos) + "res" + PATH_SEP;
  }
  // If we want a specific subdirectory path in the resource directory
  // append it to the base path.
  return subDir.empty() ? baseRes : baseRes + subDir + PATH_SEP;
}

} // namespace carousel
