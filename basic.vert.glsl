#version 330 core

layout(location = 0) in vec2 layout_position;
// layout(location = 1) in int layout_color;

out vec4 color;

// uniform mat4 model = mat4(0);
// uniform mat4 projection = mat4(0);

void
main(void)
{
    // vec4 position = projection * model * vec4(layout_position, 0.0, 1.0);
    vec4 position = vec4(layout_position, 0.0, 1.0);
    gl_Position = vec4(position.xy, 0.0, 1.0);

    color = vec4(1, 1, 1, 1); // layoutColor
}
