#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include <math.h>

#include "ShaderProgram.h"
#include "Matrix.h"

#pragma warning (disable : 4996)

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

struct Point {
	Point(float EX, float WHY) {
		x = EX;
		y = WHY;
	}
	float x; float y;
};

bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Point> &points1, const std::vector<Point> &points2) {
	float normalX = -edgeY;
	float normalY = edgeX;
	float len = sqrtf(normalX*normalX + normalY*normalY);
	normalX /= len;
	normalY /= len;

	std::vector<float> e1Projected;
	std::vector<float> e2Projected;

	for (int i = 0; i < points1.size(); i++) {
		e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
	}
	for (int i = 0; i < points2.size(); i++) {
		e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
	}

	std::sort(e1Projected.begin(), e1Projected.end());
	std::sort(e2Projected.begin(), e2Projected.end());

	float e1Min = e1Projected[0];
	float e1Max = e1Projected[e1Projected.size() - 1];
	float e2Min = e2Projected[0];
	float e2Max = e2Projected[e2Projected.size() - 1];
	float e1Width = fabs(e1Max - e1Min);
	float e2Width = fabs(e2Max - e2Min);
	float e1Center = e1Min + (e1Width / 2.0);
	float e2Center = e2Min + (e2Width / 2.0);
	float dist = fabs(e1Center - e2Center);
	float p = dist - ((e1Width + e2Width) / 2.0);

	if (p < 0) {
		return true;
	}
	return false;
}

bool checkSATCollision(const std::vector<Point> &e1Points, const std::vector<Point> &e2Points) {
	for (int i = 0; i < e1Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e1Points.size() - 1) {
			edgeX = e1Points[0].x - e1Points[i].x;
			edgeY = e1Points[0].y - e1Points[i].y;
		}
		else {
			edgeX = e1Points[i + 1].x - e1Points[i].x;
			edgeY = e1Points[i + 1].y - e1Points[i].y;
		}

		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points);
		if (!result) {
			return false;
		}
	}
	for (int i = 0; i < e2Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e2Points.size() - 1) {
			edgeX = e2Points[0].x - e2Points[i].x;
			edgeY = e2Points[0].y - e2Points[i].y;
		}
		else {
			edgeX = e2Points[i + 1].x - e2Points[i].x;
			edgeY = e2Points[i + 1].y - e2Points[i].y;
		}
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points);
		if (!result) {
			return false;
		}
	}
	return true;
}

Point rotate_point(float cx, float cy, float angle, Point p)
{
	float s = sin(angle);
	float c = cos(angle);

	// translate point back to origin:
	p.x -= cx;
	p.y -= cy;

	// rotate point
	float xnew = p.x * c - p.y * s;
	float ynew = p.x * s + p.y * c;

	// translate point back:
	p.x = xnew + cx;
	p.y = ynew + cy;
	return p;
}

class Entity {
public:
	Matrix entityMatrix;
	float position[2];		//location (center point of entity)
	float boundaries[4];	//top, bottom, left, right (from position)					!!Keep in mind these do not follow the actual shapes if they are rotated
	float size[2];
	std::vector<Point> corners; //starts with top left, then clockwise
	float angle;			//radians. counterclockwise

	float speed[2];
	float acceleration[2];
	bool collided[4]; //same as boundaries, top bot left right
	
	bool isStatic = false;

	float u;
	float v;
	float width;
	float height;
	GLuint texture;

	Entity() {}

	Entity(float x, float y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float dx, float dy, GLuint spriteTexture) {
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
	}

	Entity(float x, float y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, float dx, float dy, GLuint spriteTexture, float sizeX, float sizeY, float newAngle) {
		position[0] = x;
		position[1] = y;
		speed[0] = dx;
		speed[1] = dy;
		acceleration[0] = 0.0f;
		acceleration[1] = 0.0f;
		entityMatrix.identity();
		entityMatrix.Translate(x, y, 0);
		size[0] = sizeX;
		size[1] = sizeY;
		boundaries[0] = y + 0.1f * size[1];
		boundaries[1] = y - 0.1f * size[1];
		boundaries[2] = x - 0.1f * size[0];
		boundaries[3] = x + 0.1f * size[0];

		u = spriteU;
		v = spriteV;
		width = spriteWidth;
		height = spriteHeight;
		texture = spriteTexture;

		angle = newAngle;
		corners.push_back(rotate_point(position[0], position[1], angle, Point(boundaries[2], boundaries[0])));
		corners.push_back(rotate_point(position[0], position[1], angle, Point(boundaries[3], boundaries[0])));
		corners.push_back(rotate_point(position[0], position[1], angle, Point(boundaries[3], boundaries[1])));
		corners.push_back(rotate_point(position[0], position[1], angle, Point(boundaries[2], boundaries[1])));
	}

	void draw() {
		entityMatrix.identity();
		entityMatrix.Translate(position[0], position[1], 0);
		entityMatrix.Rotate(angle);		//counterclockwise
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

			for (size_t i = 0; i < corners.size(); i++) {
				corners[i].x += speed[0] * elapsed;
				corners[i].y += speed[1] * elapsed;
			}
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

std::vector<Entity> blocks;

void RenderMainMenu() {

	//draws text
	modelMatrix.identity();
	modelMatrix.Translate(-0.5f, 2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "JUST", 0.2f, 0.0001f);
	modelMatrix.Translate(-0.8f, -0.5f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "THREE RECTANGLES COLLIDING", 0.2f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-1.6f, -2.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, fontTexture, "PRESS SPACE TO START", 0.2f, 0.0001f);

}

void UpdateMainMenu(float elapsed) {
	// does nothing really. We just have static text to worry about here.
}

void RenderGameLevel() {
	for (size_t i = 0; i < blocks.size(); i++) {
		blocks[i].draw();
	}
	viewMatrix.identity();
	program->setViewMatrix(viewMatrix);
}

void UpdateGameLevel(float elapsed) {
	for (size_t i = 0; i < blocks.size(); i++) {
		if (blocks[i].corners.size() == 4)
			blocks[i].update(elapsed);
		
		//std::cout << "BLOCK " << i << " ||||||||||||||\n";
		//for (size_t m = 0; m < blocks[i].corners.size(); m++) {
		//	std::cout << blocks[i].corners[m].x << " " << blocks[i].corners[m].y << "\n";
		//}
		//std::cout << "||||||||||||||\n";
		

		for (size_t j = i + 1; j < blocks.size(); j++) {
			if (checkSATCollision(blocks[i].corners, blocks[j].corners)) {
				blocks[i].isStatic = true;
				blocks[j].isStatic = true;
			}
		}
	}
	/*std::cout << std::endl;*/
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

	//AllocConsole();
	//freopen("CONOUT$", "w", stdout);
	//freopen("CONOUT$", "w", stderr);

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

	blocks.push_back(Entity(-2.5f, -1.6f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, groundTexture, 5.0f, 5.0f, 0.785f));

	blocks.push_back(Entity(-2.0f, -0.2f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, groundTexture, 3.0f, 8.0f, 0.32f));

	blocks.push_back(Entity(1.0f, 0.1f, 0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, groundTexture, 1.0f, 12.0f, 0.1f));

	//player = Entity(0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0, 0, playerSpriteTexture, 1.0f, 1.4f, PLAYER);

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
						gameRunning = false;
					}
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
