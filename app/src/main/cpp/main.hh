//
// Created by ttand on 18-2-11.
//

#ifndef STBOX_MAIN_H
#define STBOX_MAIN_H

#include <unistd.h>
#include <android/log.h>
#include <android_native_app_glue.h>

std::pair<int32_t ,int32_t> AndroidGetWindowSize(android_app *Android_application);
#endif //STBOX_MAIN_H
