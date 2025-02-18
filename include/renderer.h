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
        std::vector<uint32_t> simplices;

        std::vector<uint32_t> edges;

        // All of the hyperplane normals corresponding to each tetrahedron (from convex hull)
        std::vector<glm::vec4> normals;
    };
    
    std::array<std::pair<uint32_t, uint32_t>, 6> get_edge_indices()
    {
        return { {
            { 0, 1 },
            { 0, 2 },
            { 0, 3 },
            { 1, 2 },
            { 1, 3 },
            { 2, 3 }
        } };
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
            batch.number_of_edges = tetrahedra.edges.size() / 2;

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
                for (size_t i = 0; i < max_vertices_per_slice; ++i)
                {
                    tetrahedra_colors.push_back(tetrahedra.normals[simplex_index]);
                }
            }

            {
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

                // The VBO that will be associated with the vertex attribute #1, which does not change
                // throughout the lifetime of the program (thus, we use the flag `STATIC_DRAW` below)
                glCreateBuffers(1, &batch.buffer_slice_hyperplane_normals);
                glNamedBufferData(batch.buffer_slice_hyperplane_normals, colors_size, tetrahedra_colors.data(), GL_STATIC_DRAW);

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
                glVertexArrayVertexBuffer(batch.vao_slice, binding_col, batch.buffer_slice_hyperplane_normals, 0, sizeof(glm::vec4));

                std::array<int32_t, 3> local_size;
                glGetProgramiv(compute.get_handle(), GL_COMPUTE_WORK_GROUP_SIZE, local_size.data());
            }

            // Compute the indices required to render a wireframe of all of this polychoron's tetrahedra
            std::vector<uint32_t> tetrahedra_indices;
            for (size_t simplex_index = 0; simplex_index < tetrahedra.simplices.size() / 4; ++simplex_index)
            {
                // Grab the vertex IDs of this simplex
                auto a = tetrahedra.simplices[simplex_index * 4 + 0];
                auto b = tetrahedra.simplices[simplex_index * 4 + 1];
                auto c = tetrahedra.simplices[simplex_index * 4 + 2];
                auto d = tetrahedra.simplices[simplex_index * 4 + 3];
        
                // Each simplex has 6 unique edges, we can easily enumerate these: { (0, 1), (0, 2), (0, 3), (1, 2), (1, 3), (2, 3) }
                tetrahedra_indices.push_back(a);
                tetrahedra_indices.push_back(b);

                tetrahedra_indices.push_back(a);
                tetrahedra_indices.push_back(c);

                tetrahedra_indices.push_back(a);
                tetrahedra_indices.push_back(d);

                tetrahedra_indices.push_back(b);
                tetrahedra_indices.push_back(c);

                tetrahedra_indices.push_back(b);
                tetrahedra_indices.push_back(d);

                tetrahedra_indices.push_back(c);
                tetrahedra_indices.push_back(d);
            }

            {
                glCreateVertexArrays(1, &batch.vao_skeleton);

                // The indices that will be used to draw the wireframes of all of the tetrahedra that make up this polychoron
                glCreateBuffers(1, &batch.ebo_tetrahedra);
                glNamedBufferData(batch.ebo_tetrahedra, tetrahedra_indices.size() * sizeof(uint32_t), tetrahedra_indices.data(), GL_STATIC_DRAW);

                // The indices that will be used to draw the "skeleton" (i.e. unique edges) of this polychoron
                glCreateBuffers(1, &batch.ebo_edges);
                glNamedBufferData(batch.ebo_edges, tetrahedra.edges.size() * sizeof(uint32_t), tetrahedra.edges.data(), GL_STATIC_DRAW);
            
                // The buffer containing all of the unique vertices of this polychoron
                glCreateBuffers(1, &batch.buffer_vertices);
                glNamedBufferData(batch.buffer_vertices, tetrahedra.vertices.size() * sizeof(glm::vec4), tetrahedra.vertices.data(), GL_STATIC_DRAW);

                // Set up attribute #0: positions
                glEnableVertexArrayAttrib(batch.vao_skeleton, 0);
                glVertexArrayAttribFormat(batch.vao_skeleton, 0, 4, GL_FLOAT, GL_FALSE, 0);
                glVertexArrayAttribBinding(batch.vao_skeleton, 0, 0);

                // Setup vertex attribute bindings
                glVertexArrayVertexBuffer(batch.vao_skeleton, 0, batch.buffer_vertices, 0, sizeof(float) * 4);

                // Bind the EBO to the VAO (this might be switched during rendering)
                glVertexArrayElementBuffer(batch.vao_skeleton, batch.ebo_tetrahedra);
            }

            batches.push_back(batch);
        }

        size_t get_number_of_objects() const
        {
            return batches.size();
        }

        void slice_object(size_t index, const Hyperplane& hyperplane) const
        {
            compute.use();

            compute.uniform_vec4("u_hyperplane_normal", hyperplane.normal);
            compute.uniform_float("u_hyperplane_displacement", hyperplane.displacement);
            compute.uniform_mat4("u_transform", batches[index].transform);
            compute.uniform_vec4("u_translation", batches[index].translation);
            compute.uniform_int("u_object_index", index);

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

        void set_transform(size_t index, const glm::mat4& transform, const glm::vec4& translation = glm::vec4{ 0.0f })
        {
            batches[index].transform = transform;
            batches[index].translation = translation;
        }

        void set_transforms(const glm::mat4& transform, const glm::vec4& translation = glm::vec4{ 0.0f })
        {
            for (size_t index = 0; index < batches.size(); index++)
            {
                set_transform(index, transform, translation);
            }
        }

        const glm::mat4& get_transform(size_t index) const
        {
            return batches[index].transform;
        }

        const glm::vec4& get_translation(size_t index) const
        {
            return batches[index].translation;
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

        void draw_skeleton_object(size_t index, bool tetrahedra_wireframes = true) const
        {
            glBindVertexArray(batches[index].vao_skeleton);
            
            if (tetrahedra_wireframes)
            {
                // 6 edges per tetrahedra, 2 components each
                size_t number_of_tetrahedral_edges = batches[index].number_of_tetrahedra * 6 * 2;

                glVertexArrayElementBuffer(batches[index].vao_skeleton, batches[index].ebo_tetrahedra);
                glDrawElements(GL_LINES, number_of_tetrahedral_edges, GL_UNSIGNED_INT, nullptr);
            }
            else
            {
                // The `number_of_edges` member will be set to the length of the edges EBO / 2, so we 
                // want to make sure to multiply by 2 again here
                glVertexArrayElementBuffer(batches[index].vao_skeleton, batches[index].ebo_edges);
                glDrawElements(GL_LINES, batches[index].number_of_edges * 2, GL_UNSIGNED_INT, nullptr);
            }
        }

        void draw_skeleton_objects(bool tetrahedra_wireframes = true) const
        {
            for (size_t index = 0; index < batches.size(); index++)
            {
                draw_skeleton_object(index, tetrahedra_wireframes);
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

            /// A GPU-side buffer that contains all of the tetrahedra that make up this mesh (4 vertices per tetrahedron)
            uint32_t buffer_tetrahedra = 0;

            /// A GPU-side buffer that contains all of the hyperplane normals corresponding to each of the sliced tetrahedra
            uint32_t buffer_slice_hyperplane_normals = 0;

            /// A GPU-side buffer that contains all of the vertices that make up the active 3-dimensional cross-section of this mesh
            uint32_t buffer_slice_vertices = 0;

            /// A GPU-side buffer that will be filled with indirect drawing commands via the `compute` program
            uint32_t buffer_indirect_commands = 0;

            /// The vertex array object (VAO) that is used for drawing an "outline" of this mesh (either edges or tetrahedra wireframes) 
            uint32_t vao_skeleton = 0;

            /// The index buffer used for rendering tetrahedra wireframes
            uint32_t ebo_tetrahedra = 0;

            /// The index buffer used for rendering edges (of the entire polychoron)
            uint32_t ebo_edges = 0;

            /// A GPU-side buffer that contains all of the unique vertices that make up this 4-dimensional mesh
            uint32_t buffer_vertices = 0;

            /// This batch's transformation matrix (in 4-space)
            glm::mat4 transform = glm::mat4{ 1.0f };

            /// This batch's translation (in 4-space): note that glm doesn't support 5x5 matrices, so we have to do this
            glm::vec4 translation = glm::vec4{ 0.0f };

            /// The total number of tetrahedra that are in this batch
            size_t number_of_tetrahedra = 0;

            /// The total number of unique edges that are in this batch (i.e. for the 120-cell, this equals 1200)
            size_t number_of_edges = 0;
        };

        // All drawable batches of 4D objects
        std::vector<Batch> batches;

        // The compute shader that is used to compute 3-dimensional slices of this mesh
        graphics::Shader compute;

	};
}