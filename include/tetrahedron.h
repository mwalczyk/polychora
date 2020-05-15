#include "glm.hpp"

#include <array>

namespace four
{
	/// A struct representing a tetrahedron (3-simplex) embedded in 4-dimensions. This
	/// is the building block for all 4-dimensional meshes.
	class Tetrahedron
	{

	public:

		static std::array<size_t, 12> get_edge_indices()
		{
			return {
				0, 1,
				0, 2,
				0, 3,
				1, 2,
				1, 3,
				2, 3
			};
		}

		/// Returns the number of vertices that make up a tetrahedron.
		static size_t get_number_of_vertices()
		{
			return 4;
		}

		/// Returns the number of edges that make up a tetrahedron.
		static size_t get_number_of_edges()
		{
			return 6;
		}

		/// Slicing a tetrahedron with a plane will produce either 0, 3, or 4
		/// points of intersection. In the case that the slicing procedure returns
		/// 4 unique vertices, we need to know how to connect these vertices to
		/// form a closed polygon (i.e. a quadrilateral). Assuming these vertices
		/// are sorted in either a clockwise or counter-clockwise order (relative
		/// to the normal of the plane of intersection), we can use the indices
		/// here to produce two, non-overlapping triangles.
		///
		/// Returns the indices for a quadrilateral slice.
		static std::array<size_t, 6> get_quad_indices() 
		{
			return { 
				0, 1, 2,
				0, 2, 3
			};
		}

		Tetrahedron(const std::array<glm::vec4, 4>& vertices, size_t cell_index, const glm::vec4& cell_centroid) :
			vertices{ vertices },
			cell_index{ cell_index },
			cell_centroid{ cell_centroid }
		{
		}

		
	private:

		// The 4 vertices that make up this tetrahedron
		std::array<glm::vec4, 4> vertices;

		// The integer index of the cell that this tetrahedron belongs to
		size_t cell_index;
		
		// The centroid (in 4-space) of the cell that this tetrahedron belongs to
		glm::vec4 cell_centroid;

		friend class Mesh;
	};

}