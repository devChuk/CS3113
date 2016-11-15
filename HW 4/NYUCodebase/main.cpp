#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <cstdlib>
#include <ctime>
#include <vector>

#include "ShaderProgram.h"
#include "Matrix.h"

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

// SDL & rendering objects
SDL_Window* displayWindow;
GLuint fontTexture;
GLuint playerSpriteTexture;
GLuint groundTexture;
GLuint powerupTexture;

Matrix projectionMatrix;
Matrix viewMatrix;
Matrix modelMatrix;

ShaderProgram* program;
class Entity;
float texture_coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f }; // global texture coordinates

// GameLogic values
enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL };
int state;
bool gameRunning = true;


float lastFrameTicks = 0.0f;
float elapsed;
#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

bool controlsMoveLeft = false;
bool controlsMoveRight = false;
bool controlsJump = false;
float playerSpeed = 3.0f;

enum Type { PLAYER, BLOCK, POWERUP };

void DrawText(ShaderProgram* program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (size_t i = 0; i < text.size(); i++) {
		float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
		float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}
	glUseProgram(program->programID);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);

	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

GLuint LoadTexture(const char* image_path) {
	SDL_Surface* surface = IMG_Load(image_path);

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	SDL_FreeSurface(surface);

	return textureID;
}

class Entity {
public:
	Matrix entityMatrix;
	float position[2];		//location (center point of entity)
	float boundaries[4];	//top, bottom, left, right (from position)
	float size[2];
	
	float speed[2];
	float acceleration[2];
	bool collided[4]; //same as boundaries, top bot left right

	bool isStatic = true;
	Type type;
	
	float u;
	float v;
	float width;
	float height;
	GLuint texture;

	Entity() {}

	Entity(float x, float y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float dx, float dy, GLuint spriteTexture, Type newType) {
		position[0] = x;
		position[1] = y;
		speed[0] = dx;
		speed[1] = dy;
		entityMatrix.identity();
		entityMatrix.Translate(x, y, 0);
		size[0] = 1.0f;
		size[1] = 1.0f;
		boundaries[0] = y + 0.05f * size[1] * 2;
		boundaries[1] = y - 0.05f * size[1] * 2;
		boundaries[2] = x - 0.05f * size[0] * 2;
		boundaries[3] = x + 0.05f * size[0] * 2;

		u = spriteU;
		v = spriteV;
		width = spriteWidth;
		height = spriteHeight;
		texture = spriteTexture;
		
		type = newType;
	}
	Entity(float x, float y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float dx, float dy, GLuint spriteTexture, float sizeX, float sizeY, Type newType) {
		position[0] = x;
		position[1] = y;
		speed[0] = dx;
		speed[1] = dy;
		entityMatrix.identity();
		entityMatrix.Translate(x, y, 0);
		size[0] = sizeX;
		size[1] = sizeY;
		boundaries[0] = y + 0.05f * size[1] * 2;
		boundaries[1] = y - 0.05f * size[1] * 2;
		boundaries[2] = x - 0.05f * size[0] * 2;
		boundaries[3] = x + 0.05f * size[0] * 2;

		u = spriteU;
		v = spriteV;
		width = spriteWidth;
		height = spriteHeight;
		texture = spriteTexture;

		type = newType;
	}

