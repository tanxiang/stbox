//
// Created by ttand on 19-11-11.
//

#ifndef STBOX_JOBDRAWLINE_HH
#define STBOX_JOBDRAWLINE_HH

#include "util.hh"
#include "Device.hh"
#include "JobBase.hh"
#include <android_native_app_glue.h>

namespace tt {
	struct JobDrawLine : public JobBase{

		static JobDrawLine create(android_app *app, tt::Device &device);

		JobDrawLine(JobBase&& j,android_app *app,tt::Device &device);
	};
}

#endif //STBOX_JOBDRAWLINE_HH
