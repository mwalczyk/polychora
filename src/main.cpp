#include <algorithm>
#include <iostream>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "libqhullcpp/RboxPoints.h"
#include "libqhullcpp/Qhull.h"
#include "libqhullcpp/QhullFacetList.h"
#include "libqhullcpp/QhullVertexSet.h"

#include "camera.h"
#include "maths.h"
#include "polychora.h"
#include "renderer.h"
#include "shader.h"

// Data that will be associated with the GLFW window
struct InputData
{
    bool imgui_active = false;
} input_data;

// Viewport and camera settings
const uint32_t window_w = 1200;
const uint32_t window_h = 800;
bool first_mouse = true;
float last_x;
float last_y;
float zoom = 45.0f;
glm::mat4 arcball_camera_matrix = glm::lookAt(glm::vec3{ 0.0f, 0.5f, 6.0f }, glm::vec3{ 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });
glm::mat4 arcball_model_matrix = glm::mat4{ 1.0f };

// 4-dimensional rendering settings
size_t polychoron_index = 0;
float rotation_xy = 0.0f;
float rotation_yz = 0.0f;
float rotation_zx = 0.0f;
float rotation_xw = 0.0f;
float rotation_yw = 0.0f;
float rotation_zw = 0.0f;
bool display_wireframe = false;

// Appearance settings
ImVec4 clear_color = ImVec4(0.091f, 0.062f, 0.127f, 1.0f);

GLFWwindow* window;

/**
 * A function for handling scrolling.
 */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (zoom >= 1.0f && zoom <= 90.0f)
    {
        zoom -= yoffset;
    }
    if (zoom <= 1.0f)
    {
        zoom = 1.0f;
    }
    if (zoom >= 90.0f)
    {
        zoom = 90.0f;
    }
}

/**
 * A function for handling key presses.
 */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        // Close the GLFW window
        glfwSetWindowShouldClose(window, true);
    }
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        // Reset the arcball camera
        arcball_camera_matrix = glm::lookAt(glm::vec3{ 6.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f }, glm::vec3{ 1.0f, 1.0f, 0.0f });
        arcball_model_matrix = glm::mat4{ 1.0f };
    }
}

/**
 * Get a normalized vector from the center of a virtual sphere centered at the origin to
 * a point `point_on_sphere` on the virtual ball surface, such that `point_on_sphere`
 * is aligned on screen's (x, y) coordinates.  If (x, y) is too far away from the
 * sphere, return the nearest point on the virtual ball surface.
 */
glm::vec3 get_arcball_vector(int x, int y)
{
    auto point_on_sphere = glm::vec3{
        1.0f * x / window_w * 2.0f - 1.0f,
        1.0f * y / window_h * 2.0f - 1.0f,
        0.0f
    };

    point_on_sphere.y = -point_on_sphere.y;

    const float op_squared = point_on_sphere.x * point_on_sphere.x + point_on_sphere.y * point_on_sphere.y;

    if (op_squared <= 1.0f * 1.0f)
    {
        // Pythagorean theorem
        point_on_sphere.z = sqrt(1.0f * 1.0f - op_squared);
    }
    else
    {
        // Nearest point
        point_on_sphere = glm::normalize(point_on_sphere);
    }

    return point_on_sphere;
}

/**
 * Performs arcball camera calculations.
 */
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // First, check if the user is interacting with the ImGui interface - if they are,
    // we don't want to process mouse events any further
    auto input_data = static_cast<InputData*>(glfwGetWindowUserPointer(window));

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !input_data->imgui_active)
    {
        if (first_mouse)
        {
            last_x = xpos;
            last_y = ypos;
            first_mouse = false;
        }

        if (xpos != last_x || ypos != last_y)
        {
            const float rotation_speed = 0.25f;

            glm::vec3 va = get_arcball_vector(last_x, last_y);
            glm::vec3 vb = get_arcball_vector(xpos, ypos);
            const float angle = acos(std::min(1.0f, glm::dot(va, vb))) * rotation_speed;
            const glm::vec3 axis_camera_coordinates = glm::cross(va, vb);

            glm::mat3 camera_to_object = glm::inverse(glm::mat3(arcball_camera_matrix) * glm::mat3(arcball_model_matrix));

            glm::vec3 axis_in_object_coord = camera_to_object * axis_camera_coordinates;

            arcball_model_matrix = glm::rotate(arcball_model_matrix, glm::degrees(angle), axis_in_object_coord);

            // Set last to current
            last_x = xpos;
            last_y = ypos;
        }
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        last_x = xpos;
        last_y = ypos;
    }
}