	void draw() {
		entityMatrix.identity();
		entityMatrix.Translate(position[0], position[1], 0);
		program->setModelMatrix(entityMatrix);

		std::vector<float> vertexData;
		std::vector<float> texCoordData;
		float texture_x = u;
		float texture_y = v;
		vertexData.insert(vertexData.end(), {
			(-0.1f * size[0]), 0.1f * size[1],
			(-0.1f * size[0]), -0.1f * size[1],
			(0.1f * size[0]), 0.1f * size[1],
			(0.1f * size[0]), -0.1f * size[1],
			(0.1f * size[0]), 0.1f * size[1],
			(-0.1f * size[0]), -0.1f * size[1],
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + height,
			texture_x + width, texture_y,
			texture_x + width, texture_y + height,
			texture_x + width, texture_y,
			texture_x, texture_y + height,
		});

		glUseProgram(program->programID);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
		glEnableVertexAttribArray(program->positionAttribute);
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
		glEnableVertexAttribArray(program->texCoordAttribute);
		
		glBindTexture(GL_TEXTURE_2D, texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	void update(float elapsed) {
		if (!isStatic) {

			speed[0] += acceleration[0];
			position[0] += speed[0] * elapsed;
			boundaries[2] += speed[0] * elapsed;
			boundaries[3] += speed[0] * elapsed;

			speed[1] += acceleration[1];
			position[1] += speed[1] * elapsed;
			boundaries[0] += speed[1] * elapsed;
			boundaries[1] += speed[1] * elapsed;
		}
	}

	void updateX(float elapsed) {
		if (!isStatic) {
			speed[0] += acceleration[0];
			position[0] += speed[0] * elapsed;
			boundaries[2] += speed[0] * elapsed;
			boundaries[3] += speed[0] * elapsed;
		}
	}

	void updateY(float elapsed) {
		if (!isStatic) {
			speed[1] += acceleration[1];
			position[1] += speed[1] * elapsed;
			boundaries[0] += speed[1] * elapsed;
			boundaries[1] += speed[1] * elapsed;
		}
	}

};

Entity player;
std::vector<Entity> blocks;
std::vector<Entity> powerups;

void RenderMainMenu() {

	//draws text
	modelMatrix.identity();
	modelMatrix.Translate(-0.5f, 2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "ALIEN", 0.2f, 0.0001f);
	modelMatrix.Translate(-0.8f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "RUNNING AROUND", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-3.6f, 0.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "USE SPACE TO JUMP. ARROW KEYS TO MOVE", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-1.6f, -2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "PRESS SPACE TO START", 0.2f, 0.0001f);

}

void UpdateMainMenu(float elapsed) {
	// does nothing really. We just have static text to worry about here.
}

void RenderGameLevel() {
	player.draw();
	for (size_t i = 0; i < blocks.size(); i++) {
		blocks[i].draw();
	}
	for (size_t i = 0; i < powerups.size(); i++) {
		powerups[i].draw();
	}
	viewMatrix.identity();
	viewMatrix.Translate(-player.position[0], -player.position[1], 0.0f);
	program->setViewMatrix(viewMatrix);
}

void UpdateGameLevel(float elapsed) {
	for (int i = 0; i < 4; i++) {
		player.collided[i] = false;
	}
	
	float penetration;
	OutputDebugStringA("Swag");

	player.updateY(elapsed);
	for (size_t i = 0; i < powerups.size(); i++) {
		powerups[i].updateY(elapsed);
	}

	for (size_t i = 0; i < blocks.size(); i++) {
		if (player.boundaries[1] < blocks[i].boundaries[0] &&
			player.boundaries[0] > blocks[i].boundaries[1] &&
			player.boundaries[2] < blocks[i].boundaries[3] &&
			player.boundaries[3] > blocks[i].boundaries[2])
		{
			float y_distance = fabs(player.position[1] - blocks[i].position[1]);
			float playerHeightHalf = 0.05f * player.size[1] * 2;
			float blockHeightHalf = 0.05f * blocks[i].size[1] * 2;
			penetration = fabs(y_distance - playerHeightHalf - blockHeightHalf);

			if (player.position[1] > blocks[i].position[1]) {
				player.position[1] += penetration + 0.001f;
				player.boundaries[0] += penetration + 0.001f;
				player.boundaries[1] += penetration + 0.001f;
				player.collided[1] = true;
			}
			else {
				player.position[1] -= (penetration + 0.001f);
				player.boundaries[0] -= (penetration + 0.001f);
				player.boundaries[1] -= (penetration + 0.001f);
				player.collided[0] = true;
			}
			player.speed[1] = 0.0f;
			break;
		}
	}

	for (size_t j = 0; j < powerups.size(); j++)
		for (size_t i = 0; i < blocks.size(); i++) {
			if (powerups[j].boundaries[1] < blocks[i].boundaries[0] &&
				powerups[j].boundaries[0] > blocks[i].boundaries[1] &&
				powerups[j].boundaries[2] < blocks[i].boundaries[3] &&
				powerups[j].boundaries[3] > blocks[i].boundaries[2])
			{
				float y_distance = fabs(powerups[j].position[1] - blocks[i].position[1]);
				float powerupsHeightHalf = 0.05f * powerups[j].size[1] * 2;
				float blockHeightHalf = 0.05f * blocks[i].size[1] * 2;
				penetration = fabs(y_distance - powerupsHeightHalf - blockHeightHalf);

				if (powerups[j].position[1] > blocks[i].position[1]) {
					powerups[j].position[1] += penetration + 0.001f;
					powerups[j].boundaries[0] += penetration + 0.001f;
					powerups[j].boundaries[1] += penetration + 0.001f;
					powerups[j].speed[1] = 3.0f;
				}
				else {
					powerups[j].position[1] -= (penetration + 0.001f);
					powerups[j].boundaries[0] -= (penetration + 0.001f);
					powerups[j].boundaries[1] -= (penetration + 0.001f);
					powerups[j].collided[0] = true;
					powerups[j].speed[1] = 0.0f;
				}
			
				break;
			}
		}
	
	player.updateX(elapsed);
	for (size_t i = 0; i < blocks.size(); i++) {
		if (player.boundaries[1] < blocks[i].boundaries[0] &&
			player.boundaries[0] > blocks[i].boundaries[1] &&
			player.boundaries[2] < blocks[i].boundaries[3] &&
			player.boundaries[3] > blocks[i].boundaries[2])
		{
			float x_distance = fabs(player.position[0] - blocks[i].position[0]);
			float playerWidthHalf = 0.05f * player.size[0] * 2;
			float blockWidthHalf = 0.05f * blocks[i].size[0] * 2;
			penetration = fabs(x_distance - (playerWidthHalf + blockWidthHalf));

			if (player.position[0] > blocks[i].position[0]) {
				player.position[0] += penetration + 0.001f;
				player.boundaries[2] += penetration + 0.001f;
				player.boundaries[3] += penetration + 0.001f;
				player.collided[3] = true;
			}
			else {
				player.position[0] -= (penetration + 0.001f);
				player.boundaries[2] -= (penetration + 0.001f);
				player.boundaries[3] -= (penetration + 0.001f);
				player.collided[2] = true;
			}
			player.speed[0] = 0.0f;
			break;
		}
	}

	player.speed[0] = 0.0f;

	for (size_t i = 0; i < powerups.size(); i++) {
		if (player.boundaries[1] < powerups[i].boundaries[0] &&
			player.boundaries[0] > powerups[i].boundaries[1] &&
			player.boundaries[2] < powerups[i].boundaries[3] &&
			player.boundaries[3] > powerups[i].boundaries[2])
		{
			powerups.erase(powerups.begin() + i);
			playerSpeed = 10.0f;
		}
	}


	
	if (controlsMoveLeft)
		player.speed[0] = -playerSpeed;
	else if (controlsMoveRight)
		player.speed[0] = playerSpeed;
	if (controlsJump && player.collided[1])
		player.speed[1] = 5.0f;
}

void Render() {
	glClear(GL_COLOR_BUFFER_BIT);
	switch (state) {
	case STATE_MAIN_MENU:
		RenderMainMenu();
		break;
	case STATE_GAME_LEVEL:
		RenderGameLevel();
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
}

void Update(float elapsed) {
	switch (state) {
	case STATE_MAIN_MENU:
		UpdateMainMenu(elapsed);
		break;
	case STATE_GAME_LEVEL:
		UpdateGameLevel(elapsed);
		break;
	}
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Brian Chuk's Basic Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	SDL_Event event;
	bool done = false;

	projectionMatrix.setOrthoProjection(-4.0, 4.0, -2.25f, 2.25f, -1.0f, 1.0f);
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	//insert a lot of model matrices

	//create GLUint textures
	fontTexture = LoadTexture("font1.png");
	playerSpriteTexture = LoadTexture("p1_jump.png");
	groundTexture = LoadTexture("castleCenter.png");
	powerupTexture = LoadTexture("cherry.png");
	//initialize entities
	player = Entity(0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, 0, playerSpriteTexture, 1.0f, 1.4f, PLAYER);
	player.isStatic = false;
	player.acceleration[1] = -0.01f;
	
	for (int i = 0; i < 55; i++) {
		blocks.push_back(Entity(-2.5f + (i) * 0.2f, 0.0f - (4 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, groundTexture, BLOCK));
	}

	for (int i = 0; i < 4; i++) {
		blocks.push_back(Entity(-2.0f + (i)* 0.2f, 0.0f - (1 * 0.5f), 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, groundTexture, BLOCK));
	}

	for (int i = 0; i < 4; i++) {
		blocks.push_back(Entity(1.0f + (i)* 0.2f, -1.8f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, groundTexture, BLOCK));
	}

	powerups.push_back(Entity(-1.7f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, powerupTexture, POWERUP));
	powerups[0].acceleration[1] = -0.01f;
	powerups[0].isStatic = false;

	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
				done = true;
			switch (event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
						//firing, starting the game
						if (state == STATE_MAIN_MENU) {
							state = STATE_GAME_LEVEL;
						}
						else {
							//jump.
							controlsJump = true;
						}
					}
					if (event.key.keysym.scancode == SDL_SCANCODE_LEFT && player.boundaries[2] > -3.5f) {
						controlsMoveLeft = true;
					}
					else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT && player.boundaries[3] < 3.5f) {
						controlsMoveRight = true;
					}
					break;
				case SDL_KEYUP:
					if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
						controlsMoveLeft = false;
					}
					if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
						controlsMoveRight = false;
					}
					if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
						controlsJump = false;
					}
					break;
			}
		}

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		if (gameRunning) {
			float fixedElapsed = elapsed;
			if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
				fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
			}
			while (fixedElapsed >= FIXED_TIMESTEP) {
				fixedElapsed -= FIXED_TIMESTEP;
				Update(FIXED_TIMESTEP);
			}
			Update(fixedElapsed);
			Render();
		}
	}

	SDL_Quit();
	return 0;
}
