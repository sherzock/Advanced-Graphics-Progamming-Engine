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

#ifdef Mode_ForwardShading

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
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oDepth;
layout(location = 4) out vec4 oPosition;

float near = 0.1; 
float far  = 100.0;

float DepthCalc(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

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

	oNormals = vec4(normalize(vNormal), 1.0); 
	oAlbedo = texture(uTexture, vTexCoord);

	float depth = DepthCalc(gl_FragCoord.z) / far; // divide by far for demonstration
	oDepth = vec4(vec3(depth), 1.0);

	oPosition = vec4(vPosition, 1.0);
}

#endif
#endif

//------------------------------------------------------------------------------------------------------
//----------------------------------------Deferred Shading----------------------------------------------
//------------------------------------------------------------------------------------------------------

#ifdef Mode_DeferredGeometry

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
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oAlbedo;
layout(location = 3) out vec4 oDepth;
layout(location = 4) out vec4 oPosition;

float near = 0.1; 
float far  = 100.0;

float DepthCalc(float depth) 
{
    float z = depth * 2.0 - 1.0; 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

void main()
{
	oPosition = vec4(vPosition,1.0);
	oNormals = vec4(normalize(vNormal), 1.0); 
	oAlbedo = texture(uTexture, vTexCoord);

	float depth = DepthCalc(gl_FragCoord.z) / far; 
	oDepth = vec4(vec3(depth), 1.0);

}

#endif
#endif

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

#ifdef Mode_DeferredLighting

struct Light
{
    unsigned int    type;
    vec3            color;
    vec3            direction;
    vec3            position;
};

#if defined(VERTEX)

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

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

void main()
{
    vTexCoord = aTexCoord;
    vPosition = vec3(projection * view * model * vec4(uCameraPosition, 1.0));
    gl_Position =  vec4(aPosition, 1.0);
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

uniform sampler2D oNormals;
uniform sampler2D oAlbedo;
uniform sampler2D oDepth;
uniform sampler2D oPosition;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(location = 0) out vec4 oColor;

void main()
{
	// G buffer
	vec3 position = texture(oPosition, vTexCoord).rgb;
	vec3 Normal = texture(oNormals, vTexCoord).rgb;
	vec3 albedo = texture(oAlbedo, vTexCoord).rgb;
	vec3 viewDir = normalize(vPosition - position);

	// Mat parameters
    vec3 specular = vec3(1.0);	// color reflected by mat
    float shininess = 40.0;		// how strong specular reflections are (more shininess harder and smaller spec)

	// Ambient
    float ambientIntensity = 0.5;
    vec3 ambientColor = albedo.xyz * ambientIntensity;

    vec3 N = normalize(Normal);		// normal
	vec3 V = normalize(viewDir);	// direction from pixel to camera

	vec3 diffuseColor;
	vec3 specularColor;
	
	for(int i = 0; i < uLightCount; ++i)
	{
	    float attenuation = 0.3f;
		vec3 L = vec3(0.);

		 //If it is a point light, attenuate according to distance
		if(uLight[i].type == 1)
		{
			attenuation = 2.0 / length(uLight[i].position - position);
			L = normalize(uLight[i].position - position);  //Light direction

		}
		else
		{
			L = normalize(uLight[i].direction);  //Light direction
		}
	        
		


	    vec3 R = reflect(-L, N);  //reflected vector
	    
	     //Diffuse
	    float diffuseIntensity = max(0.0, dot(N, L));
	    diffuseColor += attenuation * albedo.xyz * uLight[i].color * diffuseIntensity;
	    
	     //Specular
	    float specularIntensity = pow(max(dot(R, V), 0.0), shininess);
	    specularColor += attenuation * specular * uLight[i].color * specularIntensity;
	}

	oColor = vec4(ambientColor + diffuseColor + specularColor, 1.0);

}

#endif
#endif