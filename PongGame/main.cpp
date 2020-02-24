//This program creates the Pong game. To control the left player, use the W and S keys.
//To control the right player, use the up and down arrows. Press the spacebar to exit.
//Payal Naresh

#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include <ctime>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
glm::mat4 viewMatrix, projectionMatrix;
glm::mat4 modelMatrixPlayer1, modelMatrixPlayer2, modelMatrixBall;
glm::vec3 player1_position = glm::vec3(-4.87, 0, 0);
glm::vec3 player1_movement = glm::vec3(0, 0, 0);
glm::vec3 player2_position = glm::vec3(4.87, 0, 0);
glm::vec3 player2_movement = glm::vec3(0, 0, 0);

glm::vec3 ball_position = glm::vec3(0, 0, 0);
glm::vec3 ball_movement = glm::vec3(0, 0, 0);

float player_height = 1.0f;
float player_width = 0.30f;
float ball_width = 0.30f;
float ball_height = 0.30f;
float screen_width = 10.0f;
float screen_height = 7.50f;

float player1_speed = 4.0f;
float player2_speed = 4.0f;
float ball_speed = 4.5f;

GLuint player1TextureID, player2TextureID;
GLuint ballTextureID;

GLuint LoadTexture(const char* filePath) {
    int w, h, n;
    unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);

    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);
    return textureID;
}

//Function to determine which direction the ball will initially move in.
int getInitialBallDirection() {
    srand((time(NULL)));
    int randNum1 = (rand() % 2);
    int xBallMov = 1;
    if (randNum1 == 0) {
        xBallMov = -1;
    }
    return xBallMov;
}

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Pong!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif
    glViewport(0, 0, 640, 480);
    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");
    viewMatrix = glm::mat4(1.0f);
    modelMatrixPlayer1 = glm::mat4(1.0f);
    modelMatrixPlayer2 = glm::mat4(1.0f);
    modelMatrixBall = glm::mat4(1.0f);

    //Get the ball's initial direction (either towards player left or right)
    int dir = getInitialBallDirection();
    ball_movement = glm::vec3(dir, 0, 0);

    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    program.SetColor(1.0f, 0.0f, 0.0f, 1.0f);

    glUseProgram(program.programID);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    player1TextureID = LoadTexture("playerRedFinal.png");
    player2TextureID = LoadTexture("playerBlueFinal.png");
    ballTextureID = LoadTexture("ballYellowFinal.png");
}

void ProcessInput() {
    player1_movement = glm::vec3(0);
    player2_movement = glm::vec3(0);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            gameIsRunning = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_SPACE:
                gameIsRunning = false;
            }
            break; // SDL_KEYDOWN
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_W]) {
        player1_movement.y = 1.0f;
    }
    else if (keys[SDL_SCANCODE_S]) {
        player1_movement.y = -1.0f;
    }
    if (glm::length(player1_movement) > 1.0f) {
        player1_movement = glm::normalize(player1_movement);
    }

    if (keys[SDL_SCANCODE_UP]) {
        player2_movement.y = 1.0f;
    }
    else if (keys[SDL_SCANCODE_DOWN]) {
        player2_movement.y = -1.0f;
    }
    if (glm::length(player2_movement) > 1.0f) {
        player2_movement = glm::normalize(player2_movement);
    }
}

//This function checks if there was a collision between a player and the ball,
//and returns a boolean
bool Collision(glm::vec3 player_position, glm::vec3 ball_position) {
    float x1 = player_position.x;
    float y1 = player_position.y;
    float x2 = ball_position.x;
    float y2 = ball_position.y;

    float xdist = fabs(x2 - x1) - ((player_width + ball_width) / 2.0f);
    float ydist = fabs(y2 - y1) - ((player_height + ball_height) / 2.0f);

    if (xdist < 0 && ydist < 0) {
        return true;
    }
    return false;
}

//This function randomly picks the direction the ball should go in after hitting
//one of the players.
double pickDirection() {
    double myDirections[] = { 0.1, 0.15, 0.20, 0.25, -0.1, -0.15, -0.2, -0.25 };
    int dirIndex;
    srand(unsigned(time(NULL)));
    dirIndex = rand() % 8;
    return myDirections[dirIndex];
}

float lastTicks = 0.0f;

