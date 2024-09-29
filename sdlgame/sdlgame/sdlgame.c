#include <stdio.h>
#include <SDL.h>
#include <stdbool.h>
#include <SDL_ttf.h>
#include <stdlib.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int PONG_RADIUS = 10;
const int PADDLE_HEIGHT = 80;
const int PADDLE_WIDTH = 20;

void DrawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; 
            int dy = radius - h; 
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
            }
        }
    }
}

void CalculatePongDirection(int p1y, int p2y, int pongPosx, int pongPosy, int *pongvx, int *pongvy) {
    // Check if pong touches top or bottom of window
    if (pongPosy < PONG_RADIUS / 2) {
        *pongvy = -*pongvy;
    }
    if (pongPosy > WINDOW_HEIGHT - (PONG_RADIUS / 2)) {
        *pongvy = -*pongvy;
    }
    // Check if pong touches paddle
    if (pongPosx > (WINDOW_WIDTH - PADDLE_WIDTH) && pongPosy > p2y  && pongPosy < p2y + (PADDLE_HEIGHT)) {
        *pongvx = -*pongvx;
    }
    if (pongPosx < PADDLE_WIDTH && pongPosy > p1y && pongPosy < p1y + (PADDLE_HEIGHT)) {
        *pongvx = -*pongvx;
    }
}

void CheckPongInWindow(int pongPosx, int pongPosy, int* gameOver) {
    // Check if pong is out of bounds
    if (pongPosx > WINDOW_WIDTH) {
        *gameOver = 1;
    }
    if (pongPosx < 0) {
        *gameOver = 2;       
    }
}

void createTextTexture(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color, SDL_Texture** texture, SDL_Rect* rect) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    *texture = SDL_CreateTextureFromSurface(renderer, surface);
    
    rect->x = (WINDOW_WIDTH - surface->w) / 2; 
    rect->y = (WINDOW_HEIGHT - surface->h) / 2; 
    rect->w = surface->w;
    rect->h = surface->h;

    SDL_FreeSurface(surface);
}

