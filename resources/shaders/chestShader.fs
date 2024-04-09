#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture1;

void main()
{
    vec4 texColor = texture(texture1, TexCoords);
    vec3 darkerColor = vec3(1, 1, 1); // You can adjust these values to control the darkness
    vec3 finalColor = texColor.rgb * darkerColor;
    FragColor = vec4(finalColor, texColor.a);

    }