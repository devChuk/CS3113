#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

// load image texture
GLuint LoadImageTexture(const char *image_path)
{
	SDL_Surface *surface = IMG_Load(image_path);

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	SDL_FreeSurface(surface);

	return textureID;
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("HW 1 - Brian Chuk", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	// SETUP
	glViewport(0, 0, 640, 360);

	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;
	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);

	GLuint blocker_texture = LoadImageTexture(RESOURCE_FOLDER"blockerMad.png");
	GLuint fish_texture = LoadImageTexture(RESOURCE_FOLDER"fishDead.png");
	GLuint lolli_texture = LoadImageTexture(RESOURCE_FOLDER"lollipopWhiteRed.png");

	float blockerPos_X = 0.0f;
	float fishPos_X = 0.0f;
	float fishPos_Y = 0.0f;

	glEnable(GL_BLEND);

	// LOOP
	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		
		blockerPos_X += 0.0002f;
		fishPos_X += 0.2f;
		fishPos_Y += 0.2f;

		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);
				
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program.programID);

		
		// creating blocker
		modelMatrix.identity();
		modelMatrix.Translate(blockerPos_X, 0.0f, 0.0f);

		program.setModelMatrix(modelMatrix);

		float blockerVertices[] = { 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, blockerVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float BlockerTexCoords[] = { 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, BlockerTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, blocker_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		// creating fish
		modelMatrix.identity();

		program.setModelMatrix(modelMatrix);

		float fishVertices[] = { 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, fishVertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float fishTexCoords[] = { 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, fishTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, fish_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		// creating lolli
		modelMatrix.identity();

		program.setModelMatrix(modelMatrix);

		float lolliVerticesertices[] = { -1.05f, 2.0f, -3.55f, 2.0f, -3.55f, 1.5f, -3.55f, 1.5f, -1.05f, 1.5f, -1.05f, 2.0f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, lolliVerticesertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float lolliTexCoords[] = { 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, lolliTexCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, lolli_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);


		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
