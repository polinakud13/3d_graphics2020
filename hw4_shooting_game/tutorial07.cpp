// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <ctime>
// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;
#include <random>
#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>

std::default_random_engine generator;
std::uniform_real_distribution<float> uniform(0.0, 1.0);

float random_num(float coef = 1) {
	return (float)std::rand() / RAND_MAX * coef;
}

struct Object {
	glm::vec3 position;
	glm::vec3 rotation;
	float angle;
	float speed;
	float radius;

	glm::mat4 getModelMatrix() const {
		glm::mat4 ModelMatrix = glm::scale(vec3(radius, radius, radius));
		ModelMatrix = glm::rotate(angle, rotation) * ModelMatrix;
		ModelMatrix = glm::translate(glm::mat4(), position) * ModelMatrix;
		return ModelMatrix;
	}

	void draw(GLuint texture_id, GLuint vertexbuffer, GLuint uvbuffer, GLuint normalbuffer) {}

};

struct Fireball : public Object {
	float birthTime;
	vec3 direction;
	Fireball() {
		speed = 10.0f;
		radius = 0.5f;
		birthTime = glfwGetTime();
		direction = getDirection();
		rotation = vec3(0, 1, 0);
		position = getPosition() + direction * (radius + 3.0f);
	}
	void move(float deltaTime) {
		position += direction * (speed * deltaTime);
	}
};

struct Enemy : public Object {
	float birthTime;
	vec3 direction;
	float speed;
	int enemy_radius = 5;
	int happy_time = 0;
	Enemy() {
		speed = 0.1f;
		radius = 1.0f;
		position = vec3(random_num(2 * enemy_radius) - enemy_radius + getPosition()[0],
			random_num(enemy_radius) + 2.0f, random_num(2 * enemy_radius) - enemy_radius + getPosition()[2]);
		rotation = vec3(random_num(), random_num(), random_num());
		angle = random_num(360.0f);
		direction = getPosition() - position;
	}

	void move(float deltaTime) {
		position += direction * (speed * deltaTime);
	}

};


int ENEMIES = 2;
int MAX_HAPPY_TIME = 250;
float INTER_DISTANCE = 1.0;
int ENEMY_VERTICES = 8;

int collides(const Enemy& enemy, const Fireball& fireball, std::vector<vec3>& vertices) {
		float fireball_radius = fireball.radius;
		vec3 fireball_position = fireball.position;
		vec3 enemy_position = enemy.position;
		int return_value = 0;
		if (glm::distance(fireball_position, enemy_position) < INTER_DISTANCE) {
			return_value = 1;
		}
		for (int i = 0; i < vertices.size(); i++) {
			if (glm::distance(fireball_position, enemy_position + vertices[i]) < fireball_radius) {
				return_value = 1;
			}
			for (int j = 0; j < vertices.size(); j++) {
				for (float alpha = 0; alpha < 1; alpha += 0.1) {
					float coord1 = vertices[i][0]*alpha + vertices[j][0]*(1-alpha); 
					float coord2 = vertices[i][1]*alpha + vertices[j][1] * (1 - alpha);
					float coord3 = vertices[i][2] * alpha + vertices[j][2] * (1 - alpha);
					glm::vec3 middle_point = vec3(coord1, coord2, coord3);
					if (glm::distance(fireball_position, enemy_position + middle_point) < fireball_radius) {
						return_value = 1;
					}
				}
			}
		}
		return return_value;
	}

