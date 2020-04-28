//
// Created by ttand on 20-4-26.
//

#include "aopen.h"
#include "../../../../../../Android/Sdk/ndk-bundle/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include/android/log.h"
#include <errno.h>
#include <android/asset_manager.h>
#include <android/log.h>
static int android_read(void* cookie, char* buf, int size) {
	__android_log_print(ANDROID_LOG_INFO,"TBOX","%s", "Read");
	return AAsset_read((AAsset*)cookie, buf, size);
}

static int android_write(void* cookie, const char* buf, int size) {
	return EACCES; // can't provide write access to the apk
}

static fpos_t android_seek(void* cookie, fpos_t offset, int whence) {
	return AAsset_seek((AAsset*)cookie, offset, whence);
}

static int android_close(void* cookie) {
	AAsset_close((AAsset*)cookie);
	return 0;
}

FILE* aopen(AAsset* asset) {
	if(!asset) return NULL;
	return funopen(asset, android_read, android_write, android_seek, android_close);
}

