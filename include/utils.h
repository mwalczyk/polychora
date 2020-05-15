#pragma once

#include <numeric>

#include "glm.hpp"

namespace four
{

	namespace utils
	{

        /// Returns the index of the largest component of the vector.
        //
        // TODO: use templates
		size_t index_of_largest(const glm::vec4& v)
		{
            float largest_val = abs(v.x);
            size_t largest_index = 0;

            if (abs(v.y) > largest_val)
            {
                largest_val = abs(v.y);
                largest_index = 1;
            }
            if (abs(v.z) > largest_val)
            {
                largest_val = abs(v.z);
                largest_index = 2;
            }
            if (abs(v.w) > largest_val)
            {
                largest_val = abs(v.w);
                largest_index = 3;
            }
            
            return largest_index;
		}

        glm::vec3 truncate_n(const glm::vec4& v, size_t index)
        {
            switch (index)
            {
            case 0: return { v.y, v.z, v.w };
            case 1: return { v.x, v.z, v.w };
            case 2: return { v.x, v.y, v.w };
            case 3: return { v.x, v.y, v.z };
            }
        }

        /// Clamps `value` so that it lies in the range `min .. max`.
        float saturate_between(float value, float min_v, float max_v)
        {
            return fmax(fmin(value, max_v), min_v);
        }

        template<class T>
        T average(const std::vector<T>& values, const T& initial)
        {
            return std::accumulate(values.begin(), values.end(), initial) / static_cast<float>(values.size());
        }
	}

}