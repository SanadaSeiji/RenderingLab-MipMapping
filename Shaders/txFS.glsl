#version 330

//Toon following example on Lighthouse
//http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/toon-shading-version-i/

in vec3 position_eye, normal_eye;

in vec2 tex_coordi;

uniform mat4 view;
uniform sampler2D tex;

// fixed point light properties

vec3 light_position_world  = vec3 (0.0, 5.0, 20.0);

vec3 Ls = vec3 (0.4, 0.4, 0.4); // specular colour: light reflects of surface, directly in eye

vec3 Ld = vec3 (0.6, 0.6, 0.6); // dull diffuse light colour:roughness of surface

vec3 La = vec3 (0.2, 0.2, 0.2); //  ambient colour:background light, reflections hitting from other objs

  

// surface reflectance

vec3 Ks = vec3 (0.0, 0.5, 1.0); // fully reflect specular light; if rough surface this value be low 

vec3 Kd = vec3 (0.0, 0.0, 1.0); // blue diffuse surface reflectance, unique base colour

vec3 Ka = vec3 (0.2, 0.2, 0.2); // fully reflect ambient light, later would be complicated

float specular_exponent = 100.0; // specular 'power', size of highlight spot larger if this value is 100.0



out vec4 fragment_colour; // final colour of surface

// Does not take into account GL_TEXTURE_MIN_LOD/GL_TEXTURE_MAX_LOD/GL_TEXTURE_LOD_BIAS,
// nor implementation-specific flexibility allowed by OpenGL spec
// code following: https://stackoverflow.com/questions/24388346/how-to-access-automatic-mipmap-level-in-glsl-fragment-shader-texture
float mip_map_level(in vec2 texture_coordinate) // in texel units
{
    vec2  dx_vtc        = dFdx(texture_coordinate);
    vec2  dy_vtc        = dFdy(texture_coordinate);
    float delta_max_sqr = max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc));
    float mml = 0.5 * log2(delta_max_sqr);
    return max( 0, mml ); // Thanks @Nims
}


void main () {

       //color from texture
       vec4 texel = texture (tex, tex_coordi);
       vec3 texC = texel.xyz;

	// ambient intensity, unchanged

	vec3 Ia = La * Ka;



	// diffuse intensity

	// raise light position to eye space

	vec3 light_position_eye = vec3 (view * vec4 (light_position_world, 1.0));

	vec3 distance_to_light_eye = light_position_eye - position_eye;

	vec3 direction_to_light_eye = normalize (distance_to_light_eye);

	float dot_prod = dot (direction_to_light_eye, normal_eye); //give surface a direction from surface to light, compare and produce the factor needed

	dot_prod = max (dot_prod, 0.0);//0.0 to avoid negative dot

	vec3 Id = texC * dot_prod; // final diffuse intensity

	

	// specular intensity
        // again, compare angle between viewer and surface

	vec3 surface_to_viewer_eye = normalize (-position_eye); //camera at origin,everything raised to eye_space



	// blinn

	vec3 half_way_eye = normalize (surface_to_viewer_eye + direction_to_light_eye); //halfway!=reflection_eye...

	float dot_prod_specular = max (dot (half_way_eye, normal_eye), 0.0);

	float specular_factor = pow (dot_prod_specular, specular_exponent);//power, size of highlight

	

	vec3 Is = texC * specular_factor; // final specular intensity

	

	// final colour
       
    // using opengl own function to calculate mip map level, suitable for version 410   
    //float mipmapLevel = textureQueryLod(tex, tex_coordi).x;
	//fragment_colour = vec4 (Is + Id + Ia, texel.a);
    
	
	// convert normalized texture coordinates to texel units before calling mip_map_level
	//using own function to calculate mip map level, suitable for version 330
    float mipmapLevel = mip_map_level(tex_coordi * textureSize(tex, 0));
   
    fragment_colour = textureLod(tex, tex_coordi, mipmapLevel);

}