void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;

    //Only do this is if the ball has not touched the side walls (i.e nobody had lost yet)
    if (ball_position.x + (ball_width / 2) < (screen_width / 2) && ball_position.x - (ball_width / 2) > -(screen_width / 2)) {
        //Make sure player left is between the top and bottom screen (within bounds)
        if (player1_position.y + (player_height / 2) < (screen_height / 2) && player1_position.y - (player_height / 2) > -(screen_height / 2)) {
            player1_position += player1_movement * player1_speed * deltaTime;
            modelMatrixPlayer1 = glm::mat4(1.0f);
            modelMatrixPlayer1 = glm::translate(modelMatrixPlayer1, player1_position);
        }
        //If player left is out of bounds
        if (player1_position.y + (player_height / 2) > (screen_height / 2) || player1_position.y - (player_height / 2) < -(screen_height / 2)) {
            player1_movement.y *= -1;
            player1_position += player1_movement * player1_speed * deltaTime;
            modelMatrixPlayer1 = glm::mat4(1.0f);
            modelMatrixPlayer1 = glm::translate(modelMatrixPlayer1, player1_position);
        }
        //Make sure player right is between the top and bottom screen (within bounds)
        if (player2_position.y + (player_height / 2) < (screen_height / 2) && player2_position.y - (player_height / 2) > -(screen_height / 2)) {
            player2_position += player2_movement * player2_speed * deltaTime;
            modelMatrixPlayer2 = glm::mat4(1.0f);
            modelMatrixPlayer2 = glm::translate(modelMatrixPlayer2, player2_position);
        }
        //If player right is out of bounds
        if (player2_position.y + (player_height / 2) > (screen_height / 2) || player2_position.y - (player_height / 2) < -(screen_height / 2)) {
            player2_movement.y *= -1;
            player2_position += player2_movement * player2_speed * deltaTime;
            modelMatrixPlayer2 = glm::mat4(1.0f);
            modelMatrixPlayer2 = glm::translate(modelMatrixPlayer2, player2_position);
        }

        ball_position += ball_movement * ball_speed * deltaTime;
        modelMatrixBall = glm::mat4(1.0f);
        modelMatrixBall = glm::translate(modelMatrixBall, ball_position);

        //If player left hits the ball
        if (Collision(player1_position, ball_position) == true) {
            ball_movement.x *= -1;
            ball_movement.y += pickDirection();
            ball_position += ball_movement * ball_speed * deltaTime;
            modelMatrixBall = glm::mat4(1.0f);
            modelMatrixBall = glm::translate(modelMatrixBall, ball_position);
        }
        //If player right hits the ball
        if (Collision(player2_position, ball_position) == true) {
            ball_movement.x *= -1;
            ball_movement.y += pickDirection();
            ball_position += ball_movement * ball_speed * deltaTime;
            modelMatrixBall = glm::mat4(1.0f);
            modelMatrixBall = glm::translate(modelMatrixBall, ball_position);
        }

        //If the ball hits the top or bottom screen, change it's direction
        if ((ball_position.y + (ball_height / 2) > (screen_height / 2)) || (ball_position.y - (ball_height / 2) < -(screen_height / 2))) {
            srand((time(NULL)));
            float randNum = (rand() % 2) - 0.5;
            ball_movement.y *= -1;
            ball_position += ball_movement * ball_speed * deltaTime;
            modelMatrixBall = glm::mat4(1.0f);
            modelMatrixBall = glm::translate(modelMatrixBall, ball_position);
        }
    }

    //Player left won, everything stops
    if (ball_position.x + (ball_width / 2) > (screen_width / 2)) {
        ball_movement.x, ball_movement.y = (0, 0);
    }
    //Player right won, everything stops
    if (ball_position.x - (ball_width / 2) < -(screen_width / 2)) {
        ball_movement.x, ball_movement.y = (0, 0);
    }
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);

    program.SetModelMatrix(modelMatrixPlayer1);

    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);

    glBindTexture(GL_TEXTURE_2D, player1TextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

    program.SetModelMatrix(modelMatrixPlayer2);

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);

    glBindTexture(GL_TEXTURE_2D, player2TextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

    program.SetModelMatrix(modelMatrixBall);

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);

    glBindTexture(GL_TEXTURE_2D, ballTextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

    SDL_GL_SwapWindow(displayWindow);
}

void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();

    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }

    Shutdown();
    return 0;
}