// Random number for start velocity of pong
int generateRandomInt(int min, int max) {
    int r = (rand() % (max - min + 1)) + min;
    // if even then flip sign (so that the pong can start in the direction of other player)
    if (r % 2 == 0) {
        r = -r;
    }
    return r;
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    Uint8* keyState;
    Uint32 previousTime = SDL_GetTicks();
    int pongvx = generateRandomInt(4,6);
    int pongvy = generateRandomInt(1,4);
    int pongPosx = WINDOW_WIDTH/2;
    int pongPosy = WINDOW_HEIGHT/2;
    int gameOver = 0;
    bool startScreen = true;
    bool p1MovingUp = false;
    bool p1MovingDown = false;
    bool p2MovingUp = false;
    bool p2MovingDown = false;
    bool restartPressed = false;
    bool startPressed = false;
    bool quitPressed = false;

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Pong",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);
       
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    // Create text
    TTF_Init();
    //TTF_Font* font = TTF_OpenFont("C:/Users/julia/Downloads/Pixelify_Sans/pixelfont.ttf", 32);
    TTF_Font* font = TTF_OpenFont("pixelfont.ttf", 32);
    if (!font) {
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return -1;
    }
    
    SDL_Color textColour = { 255, 255, 255, 255 };

    SDL_Texture* gameOverText;
    SDL_Rect gameOverTextRect;
    createTextTexture(renderer, font, "GAME OVER!", textColour, &gameOverText, &gameOverTextRect);
    gameOverTextRect.y -= 50;

    SDL_Texture* restartText;
    SDL_Rect restartTextRect;
    createTextTexture(renderer, font, "Press R to Restart", textColour, &restartText, &restartTextRect);
    restartTextRect.y += 50;

    SDL_Texture* quitText;
    SDL_Rect quitTextRect;
    createTextTexture(renderer, font, "Press Q to quit", textColour, &quitText, &quitTextRect);
    quitTextRect.y += 100;

    SDL_Texture* winner1Text;
    SDL_Rect winner1TextRect;
    createTextTexture(renderer, font, "Player 1 Wins", textColour, &winner1Text, &winner1TextRect);

    SDL_Texture* winner2Text;
    SDL_Rect winner2TextRect;
    createTextTexture(renderer, font, "Player 2 Wins", textColour, &winner2Text, &winner2TextRect);

    SDL_Texture* startPongText;
    SDL_Rect startPongTextRect;
    createTextTexture(renderer, font, "Welcome To Pong!", textColour, &startPongText, &startPongTextRect);
    startPongTextRect.y -= 50;

    SDL_Texture* startInstructions1Text;
    SDL_Rect startInstructions1TextRect;
    createTextTexture(renderer, font, "Player 1 moves with W and S", textColour, &startInstructions1Text, &startInstructions1TextRect);

    SDL_Texture* startInstructions2Text;
    SDL_Rect startInstructions2TextRect;
    createTextTexture(renderer, font, "Player 2 moves with UP and DOWN arrows", textColour, &startInstructions2Text, &startInstructions2TextRect);
    startInstructions2TextRect.y += 50;

    SDL_Texture* beginText;
    SDL_Rect beginTextRect;
    createTextTexture(renderer, font, "Press SPACE to start the game", textColour, &beginText, &beginTextRect);
    beginTextRect.y += 100;

    // Create paddle rectangles
    SDL_Rect p1;
    p1.x = 0; 
    p1.y = (WINDOW_HEIGHT - 50) / 2; 
    p1.w = 20;  
    p1.h = 80;  
    SDL_Rect p2;
    p2.x = WINDOW_WIDTH-20; 
    p2.y = (WINDOW_HEIGHT - 50) / 2; 
    p2.w = 20;  
    p2.h = 80;  

    // Main loop
    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            // Handle Inputs
            keyState = SDL_GetKeyboardState(NULL);
            p1MovingUp = (keyState[SDL_SCANCODE_W] > 0);
            p1MovingDown = (keyState[SDL_SCANCODE_S] > 0);
            p2MovingUp = (keyState[SDL_SCANCODE_UP] > 0);
            p2MovingDown = (keyState[SDL_SCANCODE_DOWN] > 0);
            restartPressed = (keyState[SDL_SCANCODE_R] > 0);
            startPressed = (keyState[SDL_SCANCODE_SPACE] > 0);
            quitPressed = (keyState[SDL_SCANCODE_Q] > 0);
            
        }
        // Calculate delta time
        Uint32 currentTime = SDL_GetTicks();
        int deltaTime = (currentTime - previousTime) / 10;
        previousTime = currentTime;

        // Draw black background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Show start screen
        if (startScreen) {
            if (startPressed) startScreen = false;
            SDL_RenderCopy(renderer, startPongText, NULL, &startPongTextRect);
            SDL_RenderCopy(renderer, startInstructions1Text, NULL, &startInstructions1TextRect);
            SDL_RenderCopy(renderer, startInstructions2Text, NULL, &startInstructions2TextRect);
            SDL_RenderCopy(renderer, beginText, NULL, &beginTextRect);
        }
        // Show gameover screen
        else if (gameOver) {
            if (quitPressed) running = 0;
            if (restartPressed) {
                // Reset paddles and pong
                pongPosx = WINDOW_WIDTH / 2;
                pongPosy = WINDOW_HEIGHT / 2;
                p1.y = (WINDOW_HEIGHT - 50) / 2;
                p2.y = (WINDOW_HEIGHT - 50) / 2;
                pongvx = generateRandomInt(4, 6);;
                pongvy = generateRandomInt(1, 4);;
                gameOver = 0;
            }
            SDL_RenderCopy(renderer, gameOverText, NULL, &gameOverTextRect);
            if(gameOver==1)
                SDL_RenderCopy(renderer, winner1Text, NULL, &winner1TextRect);
            if(gameOver==2)
                SDL_RenderCopy(renderer, winner2Text, NULL, &winner2TextRect);
            SDL_RenderCopy(renderer, restartText, NULL, &restartTextRect);
            SDL_RenderCopy(renderer, quitText, NULL, &quitTextRect);
        }
        // Show game screen
        else {
            // Move players
            if (p1MovingDown && p1.y < WINDOW_HEIGHT - 80) {
                p1.y += 10 * deltaTime;
            }
            if (p1MovingUp && p1.y > 0) {
                p1.y -= 10 * deltaTime;
            }
            if (p2MovingDown && p2.y < WINDOW_HEIGHT - 80) {
                p2.y += 10 * deltaTime;
            }
            if (p2MovingUp && p2.y > 0) {
                p2.y -= 10 * deltaTime;
            }

            // Move Pong
            CalculatePongDirection(p1.y, p2.y, pongPosx, pongPosy, &pongvx, &pongvy);
            pongPosx += pongvx * deltaTime;
            pongPosy += pongvy * deltaTime;

            // Check if pong was missed by paddle
            CheckPongInWindow(pongPosx, pongPosy, &gameOver);

            // Set drawing colour to white
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

            // Render the rectangles
            SDL_RenderFillRect(renderer, &p1);
            SDL_RenderFillRect(renderer, &p2);

            // Render Pong
            DrawCircle(renderer, pongPosx, pongPosy, PONG_RADIUS);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Clean up
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

