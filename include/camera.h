#pragma once

#include "glm.hpp"

#include "maths.h"

namespace four
{

	class Camera
	{ 
		
	public:

		Camera(const glm::vec4& from, const glm::vec4& to, const glm::vec4& up, const glm::vec4& over, float fov_degrees = 45.0f) :
			from{ from },
			to{ to },
			up{ up },
			over{ over },
			fov{ fov_degrees }
		{}

		glm::mat4 look_at() const
		{
			// Construct basis using 4-dimensional cross products
			auto wd = glm::normalize(to - from);
			auto wa = glm::normalize(maths::cross(up, over, wd));
			auto wb = glm::normalize(maths::cross(over, wd, wa));
			auto wc = maths::cross(wd, wa, wb);

			// Set columns of look-at matrix
			glm::mat4 matrix = glm::mat4{ 1.0f };
			matrix[0] = wa;
			matrix[1] = wb;
			matrix[2] = wc;
			matrix[3] = wd;

			return matrix;
		}

		glm::mat4 projection() const
		{
			float t = 1.0f / tanf(glm::radians(fov) * 0.5f);

			// `t` along the diagonal
			return glm::mat4{ t };
		}

		const glm::vec4& get_from() const
		{
			return from;
		}

		const glm::vec4& get_to() const
		{
			return to;
		}

		const glm::vec4& get_up() const
		{
			return up;
		}

		const glm::vec4& get_over() const
		{
			return over;
		}

	private:

		glm::vec4 from;
		glm::vec4 to;
		glm::vec4 up;
		glm::vec4 over;
		float fov;

	};

}