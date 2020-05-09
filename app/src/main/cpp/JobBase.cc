//
// Created by ttand on 20-4-20.
//

#include "JobBase.hh"

namespace tt {
	glm::mat4 JobBase::perspective;
	glm::mat4 JobBase::lookat = glm::lookAt(
			glm::vec3{0, 0, -4},// Camera is at (-5,3,-10), in World Space
			glm::vec3{0, 0, 0},     // and looks at the origin
			glm::vec3{0, -1, 0}     /* Head is up (set to 0,-1,0 to look upside-down)*/ );

}