#include "stdafx.h"
#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
//Some Windows Headers (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include "maths_funcs.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdlib.h>

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint shaderTx;
GLuint shaderNx;
GLuint tex;
GLuint texFloor;

bool isMIP = TRUE;

unsigned int vao = 0;
int width = 1200;
int height = 900;

int pointCount = 0;


//parent teapot answers to keyboard event
mat4 local1T = identity_mat4();
mat4 local1R = identity_mat4();
mat4 local1S = identity_mat4();
//left
GLfloat rotatez = 0.0f; //rotation of child teapot around self(left)
mat4 R = identity_mat4();
mat4 camT = identity_mat4();


float roDeg = 1.0;


bool key_x_pressed = false;
bool key_z_pressed = false;
bool key_c_pressed = false;
bool key_v_pressed = false;
bool key_b_pressed = false;
bool key_n_pressed = false;
bool key_d_pressed = false;
bool key_f_pressed = false;
bool key_g_pressed = false;
bool key_h_pressed = false;
bool key_s_pressed = false;
bool key_a_pressed = false;
bool key_w_pressed = false;
bool key_q_pressed = false;




#pragma region TEXTURE_FUNCTIONS
bool load_texture(const char *file_name, GLuint *tex) {
	int x, y, n;
	int force_channels = 4;
	unsigned char *image_data = stbi_load(file_name, &x, &y, &n, force_channels);
	if (!image_data) {
		fprintf(stderr, "ERROR: could not load %s\n", file_name);
		return false;
	}
	// NPOT check
	if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
		fprintf(stderr, "WARNING: texture %s is not power-of-2 dimensions\n",
			file_name);
	}
	int width_in_bytes = x * 4;
	unsigned char *top = NULL;
	unsigned char *bottom = NULL;
	unsigned char temp = 0;
	int half_height = y / 2;

	for (int row = 0; row < half_height; row++) {
		top = image_data + row * width_in_bytes;
		bottom = image_data + (y - row - 1) * width_in_bytes;
		for (int col = 0; col < width_in_bytes; col++) {
			temp = *top;
			*top = *bottom;
			*bottom = temp;
			top++;
			bottom++;
		}
	}
	glGenTextures(1, tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		image_data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//mip mapping
	if (isMIP == TRUE) {
		// code follows: http://www.opengl-tutorial.org/beginners-tutorials/tutorial-5-a-textured-cube/
		//When MAGnifying the image (no bigger mipmap available), use LINEAR filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// When MINifying the image, use a LINEAR blend of two mipmaps, each filtered LINEARLY
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		//without mipmap, both using nearest mapping
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}

	/*
	//aniso
	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);
	// set the maximum!
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, max_aniso);
	*/

	return true;
}
#pragma endregion TEXTURE_FUNCTIONS


// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS

