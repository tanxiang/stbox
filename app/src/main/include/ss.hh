//
// Created by ttand on 19-1-8.
//

#ifndef STBOX_SS_HH
#define STBOX_SS_HH

#include <sstream>
#include <android/log.h>

namespace tt{
    class ss
    {
    private:
        std::ostringstream m_ss;
        const int m_logLevel;
    public:

        ss(int Xi_logLevel):m_logLevel(Xi_logLevel)
        {
        };

        ~ss()
        {
            __android_log_print(m_logLevel,"TBOX","%s", m_ss.str().c_str());
        }

        template<typename T> ss& operator<<(T const& Xi_val)
        {
            m_ss << Xi_val;
            return *this;
        }
    };

}

#define MY_LOG(LOG_LEVEL) tt::ss(ANDROID_LOG_##LOG_LEVEL) << __FILE__ << '#' << __LINE__ << " : "

#endif //STBOX_SS_HH
