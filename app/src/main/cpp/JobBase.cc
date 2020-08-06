//
// Created by ttand on 20-4-20.
//

//#include <glm/gtx/euler_angles.hpp>
#include "JobBase.hh"

namespace tt {
	glm::mat4 JobBase::perspective;
	glm::mat4 JobBase::lookat = glm::lookAt(
			glm::vec3{0, 0, -4},// Camera is at (-5,3,-10), in World Space
			glm::vec3{0, 0, 0},     // and looks at the origin
			glm::vec3{0, 1, 0}  );
	glm::qua<float> JobBase::fRotate;


	void JobBase::setRotate(float dx, float dy) {
		static float datx=0.0,daty=0.0;
		datx-=dx*0.01;
		daty-=dy*0.01;
		glm::vec3 eulerAngle{-daty, datx, 0.0};

		auto c = glm::cos(eulerAngle * 0.5f);
		auto s = glm::sin(eulerAngle * 0.5f);

		fRotate = glm::qua<float>{ //Y - X - Z rotate
				c.x * c.y * c.z - s.x * s.y * s.z,
				s.x * c.y * c.z - c.x * s.y * s.z,
				c.x * s.y * c.z + s.x * c.y * s.z,
				c.x * c.y * s.z + s.x * s.y * c.z
		};
	}

}