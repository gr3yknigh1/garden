#version 330 core

out vec4 FragColor;

in vec4 color;
in vec2 texture_coords;

uniform sampler2D u_texture;

void
main(void)
{
    FragColor = texture(u_texture, texture_coords) * color;
}
