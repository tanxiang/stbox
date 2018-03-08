//
// Created by ttand on 18-2-11.
//

#ifndef STBOX_MAIN_H
#define STBOX_MAIN_H
#include <unistd.h>
#include <android/log.h>
std::pair<int32_t ,int32_t> AndroidGetWindowSize();
ANativeWindow *AndroidGetApplicationWindow();
#endif //STBOX_MAIN_H