/**
 * Debug function that will be used internally by OpenGL to print out warnings, errors, etc.
 */
void message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param)
{
    auto const src_str = [source]() {
        switch (source)
        {
        case GL_DEBUG_SOURCE_API: return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
        case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
        case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
        case GL_DEBUG_SOURCE_OTHER: return "OTHER";
        }
    }();

    auto const type_str = [type]() {
        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR: return "ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER: return "MARKER";
        case GL_DEBUG_TYPE_OTHER: return "OTHER";
        }
    }();

    auto const severity_str = [severity]() {
        switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
        case GL_DEBUG_SEVERITY_LOW: return "LOW";
        case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
        case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
        }
    }();

    std::cout << src_str << ", " << type_str << ", " << severity_str << ", " << id << ": " << message << '\n';
}

glm::mat4 build_simple_rotation_matrix()
{
    return four::maths::get_simple_rotation_matrix(four::maths::Plane::XY, rotation_xy) *
           four::maths::get_simple_rotation_matrix(four::maths::Plane::YZ, rotation_yz) *
           four::maths::get_simple_rotation_matrix(four::maths::Plane::ZX, rotation_zx) *
           four::maths::get_simple_rotation_matrix(four::maths::Plane::XW, rotation_xw) *
           four::maths::get_simple_rotation_matrix(four::maths::Plane::YW, rotation_yw) *
           four::maths::get_simple_rotation_matrix(four::maths::Plane::ZW, rotation_zw);
}

/**
 * Initialize GLFW and the OpenGL context.
 */
void initialize()
{
    // Create and configure the GLFW window 
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, false);
    glfwWindowHint(GLFW_SAMPLES, 4);
    window = glfwCreateWindow(window_w, window_h, "Polychora", nullptr, nullptr);

    if (window == nullptr)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetWindowUserPointer(window, &input_data);

    // Load function pointers from glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // Setup initial OpenGL state
    {
#if defined(_DEBUG)
        // Debug logging
        //glEnable(GL_DEBUG_OUTPUT);
        //glDebugMessageCallback(message_callback, nullptr);
#endif
        // Depth testing
        glEnable(GL_DEPTH_TEST);

        // For now, we don't really know the winding order of the tetrahedron
        // slices, so we want to disable face culling
        glDisable(GL_CULL_FACE);
    }
}

double round_nearest_tenth(double x) 
{ 
    return floor(x * 10.0 + 0.5) / 10.0;
}

