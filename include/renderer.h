#pragma once

#include <array>
#include <vector>

#include "glm.hpp"

#include "hyperplane.h"
#include "shader.h"

namespace four
{

    struct Tetrahedra
    {
        // All of the tetrahedra vertices as a single, flat array (4 vertices per tetrahedra)
        std::vector<glm::vec4> vertices;

        // All of the simplex indices (4 indices per tetrahedra)
        std::vector<size_t> simplices;

        // All of the hyperplane normals corresponding to each tetrahedron (from convex hull)
        std::vector<glm::vec4> normals;
    };
    
    std::array<size_t, 12> get_edge_indices()
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

	class Renderer
	{

	public:

        Renderer() :
            compute{ graphics::Shader{ "../shaders/compute_slice.glsl" } }
        {}

        void add_tetrahedra(const Tetrahedra& tetrahedra, const glm::mat4& transform = glm::mat4{ 1.0f })
        {
            Batch batch;

            std::vector<glm::vec4> tetrahedra_vertices;
            std::vector<glm::vec4> tetrahedra_colors;
            batch.number_of_tetrahedra = tetrahedra.simplices.size() / 4;

            // Any tetrahedral slice can have at most 6 vertices (a quadrilateral, 2 triangles)
            const size_t max_vertices_per_slice = 6;
            const size_t number_of_vertices_per_tetrahedron = 4;

            for (size_t simplex_index = 0; simplex_index < tetrahedra.simplices.size() / 4; ++simplex_index)
            {
                tetrahedra_vertices.push_back(tetrahedra.vertices[tetrahedra.simplices[simplex_index * 4 + 0]]);
                tetrahedra_vertices.push_back(tetrahedra.vertices[tetrahedra.simplices[simplex_index * 4 + 1]]);
                tetrahedra_vertices.push_back(tetrahedra.vertices[tetrahedra.simplices[simplex_index * 4 + 2]]);
                tetrahedra_vertices.push_back(tetrahedra.vertices[tetrahedra.simplices[simplex_index * 4 + 3]]);

                // Next, push back all of this tetrahedron's colors (currently, we are
                // using the cell centroid to generate some sort of shading / colors)
                //
                // TODO: see notes on attribute divisors in `init_render_objects(...)`
                for (size_t i = 0; i < max_vertices_per_slice; ++i)
                {
                    tetrahedra_colors.push_back(tetrahedra.normals[simplex_index]);
                }
            }

            glCreateVertexArrays(1, &batch.vao_slice);

            // Set up attribute #0: positions
            const uint32_t attrib_pos = 0;
            const uint32_t binding_pos = 0;
            glEnableVertexArrayAttrib(batch.vao_slice, attrib_pos);
            glVertexArrayAttribFormat(batch.vao_slice, attrib_pos, 4, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayAttribBinding(batch.vao_slice, attrib_pos, binding_pos);

            // Set up attribute #1: colors
            const uint32_t atrrib_col = 1;
            const uint32_t binding_col = 1;
            glEnableVertexArrayAttrib(batch.vao_slice, atrrib_col);
            glVertexArrayAttribFormat(batch.vao_slice, atrrib_col, 4, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayAttribBinding(batch.vao_slice, atrrib_col, binding_col);

            auto vertices_size = sizeof(glm::vec4) * number_of_vertices_per_tetrahedron * batch.number_of_tetrahedra;
            auto colors_size = sizeof(glm::vec4) * max_vertices_per_slice * batch.number_of_tetrahedra;
            std::cout << "Vertices buffer size: " << vertices_size << std::endl;
            std::cout << "Colors buffer size: " << colors_size << std::endl;

            // The VBO that will be associated with the vertex attribute #1, which does not change
            // throughout the lifetime of the program (thus, we use the flag `STATIC_DRAW` below)
            glCreateBuffers(1, &batch.buffer_slice_colors);
            glNamedBufferData(batch.buffer_slice_colors, colors_size, tetrahedra_colors.data(), GL_STATIC_DRAW);

            // The buffer that will be bound at index #0 and read from
            glCreateBuffers(1, &batch.buffer_tetrahedra);
            glNamedBufferData(batch.buffer_tetrahedra, vertices_size, tetrahedra_vertices.data(), GL_STATIC_DRAW);

            // The buffer of slice vertices that will be written to whenever the slicing hyperplane moves
            auto alloc_size = sizeof(glm::vec4) * max_vertices_per_slice * batch.number_of_tetrahedra;
            glCreateBuffers(1, &batch.buffer_slice_vertices);
            glNamedBufferData(batch.buffer_slice_vertices, alloc_size, nullptr, GL_STREAM_DRAW);

            // The buffer of draw commands that will be filled out by the compute shader dispatch
            alloc_size = sizeof(DrawCommand) * batch.number_of_tetrahedra;
            glCreateBuffers(1, &batch.buffer_indirect_commands);
            glNamedBufferData(batch.buffer_indirect_commands, alloc_size, nullptr, GL_STREAM_DRAW);

            // Setup vertex attribute bindings
            glVertexArrayVertexBuffer(batch.vao_slice, binding_pos, batch.buffer_slice_vertices, 0, sizeof(glm::vec4));
            glVertexArrayVertexBuffer(batch.vao_slice, binding_col, batch.buffer_slice_colors, 0, sizeof(glm::vec4));

            std::array<int32_t, 3> local_size;
            glGetProgramiv(compute.get_handle(), GL_COMPUTE_WORK_GROUP_SIZE, local_size.data());

            batches.push_back(batch);
        }

        size_t get_number_of_objects() const
        {
            return batches.size();
        }

        void slice_object(size_t index, const Hyperplane& hyperplane) const
        {
            compute.use();

            compute.uniform_vec4("u_hyperplane_normal", hyperplane.get_normal());
            compute.uniform_float("u_hyperplane_displacement", hyperplane.get_displacement());
            compute.uniform_mat4("u_transform", batches[index].transform);

            // Bind buffers for read / write
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, batches[index].buffer_tetrahedra);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, batches[index].buffer_slice_vertices);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, batches[index].buffer_indirect_commands);

