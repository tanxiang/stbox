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

std::pair<int32_t, int32_t> AndroidGetWindowSize() {
    // On Android, retrieve the window size from the native window.
    assert(Android_application != nullptr);
    return std::make_pair(ANativeWindow_getWidth(Android_application->window),
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
    tt::Instance &ttInstace = *reinterpret_cast<tt::Instance *>(app->userData);
    try {
        switch (cmd) {
            case APP_CMD_INIT_WINDOW: {
                // The window is being shown, get it ready.
                assert(app->window);
                ttInstace.connectDevice();

                ttInstace.connectWSI(app->window);
                //auto ttDev = ttInstace.connectToDevice();
                draw_run(ttInstace.defaultDevice(), ttInstace.defaultSurface());
                break;
            }
            case APP_CMD_TERM_WINDOW:
                // The window is being hidden or closed, clean it up.
                ttInstace.disconnectWSI();
                break;
            case APP_CMD_DESTROY:
                ttInstace.disconnectDevice();
                break;
            case APP_CMD_PAUSE:
            case APP_CMD_RESUME:
            case APP_CMD_SAVE_STATE:
            case APP_CMD_START:
            case APP_CMD_STOP:
            case APP_CMD_GAINED_FOCUS:
            case APP_CMD_LOST_FOCUS:
            default:
                std::cout << "event not handled:" << cmd << std::endl;
        }
    }
    catch (std::system_error systemError) {
        std::cout << "got system error:" << systemError.what() << "!#" << systemError.code()
                  << std::endl;
    }
    catch (std::logic_error logicError) {
        std::cout << "got logic error:" << logicError.what() << std::endl;
    }
}

int Android_handle_input(struct android_app *app, AInputEvent *event) {
    tt::Instance &ttInstace = *reinterpret_cast<tt::Instance *>(app->userData);

    //todo check window instance device state
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int32_t eventSource = AInputEvent_getSource(event);
        switch (eventSource) {
            case AINPUT_SOURCE_JOYSTICK: {
                // Left thumbstick
                break;
            }

            case AINPUT_SOURCE_TOUCHSCREEN: {
                int32_t action = AMotionEvent_getAction(event);

                switch (action) {
                    case AMOTION_EVENT_ACTION_UP: {

                        return 1;
                    }
                    case AMOTION_EVENT_ACTION_DOWN: {
                        // Detect double tap
                        break;
                    }
                    case AMOTION_EVENT_ACTION_MOVE: {
                        bool handled = false;

                        break;
                    }
                    default:
                        return 1;
                }
            }

            default:
                return false;
        }
    }
    return false;
}

int Android_process(struct android_app *app) {
    int events;
    android_poll_source *source;
    // Poll all pending events.
    if (ALooper_pollAll(0, NULL, &events, (void **) &source) >= 0) {
        // Process each polled events
        if (source != NULL) source->process(app, source);
    }
    return app->destroyRequested;
}


void android_main(struct android_app *app) {
    assert(app != nullptr);
    // Set static variables.
    Android_application = app;
    // Set the callback to process system events

    app->onAppCmd = Android_handle_cmd;
    app->onInputEvent = Android_handle_input;
    auto ttInstance = tt::createInstance();

    app->userData = &ttInstance;

    // Forward cout/cerr to logcat.
    std::cout.rdbuf(new AndroidBuffer(ANDROID_LOG_INFO));
    std::cerr.rdbuf(new AndroidBuffer(ANDROID_LOG_ERROR));

    // Main loop
    do {
        Android_process(app);
    }  // Check if system requested to quit the application
    while (app->destroyRequested == 0);

    return;
}