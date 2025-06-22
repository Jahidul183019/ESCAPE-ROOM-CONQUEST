#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <thread>
#include <SDL2/SDL_mixer.h>
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
int WORLD_WIDTH = 1600;
int WORLD_HEIGHT = 1200;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* backgroundTexture = nullptr;
SDL_Texture* playerTexture = nullptr;

SDL_Rect player = {50, 100, 64, 64};
SDL_Rect camera = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

SDL_Rect obstacles[] = {
    {300, 280, 450, 400},   
        
};
int redCount = sizeof(obstacles) / sizeof(obstacles[0]);
SDL_Rect obstacles1[] = {
    {200, 80, 100, 80},   
    
};
SDL_Rect obstacles3[] = {
    {720, 80, 100, 80},   
        
};
int Count3 = sizeof(obstacles3) / sizeof(obstacles3[0]);

SDL_Rect obstacles2[] = {
    {450, 80, 100, 80},   
        
};
int Count2 = sizeof(obstacles2) / sizeof(obstacles2[0]);

int count1 = sizeof(obstacles1) / sizeof(obstacles1[0]);

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << "\n";
        return false;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cout << "SDL_image could not initialize! IMG Error: " << IMG_GetError() << "\n";
        return false;
    }
    window = SDL_CreateWindow("Main Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    return window && renderer;
}

bool loadMedia() {
    backgroundTexture = IMG_LoadTexture(renderer, "background.png");
    playerTexture = IMG_LoadTexture(renderer, "player.png");
    if (backgroundTexture) SDL_QueryTexture(backgroundTexture, NULL, NULL, &WORLD_WIDTH, &WORLD_HEIGHT);
    return backgroundTexture && playerTexture;
}

bool canMove(int x, int y) {
    SDL_Rect tempPlayer = player;
    tempPlayer.x += x;
    tempPlayer.y += y;

    // Check collision with each obstacle in obstacles
    for (int i = 0; i < redCount; i++) {
        if (SDL_HasIntersection(&tempPlayer, &obstacles[i])) {
            return false;
        }
    }
    // Check collision with each obstacle in obstacles1
    for (int i = 0; i < count1; i++) {
        if (SDL_HasIntersection(&tempPlayer, &obstacles1[i])) {
            return false;
        }
    }
    // Check collision with each obstacle in obstacles2
    for (int i = 0; i < Count2; i++) {
        if (SDL_HasIntersection(&tempPlayer, &obstacles2[i])) {
            return false;
        }
    }
    // Check collision with each obstacle in obstacles3
    for (int i = 0; i < Count3; i++) {
        if (SDL_HasIntersection(&tempPlayer, &obstacles3[i])) {
            return false;
        }
    }
    return true;
}



void handleInput(SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        bool canMovePlayer = true;

        switch (e.key.keysym.sym) {
            case SDLK_LEFT:
                canMovePlayer = canMove(-10, 0);
                if (canMovePlayer) {player.x -= 10;if(player.x<0){player.x=0;}if(player.x>950){player.x=950;}}
                break;
            case SDLK_RIGHT:
                canMovePlayer = canMove(10, 0);
                if (canMovePlayer) {player.x += 10;if(player.x<0){player.x=0;}if(player.x>950){player.x=950;}}
                break;
            case SDLK_UP:
                canMovePlayer = canMove(0, -10);
                if (canMovePlayer) {player.y -= 10;if(player.y<0){player.y=0;}if(player.y>900){player.y=900;}}
                break;
            case SDLK_DOWN:
                canMovePlayer = canMove(0, 10);
                if (canMovePlayer) {player.y += 10;if(player.y<0){player.y=0;}if(player.y>900){player.y=900;}}
                break;
        }
    }
}

void updateCamera() {
    camera.x = player.x + player.w / 2 - camera.w / 2;
    camera.y = player.y + player.h / 2 - camera.h / 2;
    if (camera.x < 0) camera.x = 0;
    if (camera.y < 0) camera.y = 0;
    if (camera.x > WORLD_WIDTH - camera.w) camera.x = WORLD_WIDTH - camera.w;
    if (camera.y > WORLD_HEIGHT - camera.h) camera.y = WORLD_HEIGHT - camera.h;
}

void render() {
    SDL_RenderClear(renderer);
    SDL_Rect bgSrcRect = camera;
    SDL_Rect bgDstRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, backgroundTexture, &bgSrcRect, &bgDstRect);

    SDL_Rect playerOnScreen = {player.x - camera.x, player.y - camera.y, player.w, player.h};
    SDL_RenderCopy(renderer, playerTexture, NULL, &playerOnScreen);

    SDL_RenderPresent(renderer);
    
}

void cleanUp() {
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

void runPuzzle1() {
    system("g++ Puzzle.cpp -o Puzzle \
  $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf SDL2_mixer) \
  -std=c++11 && ./Puzzle &");
}

void runPuzzle2() {
    system("g++ RSADecryptor.cpp -o RSADecryptor \
  $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf SDL2_mixer) \
  -std=c++11 && ./RSADecryptor &");
}

void handleClick(int mx, int my) {
    SDL_Rect clickPoint = {mx + camera.x, my + camera.y, 1, 1};
    if (SDL_HasIntersection(&clickPoint,obstacles1)) {
        std::thread(runPuzzle1).detach();
    } else if (SDL_HasIntersection(&clickPoint, obstacles2)) {
        std::thread(runPuzzle2).detach();
    }
}

int main(int argc, char* argv[]) {
    if (!init() || !loadMedia()) return -1;
    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            handleInput(e);
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                handleClick(mx, my);
            }
        }
        updateCamera();
        render();
    }
    cleanUp();
    return 0;
}