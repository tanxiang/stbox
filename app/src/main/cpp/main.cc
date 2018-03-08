//
// Created by ttand on 18-2-11.
//
#include <assert.h>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "util.hh"

// Header files.
#include <android_native_app_glue.h>
#include <cstring>
#include "shaderc/shaderc.hpp"
#include "main.hh"
#include "stboxvk.hh"
static android_app *Android_application = nullptr;
//#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "STBOX", __VA_ARGS__))

std::pair<int32_t ,int32_t> AndroidGetWindowSize() {
    // On Android, retrieve the window size from the native window.
    assert(Android_application != nullptr);
    return std::make_pair( ANativeWindow_getWidth(Android_application->window),
                           ANativeWindow_getHeight(Android_application->window));
}


ANativeWindow *AndroidGetApplicationWindow() {
    assert(Android_application != nullptr);
    return Android_application->window;
}

class AndroidBuffer : public std::streambuf {
public:
    AndroidBuffer(android_LogPriority priority) {
        priority_ = priority;
        this->setp(buffer_, buffer_ + kBufferSize - 1);
    }

private:
    static const int32_t kBufferSize = 128;
    int32_t overflow(int32_t c) {
        if (c == traits_type::eof()) {
            *this->pptr() = traits_type::to_char_type(c);
            this->sbumpc();
        }
        return this->sync() ? traits_type::eof() : traits_type::not_eof(c);
    }

    int32_t sync() {
        int32_t rc = 0;
        if (this->pbase() != this->pptr()) {
            char writebuf[kBufferSize + 1];
            memcpy(writebuf, this->pbase(), this->pptr() - this->pbase());
            writebuf[this->pptr() - this->pbase()] = '\0';

            rc = __android_log_write(priority_, "STBOX", writebuf) > 0;
            this->setp(buffer_, buffer_ + kBufferSize - 1);
        }
        return rc;
    }

    android_LogPriority priority_ = ANDROID_LOG_INFO;
    char buffer_[kBufferSize];
};

void Android_handle_cmd(android_app *app, int32_t cmd) {
    try {
        switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                // The window is being shown, get it ready.
                st_main_test(0, nullptr);
                std::cout<<"================================================="<<std::endl;
                std::cout<<"          APP_CMD_INIT_WINDOW"<<std::endl;
                std::cout<<"================================================="<<std::endl;
                break;
            case APP_CMD_TERM_WINDOW:
                // The window is being hidden or closed, clean it up.
                break;
            default:
                std::cout<<"event not handled:"<<cmd<<std::endl;
        }
    }
    catch (std::system_error systemError){
        std::cout<<"got system error:"<<systemError.what()<<"!#"<<systemError.code()<<std::endl;
    }
    catch (std::logic_error logicError){
        std::cout<<"got logic e:"<<logicError.what()<<std::endl;
    }
}

bool Android_process_command() {
    assert(Android_application != nullptr);
    int events;
    android_poll_source *source;
    // Poll all pending events.
    if (ALooper_pollAll(0, NULL, &events, (void **)&source) >= 0) {
        // Process each polled events
        if (source != NULL) source->process(Android_application, source);
    }
    return Android_application->destroyRequested;
}

void android_main(struct android_app *app) {
    // Set static variables.
    Android_application = app;
    // Set the callback to process system events
    app->onAppCmd = Android_handle_cmd;

    // Forward cout/cerr to logcat.
    std::cout.rdbuf(new AndroidBuffer(ANDROID_LOG_INFO));
    std::cerr.rdbuf(new AndroidBuffer(ANDROID_LOG_ERROR));

    // Main loop
    do {
        Android_process_command();
    }  // Check if system requested to quit the application
    while (app->destroyRequested == 0);

    return;
}