std::vector<four::Tetrahedra> run_qhull()
{
    std::vector<std::vector<four::combinatorics::PermutationSeed<float>>> all_permutation_seeds; //= four::get_all_permutation_seeds();
    const float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
    const float phi_1 = phi;
    const float phi_2 = powf(phi, 2.0f);
    const float phi_3 = powf(phi, 3.0f);
    const float phi_4 = powf(phi, 4.0f);
    const float phi_5 = powf(phi, 5.0f);
    const float phi_6 = powf(phi, 6.0f);


    auto cell8 =
    {
        // All
        four::combinatorics::PermutationSeed<float>{ { 1.0f, 1.0f, 1.0f, 1.0f }, true, four::combinatorics::Parity::ALL },
        four::combinatorics::PermutationSeed<float>{ { 2.0f, 0.0f, 0.0f, 0.0f }, true, four::combinatorics::Parity::ALL },

        // Even
        four::combinatorics::PermutationSeed<float>{ { phi, 1.0f, powf(phi, -1.0f), 0.0f }, true, four::combinatorics::Parity::EVEN },
    };

    for (size_t i = 0; i < 9; i++)
    {
        all_permutation_seeds.push_back(cell8);
    }
    std::vector<four::Tetrahedra> tetrahedra_groups;

    for (const auto& seeds : all_permutation_seeds)
    {
        auto permutations = four::combinatorics::generate<float>(seeds);
        std::cout << permutations.size() << " permutations found" << std::endl;

        std::vector<double> coordinates;
        for (const auto& permutation : permutations)
        {
            if (permutation.size() != 4)
            {
                throw std::runtime_error("Permutation does not have the correct number of dimensions");
            }
            coordinates.push_back(permutation[0]);
            coordinates.push_back(permutation[1]);
            coordinates.push_back(permutation[2]);
            coordinates.push_back(permutation[3]);
        }

        // Initialize QHull
        orgQhull::Qhull qhull;

        std::vector<glm::vec4> vertices;
        std::vector<size_t> simplices;
        std::vector<glm::vec4> normals;

        try {
            // Run QHull
            std::cout << "Running convex hull algorithm on " << permutations.size() << " vertices" << std::endl;
            qhull.runQhull("", 4, permutations.size(), coordinates.data(), "Qt");

            // Process unique points that form the convex hull
            for (const auto& point : qhull.points())
            {
                auto coords = point.coordinates();
                assert(point.dimension() == 4);

                vertices.push_back(glm::normalize(glm::vec4{ coords[0], coords[1], coords[2], coords[3] }));
            }

            // Process unique facets that form the convex hull
            std::cout << "Facet count: " << qhull.facetList().count() << std::endl;
            for (auto& face : qhull.facetList())
            {
                if (!face.isSimplicial())
                {
                    throw std::runtime_error("Non-simplical face found");
                }
                for (const auto& vertex : face.vertices())
                {
                    simplices.push_back(vertex.point().id());
                }

                auto hyperplane = face.hyperplane();
                auto center = face.getCenter();
                auto normal = hyperplane.coordinates();
                assert(hyperplane.dimension() == 4);

                normals.push_back(glm::normalize(glm::vec4{
                    round_nearest_tenth(normal[0]),
                    round_nearest_tenth(normal[1]),
                    round_nearest_tenth(normal[2]),
                    round_nearest_tenth(normal[3])
                }));
            }

            std::cout << "Convex hull resulted in:" << std::endl;
            std::cout << vertices.size() << " vertices" << std::endl;
            std::cout << simplices.size() / 4 << " simplices" << std::endl;
            std::cout << normals.size() << " hyperplane normals" << std::endl;
        }
        catch (std::exception e)
        {
            std::cout << e.what() << std::endl;
        }

        tetrahedra_groups.push_back({
            vertices,
            simplices,
            normals
        });
    }
    
    return tetrahedra_groups;
}

