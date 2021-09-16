#version 430 core
#extension GL_NV_shadow_samplers_cube : enable
out vec4 f_color;

const float PI = 3.14159;

float speed = 1.0f;
float Eta = 0.95f;
float interactiveAmplitude = 0.15f;
float interactiveWavelength = 0.5f;
float interactiveSpeed = 8.0f;
float ratio_of_reflection_and_refraction = 0.5f;

in V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
} f_in;

uniform vec3 u_color;
uniform vec3 camera;

uniform sampler2D u_texture;
uniform sampler2D tiles;

uniform samplerCube skyBox;

vec2 intersectCube(vec3 origin, vec3 ray, vec3 cubeMin, vec3 cubeMax) 
{
	vec3 tMin = (cubeMin - origin) / ray;
	vec3 tMax = (cubeMax - origin) / ray;
	vec3 t1 = min(tMin, tMax);
	vec3 t2 = max(tMin, tMax);
	float tNear = max(max(t1.x, t1.y), t1.z);
	float tFar = min(min(t2.x, t2.y), t2.z);
	return vec2(tNear, tFar);
}

vec3 getWallColor(vec3 point) 
{
	float scale = 0.5f;

	vec3 wallColor;
	vec3 normal;

	if (abs(point.x) > 0.999f) 
    {
		wallColor = texture2D(tiles, point.yz * 0.5f + vec2(1.0f, 0.5f)).rgb;
		normal = vec3(-point.x, 0.0f, 0.0f);
	} 
    else if (abs(point.z) > 0.999f) 
    {
		wallColor = texture2D(tiles, point.yx * 0.5f + vec2(1.0f, 0.5f)).rgb;
		normal = vec3(0.0f, 0.0f, -point.z);
	} 
    else 
    {
		wallColor = texture2D(tiles, point.xz * 0.5f + 0.5f).rgb;
		normal = vec3(0.0f, 1.0f, 0.0f);
	}

	scale /= length(point);
	

	return wallColor * scale;
}

vec3 getSurfaceRayColor(vec3 origin, vec3 ray, vec3 waterColor) 
{
    vec3 color;
    if (ray.y < 0.0) 
    {
        vec2 temp = intersectCube(origin, ray, vec3(-1.0f, -1.0f, -1.0f), vec3(1.0f, 1.0f, 1.0f));
        color = getWallColor(origin + ray * temp.y);
    }
    else
        color = vec3(textureCube(skyBox, ray));
    
    color *= waterColor;
    return color;
  }

void main()
{   
    vec3 normal = normalize(cross(dFdy(f_in.position), dFdx(f_in.position)));
    
    vec3 I = normalize(f_in.position - camera.xyz);
    vec3 reflectionVector = reflect(I, normalize(normal));
    vec3 refractionVector = refract(I, -normalize(normal), Eta);
    
    vec3 reflectionColor = vec3(textureCube(skyBox, reflectionVector));
    vec3 refractionColor = getSurfaceRayColor(vec3(f_in.texture_coordinate.x * 2.0f / 20000 - 1.0f, 0.0f, f_in.texture_coordinate.y * 2.0f / 20000 - 1.0f), refractionVector, vec3(1.0f)) * vec3(0.0f, 0.8f, 1.0f);
	
    if(f_in.normal.y > 0)
		f_color = vec4(mix(reflectionColor, refractionColor, ratio_of_reflection_and_refraction), 1.0f);
	else
		f_color = vec4(refractionColor, 1.0f);
}