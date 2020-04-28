//
// Created by ttand on 20-4-26.
//

#ifndef STBOX_AOPEN_H
#define STBOX_AOPEN_H
#define  __USE_BSD
#include <stdio.h>
#include <android/asset_manager.h>

#ifdef __cplusplus
extern "C" {
#endif


FILE* aopen(AAsset* asset);


#ifdef __cplusplus
}
#endif


#endif //STBOX_AOPEN_H
