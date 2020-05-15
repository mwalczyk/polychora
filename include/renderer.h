#pragma once

#include <vector>

#include "glm.hpp"

#include "hyperplane.h"
#include "shader.h"
#include "tetrahedron.h"

namespace four
{

    struct DrawCommand
    {
        uint32_t count;
        uint32_t instance_count;
        uint32_t first;
        uint32_t base_instance;
    };

    struct Tetrahedra
    {
        std::vector<glm::vec4> vertices;
        std::vector<size_t> simplices;
        std::vector<glm::vec4> normals;
    };

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

            for (size_t simplex_index = 0; simplex_index < tetrahedra.simplices.size() / 4; simplex_index++)
            {
                tetrahedra_vertices.push_back(tetrahedra.vertices[tetrahedra.simplices[simplex_index * 4 + 0]]);
                tetrahedra_vertices.push_back(tetrahedra.vertices[tetrahedra.simplices[simplex_index * 4 + 1]]);
                tetrahedra_vertices.push_back(tetrahedra.vertices[tetrahedra.simplices[simplex_index * 4 + 2]]);
                tetrahedra_vertices.push_back(tetrahedra.vertices[tetrahedra.simplices[simplex_index * 4 + 3]]);
                
                // Any tetrahedral slice can have at most 6 vertices (a quadrilateral, 2 triangles).
                const size_t max_vertices_per_slice = 6;

                // Next, push back all of this tetrahedron's colors (currently, we are
                // using the cell centroid to generate some sort of shading / colors).
                // TODO: see notes on attribute divisors in `init_render_objects(...)`
                for (size_t i = 0; i < max_vertices_per_slice; ++i)
                {
                    tetrahedra_colors.push_back(tetrahedra.normals[simplex_index]);
                }
            }

            glCreateVertexArrays(1, &batch.vao_slice);

            // Set up attribute #0: positions.
            const uint32_t ATTR_POS = 0;
            const uint32_t BINDING_POS = 0;
            glEnableVertexArrayAttrib(batch.vao_slice, ATTR_POS);
            glVertexArrayAttribFormat(batch.vao_slice, ATTR_POS, 4, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayAttribBinding(batch.vao_slice, ATTR_POS, BINDING_POS);

            // Set up attribute #1: colors.
            const uint32_t ATTR_COL = 1;
            const uint32_t BINDING_COL = 1;
            glEnableVertexArrayAttrib(batch.vao_slice, ATTR_COL);
            glVertexArrayAttribFormat(batch.vao_slice, ATTR_COL, 4, GL_FLOAT, GL_FALSE, 0);
            glVertexArrayAttribBinding(batch.vao_slice, ATTR_COL, BINDING_COL);

            // Any tetrahedral slice can have at most 6 vertices (a quadrilateral, 2 triangles).
            size_t max_vertices_per_slice = 6;

            auto vertices_size = sizeof(glm::vec4) * Tetrahedron::get_number_of_vertices() * batch.number_of_tetrahedra;
            auto colors_size = sizeof(glm::vec4) * max_vertices_per_slice * batch.number_of_tetrahedra;
            std::cout << "Vertices buffer size: " << vertices_size << std::endl;
            std::cout << "Colors buffer size: " << colors_size << std::endl;

            // The VBO that will be associated with the vertex attribute #1, which does not change
            // throughout the lifetime of the program (thus, we use the flag `STATIC_DRAW` below).
            glCreateBuffers(1, &batch.buffer_slice_colors);
            glNamedBufferData(
                batch.buffer_slice_colors,
                colors_size,
                tetrahedra_colors.data(),
                GL_STATIC_DRAW
            );

            // The buffer that will be bound at index #0 and read from.
            glCreateBuffers(1, &batch.buffer_tetrahedra);
            glNamedBufferData(
                batch.buffer_tetrahedra,
                vertices_size,
                tetrahedra_vertices.data(),
                GL_STATIC_DRAW
            );

            // Items that will be written to on the GPU (more or less every frame).
            // ...

            // The buffer of slice vertices that will be written to whenever the slicing hyperplane moves.
            auto alloc_size = sizeof(glm::vec4) * max_vertices_per_slice * batch.number_of_tetrahedra;
            glCreateBuffers(1, &batch.buffer_slice_vertices);
            glNamedBufferData(
                batch.buffer_slice_vertices,
                alloc_size,
                nullptr,
                GL_STREAM_DRAW
            );

            // The buffer of draw commands that will be filled out by the compute shader dispatch.
            alloc_size = sizeof(DrawCommand) * batch.number_of_tetrahedra;
            glCreateBuffers(1, &batch.buffer_indirect_commands);
            glNamedBufferData(
                batch.buffer_indirect_commands,
                alloc_size,
                nullptr,
                GL_STREAM_DRAW
            );

            // Setup vertex attribute bindings.
            glVertexArrayVertexBuffer(
                batch.vao_slice,
                BINDING_POS,
                batch.buffer_slice_vertices,
                0,
                sizeof(glm::vec4)
            );
            glVertexArrayVertexBuffer(
                batch.vao_slice,
                BINDING_COL,
                batch.buffer_slice_colors,
                0,
                sizeof(glm::vec4)
            );

            std::array<int32_t, 3> local_size;
            glGetProgramiv(compute.get_handle(), GL_COMPUTE_WORK_GROUP_SIZE, local_size.data());

            batches.push_back(batch);
        }

        void slice_objects(const Hyperplane& hyperplane)
        {
            compute.use();

            for (const auto& batch : batches)
            {
                compute.uniform_vec4("u_hyperplane_normal", hyperplane.get_normal());
                compute.uniform_float("u_hyperplane_displacement", hyperplane.get_displacement());
                compute.uniform_mat4("u_transform", batch.transform);

                // Bind buffers for read / write.
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, batch.buffer_tetrahedra);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, batch.buffer_slice_vertices);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, batch.buffer_indirect_commands);

                uint32_t dispatch = ceilf(batch.number_of_tetrahedra / 128.0f);
                glDispatchCompute(dispatch, 1, 1);

                // Barrier against subsequent SSBO reads and indirect drawing commands.
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
            }
        }

        void draw_sliced_objects() const
        {
            for (const auto& batch : batches)
            {
                glBindVertexArray(batch.vao_slice);

                // Bind the buffer that contains indirect draw commands.
                glBindBuffer(GL_DRAW_INDIRECT_BUFFER, batch.buffer_indirect_commands);

                // Dispatch indirect draw commands.
                glMultiDrawArraysIndirect(GL_TRIANGLES, nullptr, batch.number_of_tetrahedra, sizeof(DrawCommand));
            }
        }

	private:

        struct Batch
        {
            /// The vertex array object (VAO) that is used for drawing a 3D slice of this mesh
            uint32_t vao_slice;

            /// A GPU-side buffer that contains all of the tetrahedra that make up this mesh
            uint32_t buffer_tetrahedra;

            /// A GPU-side buffer that contains all of the colors used to render 3-dimensional slices of this mesh
            uint32_t buffer_slice_colors;

            /// A GPU-side buffer that contains all of the vertices that make up the active 3-dimensional cross-section of this mesh
            uint32_t buffer_slice_vertices;

            /// A GPU-side buffer that will be filled with indirect drawing commands via the `compute` program
            uint32_t buffer_indirect_commands;

            glm::mat4 transform = glm::mat4{ 1.0f };

            size_t number_of_tetrahedra;
        };

        std::vector<Batch> batches;

        /// The compute shader that is used to compute 3-dimensional slices of this mesh
        graphics::Shader compute;

	};
}