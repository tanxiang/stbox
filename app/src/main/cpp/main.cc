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

// Header files.
#include <android/choreographer.h>

#include <cstring>
//#include "shaderc/shaderc.hpp"

#include "util.hh"
#include "Instance.hh"
#include "main.hh"
#include "JobFont.hh"
#include "JobDraw.hh"
#include "Device.hh"
#include "stboxvk.hh"
#include "vertexdata.hh"

//#include <<glm/gtx/rotate_vector.hpp>

/*
void choreographerCallback(long frameTimeNanos, void* data) {
    assert(data);
    tt::Instance &ttInstance = *reinterpret_cast<tt::Instance *>(data);
    draw_run(ttInstance.defaultDevice(),ttInstance.defaultSurface());
    ttInstance.defaultDevice().swapchainPresent();

    auto laterTime = (std::chrono::steady_clock::now().time_since_epoch().count() - frameTimeNanos )/ 1000000;
    if(laterTime > 12)
        MY_LOG(INFO) <<"later"<< laterTime << " minseconds" ;
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
                MY_LOG(INFO) << "APP_CMD_INIT_WINDOW:" << cmd ;
                appbox.initWindow(app, instance);
		        app->userData = &appbox;
		        appbox.draw();
		        break;

            case APP_CMD_TERM_WINDOW:
                MY_LOG(INFO) << "APP_CMD_TERM_WINDOW:" << cmd ;
                appbox.cleanWindow();
		        app->userData = nullptr;
                break;
            case APP_CMD_INPUT_CHANGED:
                break;
            case APP_CMD_START:
                MY_LOG(INFO) << "APP_CMD_START:" << cmd ;
                break;
            case APP_CMD_STOP:
                MY_LOG(INFO) << "APP_CMD_STOP:" << cmd ;
                break;
            case APP_CMD_GAINED_FOCUS:
                MY_LOG(INFO) << "APP_CMD_GAINED_FOCUS:" << cmd ;
                break;
            case APP_CMD_LOST_FOCUS:
                MY_LOG(INFO) << "APP_CMD_LOST_FOCUS:" << cmd ;
                break;
            case APP_CMD_SAVE_STATE:
                MY_LOG(INFO) << "APP_CMD_SAVE_STATE:" << cmd ;
                break;
            case APP_CMD_LOW_MEMORY:
                MY_LOG(INFO) << "APP_CMD_LOW_MEMORY:" << cmd ;
                break;
            case APP_CMD_PAUSE:
                MY_LOG(INFO) << "APP_CMD_PAUSE:" << cmd ;
                break;
            case APP_CMD_RESUME:
                MY_LOG(INFO) << "APP_CMD_RESUME:" << cmd ;
                break;
            case APP_CMD_DESTROY:
                MY_LOG(INFO) << "APP_CMD_DESTROY:" << cmd ;
                break;
            default:
                MY_LOG(INFO) << "event not handled:" << cmd ;
        }
    }
    catch (std::runtime_error runtimeError) {
        MY_LOG(ERROR) << "got system error:" << runtimeError.what() << "!#"  ;
        JNIEnv* jni;
        app->activity->vm->AttachCurrentThread(&jni, NULL);
        jstring jmessage = jni->NewStringUTF(runtimeError.what());
        jclass clazz = jni->GetObjectClass(app->activity->clazz);
        // Signature has to match java implementation (arguments)
        jmethodID methodID = jni->GetMethodID(clazz, "showAlert", "(Ljava/lang/String;)V");
        jni->CallVoidMethod(app->activity->clazz, methodID, jmessage);
        jni->DeleteLocalRef(jmessage);
        app->activity->vm->DetachCurrentThread();
    }
    /*
    catch (std::system_error systemError) {
        MY_LOG(ERROR) << "got system error:" << systemError.what() << "!#" << systemError.code() ;
    }
    catch (std::logic_error logicError) {
        MY_LOG(ERROR) << "got logic error:" << logicError.what() ;
    }*/
}

int Android_handle_input(struct android_app *app, AInputEvent *event) {

    static int64_t lastTapTime;
	tt::stboxvk & appbox = *static_cast<tt::stboxvk*>(app->userData);
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
                auto thisTapTime = AMotionEvent_getEventTime(event);
                static float xOrg;
                static float yOrg;
                float x = AMotionEvent_getX(event, 0);
                float y = AMotionEvent_getY(event, 0);
                switch (action) {
                    case AMOTION_EVENT_ACTION_UP: {
                        auto downTapTime = AMotionEvent_getDownTime(event);
                        if(thisTapTime - downTapTime < 180 * 1000000){
                            MY_LOG(INFO) << "Tap event" << thisTapTime<<" x "<<x<<" y "<<y;
                        }
                        lastTapTime = thisTapTime;
	                    break;
                    }
                    case AMOTION_EVENT_ACTION_DOWN: {
                        // Detect double tap
                        break;
                    }
                    case AMOTION_EVENT_ACTION_MOVE: {
                    	//
                    	auto dtX = x - xOrg;
                    	auto dtY = y - yOrg;
	                    appbox.draw(dtX,dtY);
                        break;
                    }
                    default:
                        ;

                }
	            xOrg = x;
	            yOrg = y;
	            return 1;
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
        while (!app->destroyRequested && (ident = ALooper_pollAll(minTimeout, nullptr, &events, (void **) &source)) >= 0) {
            // Process each polled events
            if (source != nullptr) source->process(app, source);
        }
        if(app->destroyRequested)
            break;
        switch (ident) {
            case ALOOPER_POLL_WAKE:
            case ALOOPER_POLL_TIMEOUT:
                MY_LOG(INFO) << "ALOOPER_POLL:\t" << ident ;
                {

                }
                break;
            case ALOOPER_POLL_ERROR:
            default:
                MY_LOG(INFO) << "ALOOPER_ERROR:\t" << ident ;
        };

    } while (true);
    MY_LOG(INFO) << "ALOOPER_EXIT:\t ANativeActivity_finish" ;
    return ANativeActivity_finish(app->activity);
}

/*
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
*/
void android_main(struct android_app *app) {
    assert(app != nullptr);
    // Set static variables.
    // Set the callback to process system events

    app->onAppCmd = Android_handle_cmd;
    app->onInputEvent = Android_handle_input;


    // Forward cout/cerr to logcat.
    //std::cout.rdbuf(new AndroidBuffer(ANDROID_LOG_INFO));
    //std::cerr.rdbuf(new AndroidBuffer(ANDROID_LOG_ERROR));

    // Main loop
    return Android_process(app);
}