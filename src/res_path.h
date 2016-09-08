#ifndef RES_PATH_H
#define RES_PATH_H

#include <string>

namespace carousel {

/*
 * Get the resource path for resources located in res/subDir
 */
std::string GetResourcePath(const std::string &subDir = "");

} // namespace carousel

#endif
