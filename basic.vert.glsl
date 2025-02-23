#version 330 core

layout(location = 0) in vec2 layout_position;
layout(location = 1) in vec2 layout_texture_coords;
layout(location = 2) in int layout_color;

out vec2 texture_coords;
out vec4 color;

uniform mat4 model = mat4(0);
uniform mat4 projection = mat4(0);

float normalize_rgba_value(int value)
{
    return value * (1.0 / 255.0);
}

vec4 unpack_rgba_color(int color)
{
    vec4 result;

    result.r = normalize_rgba_value((color & 0xFF000000) >> 24);
    result.g = normalize_rgba_value((color & 0x00FF0000) >> 16);
    result.b = normalize_rgba_value((color & 0x0000FF00) >> 8);
    result.a = normalize_rgba_value((color & 0x000000FF) >> 0);

    return result;
}

void main(void)
{
    vec4 position = projection * model * vec4(layout_position, 0.0, 1.0);
    gl_Position = vec4(position.xy, 0.0, 1.0);

    // NOTE: Passing to fragment shader
    color = unpack_rgba_color(layout_color);
    texture_coords = layout_texture_coords;
}
