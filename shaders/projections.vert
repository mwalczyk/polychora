#version 450

#define pi 3.1415926535897932384626433832795

uniform float u_time;

// We have this here in order to avoid 5x5 matrices (technically, the would
// be the last column of this transformation matrix)
uniform vec4 u_four_from;

uniform vec4 u_four_model_translation;
uniform mat4 u_four_model_orientation;
uniform mat4 u_four_view;
uniform mat4 u_four_projection;

uniform mat4 u_three_model;
uniform mat4 u_three_view;
uniform mat4 u_three_projection;

uniform bool u_perspective_4D = false;
uniform bool u_perspective_3D = true;

uniform float u_clip_distance_w = 0.0;

layout(location = 0) in vec4 i_position;
layout(location = 1) in vec4 i_color;

out VS_OUT
{
    vec4 color;
    vec3 position;
} vs_out;

// https://github.com/hughsk/glsl-hsv2rgb/blob/master/index.glsl
vec3 hsv_to_rgb(vec3 c)
{
    vec4 k = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + k.xyz) * 6.0 - k.www);
    return c.z * mix(k.xxx, clamp(p - k.xxx, 0.0, 1.0), c.y);
}

vec4 quaternion_mult(in vec4 q, in vec4 r)
{
    return vec4(r.x*q.x-r.y*q.y-r.z*q.z-r.w*q.w,
            r.x*q.y+r.y*q.x-r.z*q.w+r.w*q.z,
            r.x*q.z+r.y*q.w+r.z*q.x-r.w*q.y,
            r.x*q.w-r.y*q.z+r.z*q.y+r.w*q.x);
}

vec3 point_rotation_by_quaternion(in vec3 point, in vec4 q)
{
    vec4 r = vec4(0.0, point.x, point.y, point.z);
    vec4 q_conj = vec4(q.x, -1*q.y, -1*q.z, -1*q.w);

    vec4 result = quaternion_mult(quaternion_mult(q, r), q_conj);//[1:]

    return vec3(result.y, result.z, result.w);
}

float sigmoid(float x)
{
    return 1.0 / (1.0 + exp(-x));
}

void main()
{
    vec4 four;
    vec3 color;

    // Custom 4-dimensional clipping plane: useful for revealing different
    // "layers" of the 4D object
    if (i_position.w > u_clip_distance_w)
    {
        gl_ClipDistance[0] = -1.0;
    }
    else 
    {
        gl_ClipDistance[0] = 1.0;
    }

    // Project 4D -> 3D with a perspective projection
    if (u_perspective_4D)
    {
        four = u_four_model_orientation * i_position;
        four = four + u_four_model_translation;
        four = four - u_four_from;
        four = u_four_view * four;
        
        // This is sometimes interesting too...
        //four = vec4(four.xyz, 1.0);

        four = u_four_projection * four;
        four /= four.w;

        float depth_cue = i_position.w * 0.5 + 0.5;

        color = hsv_to_rgb(vec3(depth_cue * 0.5, 1.0, 1.0));
    }
    // Project 4D -> 3D with a parallel (orthographic) projection
    else
    {
        // TODO: the code above doesn't always work, since the `u_four_model` matrix (rotations)
        // TODO: is already applied to the tetrahedra vertices in the compute shader prior to
        // TODO: generating the 3D slice - we do a standard orthographic projection for this
        // TODO: draw mode, instead
        four = vec4(i_position.xyz, 1.0);

        vec3 rotation = point_rotation_by_quaternion(vec3(1.0, 0.0, 0.0), i_color);
        vec3 rotation_color = normalize(rotation) * 0.5 + 0.5;
        
        color = rotation_color;
        
        vec3 cell_centroid = i_color.rgb;
        vec3 centroid_color = normalize(cell_centroid) * 0.5 + 0.5;

        color = centroid_color;
    }

    vec4 three;

    // Project 3D -> 2D with a perspective projection
    three = u_three_projection * u_three_view * u_three_model * four;

    gl_Position = three;

    // Alpha based on depth in the 4th dimension
    float alpha = u_perspective_4D ? i_position.w * 0.5 + 0.5 : 1.0;

    // Pass values to fragment shader
    vs_out.color = vec4(color, alpha);
    vs_out.position = four.xyz;
}