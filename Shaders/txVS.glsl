#version 410



layout (location = 0) in vec3 vertex_position;

layout (location = 1) in vec3 vertex_normals;

layout (location = 2) in vec2 uv;



//light need to be able to move/change colour
uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

out vec3 position_eye, normal_eye;
out vec2 tex_coordi;



void main () {

//raising everything to eye_space
//later useful in fragment shader
	position_eye = vec3 (view * model * vec4 (vertex_position, 1.0));

	normal_eye = vec3 (view * model * vec4 (vertex_normals, 0.0));

        
        tex_coordi = uv;

	gl_Position = proj * vec4 (position_eye, 1.0);

}