//-------------------------------------------------------------------------
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX)

// TODO: Write your vertex shader here

layout(location = 0) in vec3 aPosition;
//layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vec3 aBitangent;

out vec2 vTexCoord;

void main()
{
    vTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) //------------------------------------------

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
    oColor = texture(uTexture, vTexCoord);
}

#endif
#endif

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

#ifdef SHOW_TEXTURED_MESH

struct Light
{
    unsigned int    type;
    vec3            color;
    vec3            direction;
    vec3            position;
};

#if defined(VERTEX)

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vec3 aBitangent;

layout(binding = 0, std140) uniform GlobalParams
{
    vec3            uCameraPosition;
    unsigned int    uLightCount;
    Light           uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
    mat4        model;
    mat4        view;
    mat4        projection;
};

out vec2 vTexCoord;
out vec3 vPosition; // In World space
out vec3 vNormal;   // In World space
out vec3 vViewDir;  // In World space

void main()
{
    vTexCoord = aTexCoord;
    vPosition = vec3(model* vec4(aPosition, 1.0));
    vNormal = vec3(model * vec4(aNormal, 0.0));
    vViewDir = uCameraPosition - vPosition;
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) //------------------------------------------

struct Light
{
	unsigned int type;
	vec3 color;
	vec3 direction;
	vec3 position;
};

in vec2 vTexCoord;
in vec3 vPosition; // in worldspace
in vec3 vNormal; // in worldspace
in vec3 uViewDir; // in worldspace

uniform sampler2D uTexture;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(location = 0) out vec4 oColor;


void main()
{
    	// Mat parameters
    vec3 specular = vec3(1.0);	// color reflected by mat
    float shininess = 1.0;		// how strong specular reflections are (more shininess harder and smaller spec)
	vec4 albedo = texture(uTexture, vTexCoord);

	// Ambient
    float ambientIntensity = 0.5;
    vec3 ambientColor = albedo.xyz * ambientIntensity;

    vec3 N = normalize(vNormal);		// normal
	vec3 V = normalize(-uViewDir.xyz);	// direction from pixel to camera

	vec3 diffuseColor;
	vec3 specularColor;

	for(int i = 0; i < uLightCount; ++i)
	{
	    float attenuation = 0.3f;
		
		// If it is a point light, attenuate according to distance
		if(uLight[i].type == 1)
			attenuation = 2.0 / length(uLight[i].position - vPosition);
	        
	    vec3 L = normalize(uLight[i].direction - uViewDir.xyz); // Light direction 
	    vec3 R = reflect(-L, N);								// reflected vector
	    
	    // Diffuse
	    float diffuseIntensity = max(0.0, dot(N, L));
	    diffuseColor += attenuation * albedo.xyz * uLight[i].color * diffuseIntensity;
	    
	    // Specular
	    float specularIntensity = pow(max(dot(R, V), 0.0), shininess);
	    specularColor += attenuation * specular * uLight[i].color * specularIntensity;
	}

	oColor = vec4(ambientColor + diffuseColor + specularColor, 1.0);
}

#endif
#endif