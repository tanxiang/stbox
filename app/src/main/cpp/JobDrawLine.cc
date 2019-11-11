//
// Created by ttand on 19-11-11.
//

#include "JobDrawLine.hh"

namespace tt {

	JobDrawLine::JobDrawLine(JobBase &&j, android_app *app, tt::Device &device) : JobBase{
			std::move(j)} {

	}

	JobDrawLine JobDrawLine::create(android_app *app, tt::Device &device) {
		return JobDrawLine(
				device.createJob(
						{},
						{}
				),
				app,
				device
		);
	}
}