int main()
{
    initialize();

    // Some helpful constants
    const glm::vec4 x_axis = { 1.0f, 0.0f, 0.0f, 0.0f };
    const glm::vec4 y_axis = { 0.0f, 1.0f, 0.0f, 0.0f };
    const glm::vec4 z_axis = { 0.0f, 0.0f, 1.0f, 0.0f };
    const glm::vec4 w_axis = { 0.0f, 0.0f, 0.0f, 1.0f };
    const glm::vec4 origin = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float pi = 3.14159265358979f;

    auto tetrahedra_groups = run_qhull();

    // Construct the 4D mesh, slicing hyperplane, 4D camera, etc.
    auto renderer = four::Renderer{};
    for (const auto& tetrahedra : tetrahedra_groups)
    {
        renderer.add_tetrahedra(tetrahedra);
    }

    auto hyperplane = four::Hyperplane{ w_axis, 0.1f };
    auto camera = four::Camera{
        x_axis * 2.0f, // From
        origin, // To
        y_axis, // Up
        z_axis  // Over
    };

    // Load the shader program that will project 4D -> 3D -> 2D
    auto shader_projections = graphics::Shader{ "../shaders/projections.vert", "../shaders/projections.frag" };

    glm::mat4 simple_rotation_matrix = build_simple_rotation_matrix();
    renderer.slice_objects(hyperplane);

    // Uniforms for 4D -> 3D projection.
    shader_projections.use();
    shader_projections.uniform_vec4("u_four_from", camera.get_from());
    shader_projections.uniform_mat4("u_four_model", simple_rotation_matrix); // Rotation matrix in 4D
    shader_projections.uniform_mat4("u_four_view", camera.look_at());
    shader_projections.uniform_mat4("u_four_projection", camera.projection());
    shader_projections.uniform_bool("u_perspective_4D", false);

    // Uniforms for 3D -> 2D projection.
    shader_projections.uniform_mat4("u_three_view", arcball_camera_matrix);
    
    while (!glfwWindowShouldClose(window))
    {
        // Update flag that denotes whether or not the user is interacting with ImGui
        input_data.imgui_active = ImGui::GetIO().WantCaptureMouse;

        // Poll regular GLFW window events
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        bool topology_needs_update = false;

        // Draw the UI elements (buttons, sliders, etc.)
        {
            ImGui::Begin("Settings"); 

            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGui::Separator();
            ImGui::Text("Slicing and Rotations");
            ImGui::SliderInt("Polychoron", (int*)&polychoron_index, 0, renderer.get_number_of_objects() - 1);
            if (ImGui::SliderFloat("Hyperplane Displacement", &hyperplane.displacement, -3.0f, 3.0f)) topology_needs_update = true;
            if (roundf(hyperplane.get_displacement()) == hyperplane.get_displacement())
            {
                hyperplane.displacement += 0.001f;
            }
            if (ImGui::SliderFloat("Rotation XY", &rotation_xy, 0.0f, 2.0f * pi)) topology_needs_update = true;
            if (ImGui::SliderFloat("Rotation YZ", &rotation_yz, 0.0f, 2.0f * pi)) topology_needs_update = true;
            if (ImGui::SliderFloat("Rotation ZX", &rotation_zx, 0.0f, 2.0f * pi)) topology_needs_update = true;
            if (ImGui::SliderFloat("Rotation XW", &rotation_xw, 0.0f, 2.0f * pi)) topology_needs_update = true;
            if (ImGui::SliderFloat("Rotation YW", &rotation_yw, 0.0f, 2.0f * pi)) topology_needs_update = true;
            if (ImGui::SliderFloat("Rotation ZW", &rotation_zw, 0.0f, 2.0f * pi)) topology_needs_update = true;
            if (topology_needs_update)
            {
                // Don't rebuild the rotation matrices unless we have to
                simple_rotation_matrix = build_simple_rotation_matrix();
                shader_projections.uniform_mat4("u_four_model", simple_rotation_matrix);
            }

            ImGui::Checkbox("Display Wireframe", &display_wireframe);

            ImGui::End();
        }
        ImGui::Render();

        glViewport(0, 0, window_w, window_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw the 4D objects
        {
            //if (topology_needs_update)
            for (size_t i = 0; i < renderer.get_number_of_objects(); i++)
            {
                glm::vec4 translation = {
                    (i / static_cast<float>(renderer.get_number_of_objects() - 1.0f)) * 9 - 4.5f,
                    0,
                    0.0,
                     sinf(glfwGetTime() * 0.75f + i / ((float)renderer.get_number_of_objects() + 1.0f)) * 0.5f + 0.5f
                };
                translation.w *= 0.5f;

                auto rotation = four::maths::get_simple_rotation_matrix(four::maths::Plane::XW, cosf(glfwGetTime() * 0.75f + i / ((float)renderer.get_number_of_objects() + 1.0f)));
                renderer.set_transform(i, rotation * glm::mat4{ 0.5f }, translation);
                renderer.slice_object(i, hyperplane);
            }


            if (display_wireframe)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
            else
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            glm::mat4 projection = glm::perspective(glm::radians(zoom), static_cast<float>(window_w) / static_cast<float>(window_h), 0.1f, 1000.0f);

            shader_projections.use();
            shader_projections.uniform_mat4("u_three_model", arcball_model_matrix);
            shader_projections.uniform_mat4("u_three_projection", projection);
           // renderer.draw_sliced_object(polychoron_index);
            renderer.draw_sliced_objects();
        }

        // Draw the ImGui window
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Clean-up imgui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
       
    // Clean-up GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
}

