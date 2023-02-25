#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;
in vec3 colour;
//in vec2 passTexture;
in vec4 fPosEye;

out vec4 fColor;

//matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 lightPosColor;

uniform int fogCond;
uniform int nightFlag;
vec3 nightColor = vec3(0.0f, 0.0f, 0.0f); //lumina neagra

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

//import the shadow map
uniform sampler2D shadowMap;

//components
vec3 ambient;
vec3 ambientPos;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 diffusePos;
vec3 specular;
vec3 specularPos;
float specularStrength = 0.5f;

//lumini punctiforme
float constant = 1.0f;
float linear = 0.0022f;
float quadratic = 0.0020f;

//shadow
float shadow = 0.0f;


void computeDirLight(vec3 colorDir) //lumina directionala
{  
   
   //viewer is situated at the origin
   vec3 cameraPosEye = vec3(0.0f);    

    //compute eye space coordinates
    //vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(fNormal);

    //normalize light direction
    //vec3 lightDirN = vec3(normalize(view * vec4(lightDir, 0.0f)));
    vec3 lightDirN = normalize(lightDir);

    //compute view direction (in eye coordinates, the viewer is situated at the origin
    vec3 viewDir = normalize(cameraPosEye - fPosEye.xyz);

    //compute half vector
    vec3 halfVector = normalize(lightDirN + viewDir);
    


    //compute ambient light
    //ambient = ambientStrength * lightColor;
    ambient = ambientStrength * colorDir;

    //compute diffuse light
    //diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * colorDir;

    //compute specular light
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), 32.0f);
    //specular = specularStrength * specCoeff * lightColor;
    specular = specularStrength * specCoeff * colorDir;
}

void computePosLight() //lumina punctiforma
{
   //viewer is situated at the origin
   //vec3 cameraPosEye = vec3(0.0f);    

    //compute eye space coordinates
    vec4 fPosEye = vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(fNormal);

    //normalize lightPosEye
    vec3 lightPosEye = vec3(normalize(view * vec4(lightPos, 0.0f))); 

    //compute light pos
    vec3 lightDirN = normalize(lightPosEye - fPosEye.xyz);

    //compute view direction (in eye coordinates, the viewer is situated at the origin)
    vec3 viewDir = normalize(-fPosEye.xyz);

    //compute half vector
    vec3 halfVector = normalize(lightDirN + viewDir);

    //compute specular light
    float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), 32.0f);
    
    //compute distance to light
    float dist = length(lightPosEye - fPosEye.xyz);
    //compute attenuation
    float att = 1.0f / (constant + linear * dist + quadratic * (dist * dist));

    //compute ambient light
    ambientPos = att * ambientStrength * lightPosColor;
    //compute diffuse light
    diffusePos = att * max(dot(normalEye, lightDirN), 0.0f) * lightPosColor;
    specularPos = att * specularStrength * specCoeff * lightPosColor;
}

float computeShadow() {
	//perform perspective divide
	//return position of the current fragment in [-1,1]
	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	//transform to [0,1] range
	normalizedCoords = normalizedCoords * 0.5 + 0.5;
	if (normalizedCoords.z > 1.0f) {
		return 0.0f;
	}

	// Get closest depth value from light's perspective
	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	float currentDepth = normalizedCoords.z;

	// Check whether current frag pos is in shadow
	float bias = 0.005f;
	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;

	//float shadow = currentDepth > closestDepth ? 1.0 : 0.0;

	return shadow;
}


float computeFog(){
	float fogDensity = 0.05f;
 	float fragmentDistance = length(fPosEye);
 	float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

 	return clamp(fogFactor, 0.0f, 1.0f);
}



void main() 
{
    if(nightFlag == 1)
    	computeDirLight(lightColor);
    else
	computeDirLight(nightColor);

    computePosLight();
    
    //umbre
    shadow = computeShadow();

    //ceata
    float fogFactor = computeFog();
    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);

    //compute point color
    vec3 colorPos = min((ambientPos + diffusePos) * texture(diffuseTexture, fTexCoords).rgb + specularPos * texture(specularTexture, fTexCoords).rgb, 1.0f);

    //compute final vertex color
    vec3 color = min((ambient + (1.0f - shadow) * diffuse) * texture(diffuseTexture, fTexCoords).rgb + (1.0f - shadow) * specular * texture(specularTexture, fTexCoords).rgb, 1.0f) + colorPos;

    if (fogCond == 0) 
    	fColor = fogColor *(1 - fogFactor) + vec4(color, 0.0f) * fogFactor;
    else
	fColor = vec4(color, 1.0f);

    //fColor = texture(diffuseTexture, fTexTexture);
}