// Create a NULL-terminated string by reading the provided file
char* readShaderSource(const char* shaderFile) {
	FILE* fp = fopen(shaderFile, "rb"); //!->Why does binary flag "RB" work and not "R"... wierd msvc thing?

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders(GLuint shaderProgramID, const char *vs, const char *fs)
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, vs, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, fs, GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { 0 };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	glUseProgram(shaderProgramID);
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS


#pragma region VBO_FUNCTIONS
// VBO Functions - following Anton's code
// https://github.com/capnramses/antons_opengl_tutorials_book/tree/master/20_normal_mapping


int generateObjectBufferTeapot(const char *file_name) {

	int point_count = 0;
	float *points = NULL;
	float *normals = NULL;
	float *texcoords = NULL;
	float *vtans = NULL;

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(file_name,
		aiProcess_Triangulate | aiProcess_CalcTangentSpace);

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return 0;
	}
	fprintf(stderr, "reading mesh");
	printf("  %i animations\n", scene->mNumAnimations);
	printf("  %i cameras\n", scene->mNumCameras);
	printf("  %i lights\n", scene->mNumLights);
	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	/* get first mesh in file only */
	const aiMesh *mesh = scene->mMeshes[0];
	printf("    %i vertices in mesh[0]\n", mesh->mNumVertices);

	/* pass back number of vertex points in mesh */
	point_count = mesh->mNumVertices;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);


	if (mesh->HasPositions()) {

		points = (float *)malloc(point_count * 3 * sizeof(float));
		for (int i = 0; i < point_count; i++) {
			const aiVector3D *vp = &(mesh->mVertices[i]);
			points[i * 3] = (float)vp->x;
			points[i * 3 + 1] = (float)vp->y;
			points[i * 3 + 2] = (float)vp->z;
		}

		GLuint vp_vbo = 0;
		glGenBuffers(1, &vp_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vp_vbo);
		glBufferData(GL_ARRAY_BUFFER, 3 * point_count * sizeof(float), points, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		free(points);

	}
	if (mesh->HasNormals()) {
		normals = (float *)malloc(point_count * 3 * sizeof(float));
		for (int i = 0; i < point_count; i++) {
			const aiVector3D *vn = &(mesh->mNormals[i]);
			normals[i * 3] = (float)vn->x;
			normals[i * 3 + 1] = (float)vn->y;
			normals[i * 3 + 2] = (float)vn->z;
		}

		GLuint vn_vbo = 0;
		glGenBuffers(1, &vn_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vn_vbo);
		glBufferData(GL_ARRAY_BUFFER, 3 * point_count * sizeof(float), normals, GL_STATIC_DRAW);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

		free(normals);
	}
	if (mesh->HasTextureCoords(0)) {
		printf("loading uv \n");
		texcoords = (float *)malloc(point_count * 2 * sizeof(float));
		for (int i = 0; i < point_count; i++) {
			const aiVector3D *vt = &(mesh->mTextureCoords[0][i]);
			texcoords[i * 2] = (float)vt->x;
			texcoords[i * 2 + 1] = (float)vt->y;
		}

		GLuint vt_vbo = 0;
		glGenBuffers(1, &vt_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vt_vbo);
		glBufferData(GL_ARRAY_BUFFER, 2 * point_count * sizeof(float), texcoords, GL_STATIC_DRAW);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		free(texcoords);
	}

	if (mesh->HasTangentsAndBitangents()) {
		printf("loading tg \n");
		vtans = (float *)malloc(point_count * 4 * sizeof(float));
		for (int i = 0; i < point_count; i++) {
			const aiVector3D *tangent = &(mesh->mTangents[i]);
			const aiVector3D *bitangent = &(mesh->mBitangents[i]);
			const aiVector3D *normal = &(mesh->mNormals[i]);

			// put the three vectors into my vec3 struct format for doing maths
			vec3 t(tangent->x, tangent->y, tangent->z);
			vec3 n(normal->x, normal->y, normal->z);
			vec3 b(bitangent->x, bitangent->y, bitangent->z);
			// orthogonalise and normalise the tangent so we can use it in something
			// approximating a T,N,B inverse matrix
			vec3 t_i = normalise(t - n * dot(n, t));

			// get determinant of T,B,N 3x3 matrix by dot*cross method
			float det = (dot(cross(n, t), b));
			if (det < 0.0f) {
				det = -1.0f;
			}
			else {
				det = 1.0f;
			}

			// push back 4d vector for inverse tangent with determinant
			vtans[i * 4] = t_i.v[0];
			vtans[i * 4 + 1] = t_i.v[1];
			vtans[i * 4 + 2] = t_i.v[2];
			vtans[i * 4 + 3] = det;

		}

		GLuint tangents_vbo = 0;
		glGenBuffers(1, &tangents_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tangents_vbo);
		glBufferData(GL_ARRAY_BUFFER, 4 * point_count * sizeof(GLfloat), vtans,
			GL_STATIC_DRAW);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, NULL);
		glEnableVertexAttribArray(3);


		free(vtans);
	}

	return point_count;

}


#pragma endregion VBO_FUNCTIONS


