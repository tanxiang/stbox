//
// Created by ttand on 18-2-11.
//
#include <assert.h>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <chrono>

#include "util.hh"

// Header files.
#include <android/choreographer.h>

#include <cstring>
//#include "shaderc/shaderc.hpp"
#include "main.hh"
#include "stboxvk.hh"
#include "vertexdata.hh"




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

/*
void choreographerCallback(long frameTimeNanos, void* data) {
    assert(data);
    tt::Instance &ttInstance = *reinterpret_cast<tt::Instance *>(data);
    draw_run(ttInstance.defaultDevice(),ttInstance.defaultSurface());
    ttInstance.defaultDevice().swapchainPresent();

    auto laterTime = (std::chrono::steady_clock::now().time_since_epoch().count() - frameTimeNanos )/ 1000000;
    if(laterTime > 12)
        std::cout <<"later"<< laterTime << " minseconds" <<std::endl;
    //draw_run(ttInstance.defaultDevice(), ttInstance.defaultSurface());

    if (ttInstance.isFocus()) {
        AChoreographer_postFrameCallback(AChoreographer_getInstance(), choreographerCallback,
                                         &ttInstance);
    }
}
*/
void Android_handle_cmd(android_app *app, int32_t cmd) {
    static tt::Instance instance = tt::createInstance();
    static tt::stboxvk appbox;
    try {
        switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                // The window is being shown, get it ready.
                std::cout << "APP_CMD_INIT_WINDOW:" << cmd << std::endl;
                appbox.initWindow(app, instance);
                break;

            case APP_CMD_TERM_WINDOW:
                std::cout << "APP_CMD_TERM_WINDOW:" << cmd << std::endl;
                appbox.cleanWindow();
                break;
            case APP_CMD_INPUT_CHANGED:
                break;
            case APP_CMD_START:
                std::cout << "APP_CMD_START:" << cmd << std::endl;
                break;
            case APP_CMD_STOP:
                std::cout << "APP_CMD_STOP:" << cmd << std::endl;
                break;
            case APP_CMD_GAINED_FOCUS:
                std::cout << "APP_CMD_GAINED_FOCUS:" << cmd << std::endl;
                break;
            case APP_CMD_LOST_FOCUS:
                std::cout << "APP_CMD_LOST_FOCUS:" << cmd << std::endl;
                break;
            case APP_CMD_SAVE_STATE:
                std::cout << "APP_CMD_SAVE_STATE:" << cmd << std::endl;
                break;
            case APP_CMD_LOW_MEMORY:
                std::cout << "APP_CMD_LOW_MEMORY:" << cmd << std::endl;
                break;
            case APP_CMD_PAUSE:
                std::cout << "APP_CMD_PAUSE:" << cmd << std::endl;
                break;
            case APP_CMD_RESUME:
                std::cout << "APP_CMD_RESUME:" << cmd << std::endl;
                break;
            case APP_CMD_DESTROY:
                std::cout << "APP_CMD_DESTROY:" << cmd << std::endl;
                break;
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

void Android_process(struct android_app *app) {
    int events;
    android_poll_source *source;
    // Poll all pending events.
    do {
        int ident;
        int minTimeout = -1;
        while (!app->destroyRequested && (ident = ALooper_pollAll(minTimeout, NULL, &events, (void **) &source)) >= 0) {
            // Process each polled events
            if (source != NULL) source->process(app, source);
        }
        if(app->destroyRequested)
            break;
        switch (ident) {
            case ALOOPER_POLL_WAKE:
            case ALOOPER_POLL_TIMEOUT:
                std::cout << "ALOOPER_POLL:\t" << ident << std::endl;
                {

                }
                break;
            case ALOOPER_POLL_ERROR:
            default:
                std::cout << "ALOOPER_ERROR:\t" << ident << std::endl;
        };

    } while (true);
    std::cout << "ALOOPER_EXIT:\t ANativeActivity_finish" << std::endl;
    return ANativeActivity_finish(app->activity);
}


void android_main(struct android_app *app) {
    assert(app != nullptr);
    // Set static variables.
    // Set the callback to process system events

    app->onAppCmd = Android_handle_cmd;
    app->onInputEvent = Android_handle_input;


    // Forward cout/cerr to logcat.
    std::cout.rdbuf(new AndroidBuffer(ANDROID_LOG_INFO));
    std::cerr.rdbuf(new AndroidBuffer(ANDROID_LOG_ERROR));

    // Main loop
    return Android_process(app);
}