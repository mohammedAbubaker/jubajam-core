#version 430 core

// The final color output of the shader
out vec4 FragColor;

// Inputs coming from the vertex shader
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Uniforms sent from the C++ application
uniform vec3 viewPos; // The camera's position in world space
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{
    // --- Lighting Properties ---
    vec3 lightPos = viewPos; // Place a simple light source at the camera's position
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    // --- Ambient Lighting ---
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
  	
    // --- Diffuse Lighting ---
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // --- Specular Lighting ---
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0); // 32 is the shininess coefficient
    
    // Use the specular map for the color of the highlight
    vec3 specularColor = texture(texture_specular1, TexCoords).rgb;
    vec3 specular = specularStrength * spec * specularColor;
    
    // --- Combine Results ---
    // Get the base color from the diffuse texture
    vec3 objectColor = texture(texture_diffuse1, TexCoords).rgb;

    // Combine all lighting components with the object's texture color
    vec3 result = (ambient + diffuse) * objectColor + specular;
    FragColor = vec4(result, 1.0);
}
