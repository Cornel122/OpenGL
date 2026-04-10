#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

uniform vec3 lightDir;
uniform vec3 lightColor;

struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight pointLight;

uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

float ambientStrength = 0.2f;
float specularStrength = 0.5f;
float shininess = 32.0f;

float computeFog()
{
    float fogDensity = 0.15f;
    float distance = length(fPosEye.xyz);
    return clamp(exp(-fogDensity * distance), 0.0, 1.0);
}

float computeShadow(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = 0.005;
    return (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    vec3 ambientComp = ambientStrength * light.color * attenuation;
    vec3 diffuseComp = diff * light.color * attenuation;
    vec3 specularComp = specularStrength * spec * light.color * attenuation;

    return ambientComp + diffuseComp + specularComp;
}

void main()
{
    vec4 texColor = texture(diffuseTexture, fTexCoords);
    if(texColor.a <= 0.00001) discard;

    vec3 normal = normalize(fNormal);
    vec3 viewDir = normalize(-fPosEye.xyz); 
    vec3 lightDirN = normalize(lightDir);
    float diff = max(dot(normal, lightDirN), 0.0);
    vec3 reflectDir = reflect(-lightDirN, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    vec3 ambientComp = ambientStrength * lightColor;
    vec3 diffuseComp = diff * lightColor;
    vec3 specularComp = specularStrength * spec * lightColor;

    float shadow = computeShadow(fragPosLightSpace);
    vec3 dirLight = ambientComp + (1.0 - shadow) * (diffuseComp + specularComp);
    dirLight *= texColor.rgb;
    specularComp *= texture(specularTexture, fTexCoords).rgb;

    vec3 pointLightColor = CalcPointLight(pointLight, normal, fPosEye.xyz, viewDir);
    pointLightColor *= texColor.rgb;

    vec3 finalColor = dirLight + pointLightColor;
    finalColor = min(finalColor, vec3(1.0));

    float fogFactor = computeFog();
    vec4 fogColor = vec4(0.5,0.5,0.5,1.0);

    fColor = mix(fogColor, vec4(finalColor, 1.0), fogFactor);
}
	