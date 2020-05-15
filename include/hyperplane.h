#pragma once

#include "glm.hpp"

namespace four
{

	/// A 4-dimensional hyperplane, specified in Hessian normal form:
	///
	///     n.dot(x) = -d
	///
	/// where `d` is the distance of the hyperplane from the origin. Here, the sign
	/// of `d` determines the side of the plane on which the origin is located. If
	/// `d` is positive, it is in the half-space determined by the direction of `n`.
	/// Otherwise, it is in the other half-space.
	///
	/// Reference: `https://mathworld.wolfram.com/HessianNormalForm.html`
	class Hyperplane
	{
		
	public:

		Hyperplane(const glm::vec4& normal, float displacement) :
			normal{ glm::normalize(normal) },
			displacement{ displacement }
		{
		}

		/// Returns `true` if `point` is "inside" the hyperplane (within some epsilon) and `false` otherwise.
		bool inside(const glm::vec4& point, float epsilon = 0.001f) const
		{
			return abs(signed_distance(point)) <= epsilon;
		}

		/// Returns the signed distance (in 4-space) of `point` to the hyperplane.
		///
		/// Reference: `https://mathworld.wolfram.com/Point-PlaneDistance.html`
		float signed_distance(const glm::vec4& point) const
		{
			return glm::dot(normal, point) + displacement;
		}

		const glm::vec4& get_normal() const
		{
			return normal;
		}

		float get_displacement() const 
		{
			return displacement;
		}

	//private:

		glm::vec4 normal;
		float displacement;

	};
	
}