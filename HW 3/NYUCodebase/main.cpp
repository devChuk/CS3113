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
GLuint spriteSheetTexture;
Matrix projectionMatrix;
Matrix viewMatrix;
Matrix modelMatrix;
class Entity;
float texture_coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f }; // global texture coordinates

// Game values
enum GameState { STATE_MAIN_MENU, STATE_GAME_LEVEL };
bool gameRunning = true;
enum Type { PLAYER, ALIEN };
float lastFrameTicks = 0.0f;
float elapsed;
float timeSinceLastFire = 0.0f;
float timeSinceLastEnemyFire = 0.0f;
bool controlsMoveLeft = false;
bool controlsMoveRight = false;
bool controlsFireBullet = false;

int state;
ShaderProgram* program;

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
	float position[2];
	float boundaries[4]; //top, bottom, left, right
	float speed[2];
	Matrix entityMatrix;
	float u;
	float v;
	float width;
	float height;
	float size = 1.0f;
	Type type;

	Entity() {}

	Entity(float x, float y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float dx, float dy) {
		position[0] = x;
		position[1] = y;
		speed[0] = dx;
		speed[1] = dy;
		entityMatrix.identity();
		entityMatrix.Translate(x, y, 0);
		boundaries[0] = y + 0.05f * size;
		boundaries[1] = y - 0.05f * size;
		boundaries[2] = x - 0.05f * size;
		boundaries[3] = x + 0.05f * size;

		u = spriteU;
		v = spriteV;
		width = spriteWidth;
		height = spriteHeight;
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
			(-0.1f * size), 0.1f * size,
			(-0.1f * size), -0.1f * size,
			(0.1f * size), 0.1f * size,
			(0.1f * size), -0.1f * size,
			(0.1f * size), 0.1f * size,
			(-0.1f * size), -0.1f * size,
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
		glBindTexture(GL_TEXTURE_2D, spriteSheetTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

};

void RenderMainMenu() {

	//draws text
	modelMatrix.identity();
	modelMatrix.Translate(-0.5f, 2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "SPACE", 0.2f, 0.0001f);
	modelMatrix.Translate(-0.3f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "INVADERS", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-3.6f, 0.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "USE ARROW KEYS TO MOVE, SPACE TO FIRE", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-1.6f, -2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "PRESS SPACE TO START", 0.2f, 0.0001f);

}

Entity player;
std::vector<Entity> enemies;
std::vector<Entity> playerlasers;
std::vector<Entity> enemylasers;

void UpdateMainMenu(float elapsed) {
	// does nothing really. We just have static text to worry about here.
}

void RenderGameLevel() {
	player.draw();
	for (size_t i = 0; i < enemies.size(); i++) {
		enemies[i].draw();
	}
	for (size_t i = 0; i < playerlasers.size(); i++) {
		playerlasers[i].draw();
	}

	for (size_t i = 0; i < enemylasers.size(); i++) {
		enemylasers[i].draw();
	}
}

void UpdateGameLevel(float elapsed) {

	if (controlsMoveLeft) {
		player.position[0] -= player.speed[0] * elapsed;
		player.boundaries[2] -= player.speed[0] * elapsed;
		player.boundaries[3] -= player.speed[0] * elapsed;
	}
	else if (controlsMoveRight) {
		player.position[0] += player.speed[0] * elapsed;
		player.boundaries[2] += player.speed[0] * elapsed;
		player.boundaries[3] += player.speed[0] * elapsed;
	}

	if (controlsFireBullet) {
		if (timeSinceLastFire > 0.5f) {
			timeSinceLastFire = 0;
			playerlasers.push_back(Entity(player.position[0], player.position[1], 858.0f / 1024.0f, 230.0f / 1024.0f, 9.0f / 1024.0f, 54.0f / 1024.0f, 0, 4.0f));
		}
	}

	for (size_t i = 0; i < enemies.size(); i++) {
		enemies[i].position[1] -= enemies[i].speed[1] * elapsed;
		enemies[i].boundaries[0] -= enemies[i].speed[1] * elapsed;
		enemies[i].boundaries[1] -= enemies[i].speed[1] * elapsed;

		enemies[i].position[0] += enemies[i].speed[0] * elapsed;
		enemies[i].boundaries[2] += enemies[i].speed[0] * elapsed;
		enemies[i].boundaries[3] += enemies[i].speed[0] * elapsed;


		if ((enemies[i].boundaries[3] > 3.3f && enemies[i].speed[0] > 0) || (enemies[i].boundaries[2] < -3.3f && enemies[i].speed[0] < 0)) {
			for (size_t i = 0; i < enemies.size(); i++) {
				enemies[i].speed[0] = -enemies[i].speed[0];
			}
		}

		if (enemies[i].boundaries[1] < player.boundaries[0] &&
			enemies[i].boundaries[0] > player.boundaries[1] &&
			enemies[i].boundaries[2] < player.boundaries[3] &&
			enemies[i].boundaries[3] > player.boundaries[2]) {
			gameRunning = false;
		}
	}

	std::vector<int> removePlayerLaserIndex;

	for (size_t i = 0; i < playerlasers.size(); i++) {
		playerlasers[i].position[1] += playerlasers[i].speed[1] * elapsed;
		playerlasers[i].boundaries[0] += playerlasers[i].speed[1] * elapsed;
		playerlasers[i].boundaries[1] += playerlasers[i].speed[1] * elapsed;

		for (size_t j = 0; j < enemies.size(); j++) {
			if (enemies[j].boundaries[1] < playerlasers[i].boundaries[0] &&
				enemies[j].boundaries[0] > playerlasers[i].boundaries[1] &&
				enemies[j].boundaries[2] < playerlasers[i].boundaries[3] &&
				enemies[j].boundaries[3] > playerlasers[i].boundaries[2]) {
				enemies.erase(enemies.begin() + j);
				removePlayerLaserIndex.push_back(i);
			}
		}
	}

	for (int i = 0; i < removePlayerLaserIndex.size(); i++) {
		playerlasers.erase(playerlasers.begin() + removePlayerLaserIndex[i] - i);
	}

	if (timeSinceLastEnemyFire > 0.4f) {
		timeSinceLastEnemyFire = 0;
		int shooter = rand() % enemies.size();
		enemylasers.push_back(Entity(enemies[shooter].position[0], enemies[shooter].position[1], 854.0f / 1024.0f, 639.0f / 1024.0f, 9.0f / 1024.0f, 37.0f / 1024.0f, 0, -2.0f));
	}

	for (size_t i = 0; i < enemylasers.size(); i++) {
		enemylasers[i].position[1] += enemylasers[i].speed[1] * elapsed;
		enemylasers[i].boundaries[0] += enemylasers[i].speed[1] * elapsed;
		enemylasers[i].boundaries[1] += enemylasers[i].speed[1] * elapsed;
		if (enemylasers[i].boundaries[1] < player.boundaries[0] &&
			enemylasers[i].boundaries[0] > player.boundaries[1] &&
			enemylasers[i].boundaries[2] < player.boundaries[3] &&
			enemylasers[i].boundaries[3] > player.boundaries[2]) {
			gameRunning = false;
		}
	}

	if (enemies.size() == 0)
		gameRunning = false;
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
	displayWindow = SDL_CreateWindow("Brian Chuk's Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_OPENGL);
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
	spriteSheetTexture = LoadTexture("sheet.png");
	//initialize entities
	player = Entity(0.0f, -2.0f, 224.0f / 1024.0f, 832.0f / 1024.0f, 99.0f / 1024.0f, 75.0f / 1024.0f, 3.0f, 0);
	for (int i = 0; i < 55; i++) {
		enemies.push_back(Entity(-2.5 + (i % 11) * 0.5, 2.0 - (i / 11 * 0.5), 425.0f / 1024.0f, 552.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 1.0f, 0.03f));
	}

	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
				done = true;
			//if (event.type == SDL_KEYDOWN) {
			switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					//firing, starting the game
					if (state == STATE_MAIN_MENU) {
						state = STATE_GAME_LEVEL;
					}
					else {
						//spawn a bullet.
						controlsFireBullet = true;
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
					controlsFireBullet = false;
				}
				break;
			}
		}

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		timeSinceLastFire += elapsed;
		timeSinceLastEnemyFire += elapsed;

		if (gameRunning) {
			Update(elapsed);
			Render();
		}
	}

	SDL_Quit();
	return 0;
}
