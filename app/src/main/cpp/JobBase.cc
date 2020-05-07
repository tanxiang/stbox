//
// Created by ttand on 20-4-20.
//

#include "JobBase.hh"

namespace tt{
	glm::mat4 JobBase::perspective;
	glm::vec3 JobBase::camPos{0, 0, -4};
	glm::vec3 JobBase::camTo{0, 0, 0};
	glm::vec3 JobBase::camUp{0,-1,0};
	glm::mat4 JobBase::lookat = glm::lookAt(
			camPos,  // Camera is at (-5,3,-10), in World Space
			camTo,     // and looks at the origin
			camUp     // Head is up (set to 0,-1,0 to look upside-down)
	);

}