void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	//top middle: Green teapot

	pointCount = generateObjectBufferTeapot("../Meshs/C.obj");
	glUseProgram(shaderTx);
	//add in textures
	//load_texture("../Textures/mip3.gif", &tex);
	//load_texture("../Textures/brickwall.jpg", &tex);
	//load_texture("../Textures/yarn.jpg", &tex);
	load_texture("../Textures/mip1.jpg", &tex);
	//load_texture("../Textures/mip2.jpg", &tex);
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK);		// cull back face
	glFrontFace(GL_CCW);		// GL_CCW for counter clock-wise
								//load_texture("../Textures/brickwall_normal.jpg", &bumpTex);
	int matrix_location;
	int view_mat_location;
	int proj_mat_location;
	int tex_location;

	if (isMIP == TRUE) {
		//Declare your uniform variables that will be used in your shader
		matrix_location = glGetUniformLocation(shaderTx, "model");
		view_mat_location = glGetUniformLocation(shaderTx, "view");
		proj_mat_location = glGetUniformLocation(shaderTx, "proj");
		tex_location = glGetUniformLocation(shaderTx, "tex");

	}
	else {

		matrix_location = glGetUniformLocation(shaderNx, "model");
		view_mat_location = glGetUniformLocation(shaderNx, "view");
		proj_mat_location = glGetUniformLocation(shaderNx, "proj");
		tex_location = glGetUniformLocation(shaderNx, "tex");

	}


	glUniform1i(tex_location, 0); //must be 0!!


								  //transform
	mat4 view = identity_mat4();

	view = translate(view, vec3(0.0, 0.0, -8.0));

	mat4 persp_proj = perspective(45.0, (float)width / (float)height, 0.1, 100.0);
	mat4 local = identity_mat4();
	local = scale(local, vec3(14.0, 4.0, 0.2));

	local = rotate_y_deg(local, 75.0);


	//local = rotate_z_deg(local, rotatez); //  rotate ball itself

	//cam zooming
	if (key_q_pressed == true) {
		//local1T is global var, won't reset every time when display() called
		camT = translate(camT, vec3(0.0f, 0.0, 0.05));
	}
	if (key_w_pressed == true) {
		camT = translate(camT, vec3(0.0f, 0.0, -0.05));
	}

	view = camT * view;




	//keyboard
	//rotation
	//answer keyboard event now out of same reason
	if (key_a_pressed == true) {

		R = rotate_y_deg(R, roDeg);
	}
	if (key_s_pressed == true) {

		R = rotate_y_deg(R, -roDeg);
	}
	if (key_d_pressed == true) {

		R = rotate_x_deg(R, roDeg);
	}
	if (key_f_pressed == true) {
		R = rotate_x_deg(R, -roDeg);
	}
	if (key_g_pressed == true) {
		R = rotate_z_deg(R, roDeg);
	}
	if (key_h_pressed == true) {
		R = rotate_z_deg(R, -roDeg);
	}

	local = R * local;




	// gloabal is the model matrix
	// for the root, we orient it in global space
	mat4 global = local;

	// update uniforms & draw
	glViewport(0, 0, width, height);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global.m);
	glDrawArrays(GL_TRIANGLES, 0, pointCount);

	//-------parallel wall---------------------------------
	mat4 local2 = local;
	local2 = rotate_y_deg(local2, 190.0);
	local2 = translate(local2, vec3(2.0, 0.0, 0.0));
	mat4 global2 = local2;
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, global2.m);
	glDrawArrays(GL_TRIANGLES, 0, pointCount);

	//swap buffer for all 
	glutSwapBuffers();
}



void updateScene() {

	// Placeholder code, if you want to work with framerate
	// Wait until at least 16ms passed since start of last frame (Effectively caps framerate at ~60fps)
	static DWORD  last_time = 0;
	DWORD  curr_time = timeGetTime();
	float  delta = (curr_time - last_time) * 0.001f;
	if (delta > 0.03f)
		delta = 0.03f;
	last_time = curr_time;

	// 0.2f as unit, rotate every frame
	rotatez += .5f;
	// Draw the next frame
	glutPostRedisplay();
}


