out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec4 objectColor;

void main()
{
    vec4 theColor = objectColor;
    if (theColor.r < 0.0) {
        theColor = Color;
    }

    // ambient
    float ambientStrength = 0.0;
    vec3 ambient = ambientStrength * lightColor;

    // diffuse
    vec3 materialDiffuse = vec3(0.5,0.5,0.5);
    if (lightPos.x < 0.0) {
        materialDiffuse = vec3(0.8,0.8,0.8);
    }
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = abs(dot(norm, lightDir));
    vec3 diffuse = diff * lightColor * theColor.xyz;

    // specular
    vec3 specular = vec3(0.0,0.0,0.0);
    if (lightPos.x < 0.0) {
        float materialShininess = 10.0;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(reflectDir, viewDir), 0.0), materialShininess);
        specular = spec * lightColor;
    } else {
        ambient = vec3(0.2,0.2,0.2);
    }

    FragColor = vec4((ambient + diffuse + specular)*0.8, theColor.a);
}