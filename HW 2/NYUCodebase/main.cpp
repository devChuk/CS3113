#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <cstdlib>
#include <ctime>

#include "ShaderProgram.h"
#include "Matrix.h"


#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

// Paddle class
class Paddle {
public:
	Paddle(float lft, float rght, float tp, float bttm) : left(lft), right(rght), top(tp), bottom(bttm) {}

	float left;
	float right;
	float top;
	float bottom;
};

// Ball class
class Ball {
public:
	Ball(float posX, float posY, float vel, float spd, float acc, float dirX, float dirY) : pos_x(posX), pos_y(posY), speed(spd), accel(acc), dx(dirX), dy(dirY) {}
	Ball() {}
	float pos_x = 0.0f;
	float pos_y = 0.0f;
	float speed = 0.4f;
	float accel = 10.1f;
	float dx = (float)(rand() % 5 + 1);
	float dy = (float)(rand() % 10 - 4);

	void reset() {
		pos_x = 0.0f;
		pos_y = 0.0f;
		speed = 0.4f;
		accel = 10.1f;
		dx = (float)(rand() % 5 + 1);
		dy = (float)(rand() % 10 - 4);
	}

	void move(float elapsed) {
		pos_x += (speed * dx * elapsed);
		pos_y += (speed * dy * elapsed);
	}
};

// load image texture
GLuint LoadImageTexture(const char *image_path)
{
	SDL_Surface *surface = IMG_Load(image_path);

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface->w, surface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	SDL_FreeSurface(surface);

	return textureID;
}

int main(int argc, char *argv[])
{
	srand(time(NULL));

	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Pong Game By Brian Chuk", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 900, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	Matrix projectionMatrix;
	Matrix paddleLeftMatrix;
	Matrix paddleRightMatrix;
	Matrix ballMatrix;
	Matrix viewMatrix;
	projectionMatrix.setOrthoProjection(-4.0, 4.0, -2.25f, 2.25f, -1.0f, 1.0f);

	GLuint white = LoadImageTexture("white.png");

	Paddle paddleLeft(-3.7f, -3.6f, 0.5f, -0.5f);
	Paddle paddleRight(3.6f, 3.7f, 0.5f, -0.5f);
	Ball ball = Ball();

	float lastFrameTicks = 0.0f;

	SDL_Event event;
	bool done = false;
	bool gameRunning = false;
	while (!done) {
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//// Controls
		while (SDL_PollEvent(&event)) {
			// Game End
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			if (event.type == SDL_KEYDOWN) {
				// Game Start
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE && !gameRunning)
						gameRunning = true;
				
				// Left Paddle
				if (event.key.keysym.scancode == SDL_SCANCODE_W && paddleLeft.top < 2.25f) {
					paddleLeft.top += 0.3f;
					paddleLeft.bottom += 0.3f;
					paddleLeftMatrix.Translate(0.0f, 0.3f, 0.0f);
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_S && paddleLeft.bottom > -2.25f) {
					paddleLeft.top -= 0.3f;
					paddleLeft.bottom -= 0.3f;
					paddleLeftMatrix.Translate(0.0f, -0.3f, 0.0f);
				}

				// Right Paddle
				if (event.key.keysym.scancode == SDL_SCANCODE_UP && paddleRight.top < 2.25f) {
					paddleRight.top += 0.3f;
					paddleRight.bottom += 0.3f;
					paddleRightMatrix.Translate(0.0f, 0.3f, 0.0f);
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN && paddleRight.bottom > -2.25f) {
					paddleRight.top -= 0.3f;
					paddleRight.bottom -= 0.3f;
					paddleRightMatrix.Translate(0.0f, -0.3f, 0.0f);
				}
			}
		}

		//// Graphics & Drawing
		program.setModelMatrix(paddleLeftMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program.programID);

		float texture_coords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f }; // global texture coordinates

		// Left Paddle
		float paddleLeftVertices[] = { -3.7f, -0.5f, -3.6f, -0.5f, -3.6f, 0.5f, -3.6f, 0.5f, -3.7f, 0.5f, -3.7f, -0.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, paddleLeftVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, white);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		// Right Paddle
		program.setModelMatrix(paddleRightMatrix);
		float paddleRightVertices[] = { 3.6f, -0.5f, 3.7f, -0.5f, 3.6f, 0.5f, 3.7f, 0.5f, 3.6f, 0.5f, 3.7f, -0.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, paddleRightVertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, white);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		// Ball
		program.setModelMatrix(ballMatrix);
		float ball_coords[] = { -0.1f, -0.1f, 0.1f, -0.1f, 0.1f, 0.1f, 0.1f, 0.1f, -0.1f, 0.1f, -0.1f, -0.1f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, ball_coords);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, white);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		//// Movement & Game Logic
		if (gameRunning) {
			if (ball.pos_x <= paddleLeft.right && ball.pos_y <= paddleLeft.top && ball.pos_y >= paddleLeft.bottom ||
				ball.pos_x >= paddleRight.left && ball.pos_y <= paddleRight.top && ball.pos_y >= paddleRight.bottom) {
				ball.dx *= -1;
				ball.speed += ball.accel * elapsed;
				ball.move(elapsed);
				ballMatrix.Translate((ball.speed * ball.dx * elapsed), (ball.speed * ball.dy * elapsed), 0.0f);
			}
			// Left side wins!
			else if (ball.pos_x >= paddleRight.right) {
				gameRunning = false;
				ballMatrix.Translate(-ball.pos_x, -ball.pos_y, 0.0f);
				ball.reset();
				OutputDebugString("Left side wins!\n");
			}
			// Right side wins!
			else if (ball.pos_x  <= paddleLeft.left) {
				gameRunning = false;
				ballMatrix.Translate(-ball.pos_x, -ball.pos_y, 0.0f);
				ball.reset();
				OutputDebugString("Right side wins!\n");
			}
			// Wall collisions
			else if (ball.pos_y + 0.1f >= 2.25f || ball.pos_y - 0.1f <= -2.25f) {
				ball.dy *= -1;
				ball.speed += ball.accel * elapsed;
				ball.move(elapsed);
				ballMatrix.Translate((ball.speed * ball.dx * elapsed), (ball.speed * ball.dy * elapsed), 0.0f);
			}
			// Normal movement
			else {
				ball.move(elapsed);
				ballMatrix.Translate((ball.speed * ball.dx * elapsed), (ball.speed * ball.dy * elapsed), 0.0f);
			}
		}

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