void init()
{
	// Set up the shaders
	shaderTx = CompileShaders(shaderTx, "../Shaders/txVS.glsl", "../Shaders/txFS.glsl");


}

#pragma region KEYBOARD
// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {

	if (key == 'x') {
		//Translate the base, etc.
		key_x_pressed = true;
		display();
	}

	if (key == 'z') {
		//Translate the base, etc.
		key_z_pressed = true;
		display();
	}

	if (key == 'c') {
		//Rotate the base, etc.
		key_c_pressed = true;
		display();
	}

	if (key == 'v') {
		//Rotate the base, etc.
		key_v_pressed = true;
		display();
	}

	if (key == 'b') {
		//Scale the base, etc.
		key_b_pressed = true;
		display();
	}

	if (key == 'n') {
		//Scale the base, etc.
		key_n_pressed = true;
		display();
	}



	if (key == 's') {
		//Rotate the base, etc.
		key_s_pressed = true;
		display();
	}

	if (key == 'a') {
		//Rotate the base, etc.
		key_a_pressed = true;
		display();
	}

	if (key == 'h') {
		//Rotate the base, etc.
		key_h_pressed = true;
		display();
	}

	if (key == 'd') {
		//Rotate the base, etc.
		key_d_pressed = true;
		display();
	}

	if (key == 'f') {
		//Scale the base, etc.
		key_f_pressed = true;
		display();
	}

	if (key == 'g') {
		//Scale the base, etc.
		key_g_pressed = true;
		display();
	}



	if (key == 'w') {
		//Scale the base, etc.
		key_w_pressed = true;
		display();
	}

	if (key == 'q') {
		//Scale the base, etc.
		key_q_pressed = true;
		display();
	}

}

void keyUp(unsigned char key, int x, int y) {
	if (key == 'x') {
		key_x_pressed = false;
		display();
	}

	if (key == 'z') {
		//Translate the base, etc.
		key_z_pressed = false;
		display();
	}

	if (key == 'c') {
		key_c_pressed = false;
		display();
	}

	if (key == 'v') {
		//Translate the base, etc.
		key_v_pressed = false;
		display();
	}

	if (key == 'b') {
		//Rotate the base, etc.
		key_b_pressed = false;
		display();
	}

	if (key == 'n') {
		//Rotate the base, etc.
		key_n_pressed = false;
		display();
	}




	if (key == 's') {
		//Rotate the base, etc.
		key_s_pressed = false;
		display();
	}

	if (key == 'a') {
		//Rotate the base, etc.
		key_a_pressed = false;
		display();
	}

	if (key == 'd') {
		//Rotate the base, etc.
		key_d_pressed = false;
		display();
	}

	if (key == 'f') {
		//Rotate the base, etc.
		key_f_pressed = false;
		display();
	}


	if (key == 'g') {
		//Rotate the base, etc.
		key_g_pressed = false;
		display();
	}

	if (key == 'h') {
		//Rotate the base, etc.
		key_h_pressed = false;
		display();
	}




	if (key == 'w') {
		//Scale the base, etc.
		key_w_pressed = false;
		display();
	}

	if (key == 'q') {
		//Scale the base, etc.
		key_q_pressed = false;
		display();
	}
}
#pragma endregion KEYBOARD

#pragma region MOUSE EVENT
void mouseClick(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (isMIP == FALSE)  isMIP = TRUE;
		else isMIP = FALSE;
	}

}
#pragma region MOUSE EVENT

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Transform");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	// glut keyboard callbacks
	glutKeyboardFunc(keypress);
	glutKeyboardUpFunc(keyUp);
	// glut mouse callbacks
	glutMouseFunc(mouseClick); //check for mouse

							   // A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();

	// Begin infinite event loop
	glutMainLoop();
	return 0;
}











