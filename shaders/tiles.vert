#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture_coordinate;

uniform mat4 u_model;
uniform vec4 plane;

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 u_projection;
    mat4 u_view;
};

out V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
} v_out;

out vec3 vs_normal;

void main()
{
    gl_Position = u_projection * u_view * u_model * vec4(position, 1.0f);

    vec4 worldPos = u_model * vec4(position, 1.0f);

    gl_ClipDistance[0] = dot(worldPos, plane);

    v_out.position = vec3(u_model * vec4(position, 1.0f));
    v_out.normal = mat3(transpose(inverse(u_model))) * normal;
    v_out.texture_coordinate = vec2(texture_coordinate.x, 1.0f - texture_coordinate.y);

    vs_normal = mat3(u_view) * mat3(u_model) * normal;
}