            uint32_t dispatch = ceilf(batches[index].number_of_tetrahedra / 128.0f);
            glDispatchCompute(dispatch, 1, 1);

            // Barrier against subsequent SSBO reads and indirect drawing commands
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
        }

        void slice_objects(const Hyperplane& hyperplane) const
        {
            for (size_t index = 0; index < batches.size(); index++)
            {
                slice_object(index, hyperplane);
            }
        }

        void set_transform(size_t index, const glm::mat4& transform)
        {
            batches[index].transform = transform;
        }

        void set_transforms(const glm::mat4& transform)
        {
            for (size_t index = 0; index < batches.size(); index++)
            {
                set_transform(index, transform);
            }
        }

        void draw_sliced_object(size_t index) const
        {
            // First, bind this batch's VAO
            glBindVertexArray(batches[index].vao_slice);

            // Bind the buffer that contains indirect draw commands
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batches[index].buffer_indirect_commands);

            // Dispatch indirect draw commands
            glMultiDrawArraysIndirect(GL_TRIANGLES, nullptr, batches[index].number_of_tetrahedra, sizeof(DrawCommand));
        }

        void draw_sliced_objects() const
        {
            for (size_t index = 0; index < batches.size(); index++)
            {
                draw_sliced_object(index);
            }
        }

	private:

        struct DrawCommand
        {
            uint32_t count;
            uint32_t instance_count;
            uint32_t first;
            uint32_t base_instance;
        };

        struct Batch
        {
            /// The vertex array object (VAO) that is used for drawing a 3D slice of this mesh
            uint32_t vao_slice = 0;

            /// A GPU-side buffer that contains all of the tetrahedra that make up this mesh
            uint32_t buffer_tetrahedra = 0;

            /// A GPU-side buffer that contains all of the colors used to render 3-dimensional slices of this mesh
            uint32_t buffer_slice_colors = 0;

            /// A GPU-side buffer that contains all of the vertices that make up the active 3-dimensional cross-section of this mesh
            uint32_t buffer_slice_vertices = 0;

            /// A GPU-side buffer that will be filled with indirect drawing commands via the `compute` program
            uint32_t buffer_indirect_commands;

            glm::mat4 transform = glm::mat4{ 1.0f };

            size_t number_of_tetrahedra = 0;
        };

        // All drawable batches of 4D objects
        std::vector<Batch> batches;

        // The compute shader that is used to compute 3-dimensional slices of this mesh
        graphics::Shader compute;

	};
}