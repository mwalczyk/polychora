#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "hyperplane.h"
#include "utils.h"

namespace four
{
	
	namespace maths
	{

		/// An enumeration representing a plane of rotation in 4D space.
		enum class Plane
		{
			XY,
			YZ,
			ZX,
			XW,
			YW,
			ZW
		};

        /// 4-dimensional rotations are best thought about as rotations parallel to a plane.
        /// For any of the six rotations below, only two coordinates change. In the future,
        /// it might be interesting to explore the 4D equivalent of quaternions: rotors.
        ///
        /// Reference: `https://math.stackexchange.com/questions/1402362/rotation-in-4d` and `http://hollasch.github.io/ray4/Four-Space_Visualization_of_4D_Objects.html#rotmats`
        glm::mat4 get_simple_rotation_matrix(Plane plane, float angle)
        {
            float c = cosf(angle);
            float s = sinf(angle);

            switch (plane)
            {
            case Plane::XY:
                return glm::mat4(
                    glm::vec4(c, -s, 0.0, 0.0),
                    glm::vec4(s, c, 0.0, 0.0),
                    glm::vec4(0.0, 0.0, 1.0, 0.0),
                    glm::vec4(0.0, 0.0, 0.0, 1.0)
                 );
            case Plane::YZ:
                return glm::mat4(
                    glm::vec4(1.0, 0.0, 0.0, 0.0),
                    glm::vec4(0.0, c, -s, 0.0),
                    glm::vec4(0.0, s, c, 0.0),
                    glm::vec4(0.0, 0.0, 0.0, 1.0)
                 );
            case Plane::ZX:
                return glm::mat4(
                    glm::vec4(c, 0.0, s, 0.0),
                    glm::vec4(0.0, 1.0, 0.0, 0.0),
                    glm::vec4(-s, 0.0, c, 0.0),
                    glm::vec4(0.0, 0.0, 0.0, 1.0)
                );
            case Plane::XW:
                return glm::mat4(
                    glm::vec4(c, 0.0, 0.0, -s),
                    glm::vec4(0.0, 1.0, 0.0, 0.0),
                    glm::vec4(0.0, 0.0, 1.0, 0.0),
                    glm::vec4(s, 0.0, 0.0, c)
                );
            case Plane::YW:
                return glm::mat4(
                    glm::vec4(1.0, 0.0, 0.0, 0.0),
                    glm::vec4(0.0, c, 0.0, s),
                    glm::vec4(0.0, 0.0, 1.0, 0.0),
                    glm::vec4(0.0, -s, 0.0, c)
                );
            case Plane::ZW:
                return glm::mat4(
                    glm::vec4(1.0, 0.0, 0.0, 0.0),
                    glm::vec4(0.0, 1.0, 0.0, 0.0),
                    glm::vec4(0.0, 0.0, c, s),
                    glm::vec4(0.0, 0.0, -s, c)
                );
            }
        }

        /// Returns a "double rotation" matrix, which represents two planes of rotation.
        /// The only fixed point is the origin. These are also known as Clifford rotations.
        /// Clifford rotations can be decomposed into two independent, simultaneous plane
        /// rotations (each of which can have a different "rate" of rotation, i.e. `alpha`
        /// and `beta`).
        ///
        /// If `alpha` and `beta` are equal and non-zero, then the rotation is called an
        /// isoclinic rotation.
        ///
        /// This function accepts the first plane of rotation as an argument. The second
        /// plane of rotation is determined by the first (the pair of 2-planes are orthogonal
        /// to one another). The resulting matrix represents a rotation by `alpha` about the
        /// first plane and a rotation of `beta` about the second plane.
        ///
        /// Reference: `https://en.wikipedia.org/wiki/Plane_of_rotation#Double_rotations`
        glm::mat4 get_double_rotation_matrix(Plane first_plane, float alpha, float beta)
        {
            switch (first_plane)
            {
            // α-XY, β-ZW
            case Plane::XY: return get_simple_rotation_matrix(Plane::XY, alpha) * get_simple_rotation_matrix(Plane::ZW, beta);

            // α-YZ, β-XW
            case Plane::YZ: return get_simple_rotation_matrix(Plane::YZ, alpha) * get_simple_rotation_matrix(Plane::XW, beta);

            // α-ZX, β-YW
            case Plane::ZX: return get_simple_rotation_matrix(Plane::ZX, alpha) * get_simple_rotation_matrix(Plane::YW, beta);

            // α-XW, β-YZ
            case Plane::XW: return get_simple_rotation_matrix(Plane::XW, alpha) * get_simple_rotation_matrix(Plane::YZ, beta);

            // α-YW, β-ZX
            case Plane::YW: return get_simple_rotation_matrix(Plane::YW, alpha) * get_simple_rotation_matrix(Plane::ZX, beta);

            // α-ZW, β-XY
            case Plane::ZW: return get_simple_rotation_matrix(Plane::ZW, alpha) * get_simple_rotation_matrix(Plane::XY, beta);
            }
        }

