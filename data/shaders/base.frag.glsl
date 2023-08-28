uniform sampler2D textureUnit;
uniform int withTexture;

in vec4 v_color;
out vec4 fragColor;

in vec2 v_uv;

void main()
{
    if (withTexture == 1) {
        fragColor = texture( textureUnit, v_uv ) * v_color;
    } else {
        fragColor = v_color;
    }
}