int main( void )
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( 1024, 768, "Tutorial 07 - Model Loading", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

	// Dark blue background
	glClearColor(0.2f, 0.5f, 0.9f, 0.1f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS); 

	// Cull triangles which normal is not towards the camera
	//glEnable(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	GLuint FloorID = LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader");
	GLuint FloorMatrixID = glGetUniformLocation(FloorID, "MVP");
	GLuint FloorTexture = loadDDS("flower_field_texture.DDS");
	GLuint FloorTextureID = glGetUniformLocation(FloorID, "myTextureSampler");
	// Create and compile our GLSL program from the shaders
	GLuint SphereID = LoadShaders( "TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader" );

	// Get a handle for our "MVP" uniform
	GLuint SphereMatrixID = glGetUniformLocation(SphereID, "MVP");

	// Load the texture
	GLuint SphereTexture = loadDDS("flowerball.DDS");
	
	// Get a handle for our "myTextureSampler" uniform
	GLuint SphereTextureID  = glGetUniformLocation(SphereID, "myTextureSampler");



	GLuint enemyID = LoadShaders("EnemyTransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint EnemyMatrixID = glGetUniformLocation(enemyID, "MVP");

	// Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
	// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
	static const GLfloat enemy_vertex_buffer_data[] = {
			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f,-1.0f,
			1.0f, -1.0f,1.0f,

			1.0f, 1.0f, 1.0f,
			1.0f, 1.0f,-1.0f,
			-1.0f, 1.0f,1.0f,

			1.0f, 1.0f, 1.0f,
			-1.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 1.0f,

			1.0f, 1.0f, -1.0f,
			-1.0f, 1.0f, 1.0f,
			1.0f,-1.0f, 1.0f
	};

	std::vector<vec3> vertices;
	vertices.push_back(vec3(1.0f, 1.0f, 1.0f));
	vertices.push_back(vec3(1.0f, 1.0f, -1.0f));
	vertices.push_back(vec3(1.0f, -1.0f, 1.0f));
	vertices.push_back(vec3(-1.0f, 1.0f, 1.0f));

	static const GLfloat enemy_sad_color_buffer_data[] = {
			0.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  0.0f,

			0.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  0.0f,

			0.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  0.0f,

			0.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  0.0f,
			0.0f,  0.0f,  0.0f,
	};

	// One color for each vertex. They were generated randomly.
	static const GLfloat enemy_happy_color_buffer_data[] = {
			0.583f,  0.771f,  0.014f,
			0.609f,  0.115f,  0.436f,
			0.327f,  0.483f,  0.844f,


			0.597f,  0.770f,  0.761f,
			0.559f,  0.436f,  0.730f,
			0.359f,  0.583f,  0.152f,


			0.014f,  0.184f,  0.576f,
			0.771f,  0.328f,  0.970f,
			0.406f,  0.615f,  0.116f,


			0.997f,  0.513f,  0.064f,
			0.945f,  0.719f,  0.592f,
			0.543f,  0.021f,  0.978f
	};

	GLuint enemyvertexbuffer;
	glGenBuffers(1, &enemyvertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, enemyvertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(enemy_vertex_buffer_data), enemy_vertex_buffer_data, GL_STATIC_DRAW);

	GLuint enemyhappycolorbuffer;
	glGenBuffers(1, &enemyhappycolorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, enemyhappycolorbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(enemy_happy_color_buffer_data), enemy_happy_color_buffer_data, GL_STATIC_DRAW);

	GLuint enemysadcolorbuffer;
	glGenBuffers(1, &enemysadcolorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, enemysadcolorbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(enemy_sad_color_buffer_data), enemy_sad_color_buffer_data, GL_STATIC_DRAW);

	glm::mat4 ModelMatrix = glm::mat4(1.0);
	

	std::vector<Enemy> enemies;


	std::vector<glm::vec3> vertices_ball;
	std::vector<glm::vec2> uvs_ball;
	std::vector<glm::vec3> normals_ball;
	bool res_ball = loadOBJ("discoball.obj", vertices_ball, uvs_ball, normals_ball);

	// Load it into a VBO

	GLuint vertexbuffer_fireball;
	glGenBuffers(1, &vertexbuffer_fireball);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_fireball);
	glBufferData(GL_ARRAY_BUFFER, vertices_ball.size() * sizeof(glm::vec3), &vertices_ball[0], GL_STATIC_DRAW);

	GLuint uvbuffer_fireball;
	glGenBuffers(1, &uvbuffer_fireball);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_fireball);
	glBufferData(GL_ARRAY_BUFFER, uvs_ball.size() * sizeof(glm::vec2), &uvs_ball[0], GL_STATIC_DRAW);
	int ball_vertex_size = vertices_ball.size();
	int ball_uv_size = uvs_ball.size();

	std::vector<Fireball> fireball_vector; // набор сфер
	float sphere_speed = 10.0f;

	float render_distance = 70.0f;


	std::vector<glm::vec3> vertices_floor;
	std::vector<glm::vec2> uvs_floor;
	std::vector<glm::vec3> normals_floor; // Won't be used at the moment.

	bool res_floor = loadOBJ("floor.obj", vertices_floor, uvs_floor, normals_floor);

	GLuint vertexbuffer_floor;
	glGenBuffers(1, &vertexbuffer_floor);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_floor);
	glBufferData(GL_ARRAY_BUFFER, vertices_floor.size() * sizeof(glm::vec3), &vertices_floor[0], GL_STATIC_DRAW);

	GLuint uvbuffer_floor;
	glGenBuffers(1, &uvbuffer_floor);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_floor);
	glBufferData(GL_ARRAY_BUFFER, uvs_floor.size() * sizeof(glm::vec2), &uvs_floor[0], GL_STATIC_DRAW);
	int floor_vertex_size = vertices_floor.size();
	int floor_uv_size = uvs_floor.size();
	glm::vec3 floor_position = getPosition();
	floor_position[1] -= 5.0;

	float lastTime = glfwGetTime();
	float counter = 1;
	do{

		
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		counter += 1;
		// Use our shader
		
		double currentTime = glfwGetTime();
		float deltaTime = float(currentTime - lastTime);
		lastTime = currentTime;
		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glUseProgram(FloorID);
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(5.0);
		ModelMatrix = glm::translate(glm::mat4(), floor_position) * ModelMatrix;
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		//MAKING FLOOR

		glUniformMatrix4fv(FloorMatrixID, 1, GL_FALSE, &MVP[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, FloorTexture);
		glUniform1i(FloorTextureID, 0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_floor);
		glVertexAttribPointer(
			0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_floor);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);
		glDrawArrays(GL_TRIANGLES, 0, floor_vertex_size);
		//glDisableVertexAttribArray(0);
		//glDisableVertexAttribArray(1);
		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform

		static int oldState = GLFW_RELEASE;
		int newState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		if (newState == GLFW_RELEASE && oldState == GLFW_PRESS) {
			fireball_vector.push_back(Fireball());
		}
		oldState = newState;
		
		for (int i = 0; i < fireball_vector.size(); i++) {
			if (glm::length(fireball_vector[i].position - getPosition()) > render_distance) {
				fireball_vector.erase(fireball_vector.begin() + i);
			}
		}

		for (int i = 0; i < fireball_vector.size(); i++) {
			glUseProgram(SphereID);

			ModelMatrix = fireball_vector[i].getModelMatrix();
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			fireball_vector[i].move(deltaTime);
			glUniformMatrix4fv(SphereMatrixID, 1, GL_FALSE, &MVP[0][0]);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, SphereTexture);
			// Set our "myTextureSampler" sampler to use Texture Unit 0
			glUniform1i(SphereTextureID, 0);

			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_fireball);
			glVertexAttribPointer(
				0,                  // attribute
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
			);

			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, uvbuffer_fireball);
			glVertexAttribPointer(
				1,                                // attribute
				2,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
			);
			glDrawArrays(GL_TRIANGLES, 0, ball_vertex_size);

		}

		glUseProgram(enemyID);
		float enemy_radius = 4;

		int enem_count = enemies.size();
		if (enem_count < 3) { // создаем цель
			enemies.push_back(Enemy());
		}
		
		for (int i = 0; i < enemies.size(); i++) { //каждую цель двигаем так, как сказали при ее создании
			// не уверенна, что надо. У Вики вроде эта штука равна glm::mat4(1.0); на каждой итерации. Но у меня это слишком дергано
			auto ModelMatrix = enemies[i].getModelMatrix();
			MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			// Send our transformation to the currently bound shader,
			// in the "MVP" uniform
			glUniformMatrix4fv(EnemyMatrixID, 1, GL_FALSE, &MVP[0][0]);

			// 1rst attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, enemyvertexbuffer);
			glVertexAttribPointer(
				0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
			);

			// 2nd attribute buffer : colors
			glEnableVertexAttribArray(1);
			if (enemies[i].happy_time) {
				glBindBuffer(GL_ARRAY_BUFFER, enemyhappycolorbuffer);
			}
			else {
				glBindBuffer(GL_ARRAY_BUFFER, enemysadcolorbuffer);
			}
			glVertexAttribPointer(
				1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
				3,                                // size
				GL_FLOAT,                         // type
				GL_FALSE,                         // normalized?
				0,                                // stride
				(void*)0                          // array buffer offset
			);
			glDrawArrays(GL_TRIANGLES, 0, 8 * 3);

		}
		std::vector<int> to_be_erased_fireballs;
		for (int i = 0; i < enem_count; i++) {
			for (int j = 0; j < fireball_vector.size(); j++) {
				if (collides(enemies[i], fireball_vector[j], vertices)) {
					to_be_erased_fireballs.push_back(j);
					enemies[i].happy_time += 1;
				}
			}
		}

		for (int i = 0; i < to_be_erased_fireballs.size(); i++) {
			fireball_vector.erase(fireball_vector.begin() + to_be_erased_fireballs[i]);
		}
		std::vector<int> to_be_erased_enemies;
		for (int i = 0; i < enem_count; i++) {
			if (enemies[i].happy_time) {
				enemies[i].happy_time += 1;
			}
			if (enemies[i].happy_time > MAX_HAPPY_TIME) {
				to_be_erased_enemies.push_back(i);
			}
		}
		for (int i = 0; i < to_be_erased_enemies.size(); i++) {
			enemies.erase(enemies.begin() + to_be_erased_enemies[i]);
		}

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &uvbuffer_fireball);
	glDeleteBuffers(1, &vertexbuffer_fireball);
	glDeleteProgram(SphereID);
	glDeleteProgram(FloorID);
	glDeleteBuffers(1, &uvbuffer_floor);
	glDeleteBuffers(1, &vertexbuffer_floor);
	glDeleteTextures(1, &SphereTexture);
	glDeleteTextures(1, &FloorTexture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