        /// See the notes above in `get_double_rotation_matrix(...)`. This function is
        /// mostly here for completeness.
        glm::mat4 get_isoclinic_rotation_matrix(Plane first_plane, float alpha_beta)
        {
            return get_double_rotation_matrix(first_plane, alpha_beta, alpha_beta);
        }

        /// Takes a 4D cross product between `u`, `v`, and `w`. The result is a vector in
        /// 4-dimensions that is simultaneously orthogonal to `u`, `v`, and `w`.
        ///
        /// Reference: `https://ef.gy/linear-algebra:normal-vectors-in-higher-dimensional-spaces`
        glm::vec4 cross(const glm::vec4& u, const glm::vec4& v, const glm::vec4& w)
        {
            const auto a = (v[0] * w[1]) - (v[1] * w[0]);
            const auto b = (v[0] * w[2]) - (v[2] * w[0]);
            const auto c = (v[0] * w[3]) - (v[3] * w[0]);
            const auto d = (v[1] * w[2]) - (v[2] * w[1]);
            const auto e = (v[1] * w[3]) - (v[3] * w[1]);
            const auto f = (v[2] * w[3]) - (v[3] * w[2]);

            auto result = glm::vec4{
                 (u[1] * f) - (u[2] * e) + (u[3] * d),
                -(u[0] * f) + (u[2] * c) - (u[3] * b),
                 (u[0] * e) - (u[1] * c) + (u[3] * a),
                -(u[0] * d) + (u[1] * b) - (u[2] * a)
            };
            
            return result;
        }

		/// Given a set of vertices embedded in 4-dimensions that lie inside `hyperplane`,
		/// find a proper ordering of the points such that the resulting list of vertices can
		/// be traversed in order to create a fan of distinct, non-overlapping triangles. Note
		/// that for the purposes of this application, we don't care if the list ends up
		/// in a "clockwise" or "counter-clockwise" order.
		///
		/// Reference: `https://math.stackexchange.com/questions/978642/how-to-sort-vertices-of-a-polygon-in-counter-clockwise-order`
		std::vector<glm::vec4> sort_points_on_plane(const std::vector<glm::vec4>& points, const Hyperplane& hyperplane)
		{
            size_t largest_index = utils::index_of_largest(hyperplane.normal);

            // First, project the 4D points to 3D. We do this by dropping the coordinate
            // corresponding to the largest value of the hyperplane's normal vector.
            //
            // TODO: does this work all the time?
            std::vector<glm::vec3> projected;
            for (const auto& point : points)
            {
                projected.push_back(utils::truncate_n(point, largest_index));
            }
            assert(projected.size() == points.size());

            // Now, we are working in 3-dimensions.
            auto a = projected[0];
            auto b = projected[1];
            auto c = projected[2];
            auto centroid = utils::average(projected, glm::vec3{ 0.0f });

            // Calculate the normal of this polygon by taking the cross product
            // between two of its edges.
            auto ab = b - a;
            auto bc = c - b;
            auto polygon_normal = glm::normalize(glm::cross(bc, ab));
            auto first_edge = glm::normalize(a - centroid);

            // Sort the new set of 3D points based on their signed angles.
            std::vector<std::pair<size_t, float>> indices;
            for (size_t i = 1; i < projected.size(); ++i)
            {
                auto edge = glm::normalize(projected[i] - centroid);
                auto angle = utils::saturate_between(glm::dot(first_edge, edge), -1.0f, 1.0f);
                auto signed_angle = acosf(angle);

                if (glm::dot(polygon_normal, glm::cross(first_edge, edge)) < 0.0f) 
                {
                    signed_angle *= -1.0f;
                }

                size_t index = indices.size() + 1;

                indices.push_back({ index, signed_angle });
            }

            // Add the first point `a`.
            indices.push_back({ 0, 0.0f });
            std::sort(indices.begin(), indices.end(), 
                [](const std::pair<size_t, float> & a, const std::pair<size_t, float> & b) 
                {
                    return (a.second < b.second);
                }
            );

            // Now, return the original set of 4D points in the proper order.
            std::vector<glm::vec4> points_sorted;
            for (const auto [index, angle] : indices)
            {
                points_sorted.push_back(points[index]);
            }
            return points_sorted;
		}

	}

}