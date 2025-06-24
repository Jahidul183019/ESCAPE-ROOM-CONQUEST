#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <cmath>

const int WIN_W = 800, WIN_H = 600;
const int COMP_COUNT = 6;
const SDL_Point targetSlots[COMP_COUNT] = {
    {124, 457}, {530, 178}, {534, 282}, {433, 494}, {536, 452}, {125, 305}
};
const char* fileNames[COMP_COUNT] = {
    "battery.png",
    "resistor.png",
    "capacitor.png",
    "diode.png",
    "voltmeter.png",
    "ammeter.png"
};

bool isNear(int x1, int y1, int x2, int y2, int range = 40) {
    return (std::abs(x1 - x2) < range && std::abs(y1 - y2) < range);
}

int main(int argc, char* argv[]) {
    // Initialize SDL subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        return 1;
    }
    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        return 1;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Mix_OpenAudio Error: " << Mix_GetError() << std::endl;
        return 1;
    }

    // Load sound effects (place your .wav files in assets/)
    Mix_Chunk* pickSound    = Mix_LoadWAV("pick.mp3");
    Mix_Chunk* placeSound   = Mix_LoadWAV("place.mp3");
    Mix_Chunk* successSound = Mix_LoadWAV("success_circuit.mp3");
    Mix_Chunk* failSound    = Mix_LoadWAV("fail_circuit.mp3");
    if (!pickSound || !placeSound || !successSound || !failSound) {
        std::cerr << "Failed to load one or more sound effects: " << Mix_GetError() << std::endl;
    }

    // Load a simple startup sound
    Mix_Chunk* cSound = Mix_LoadWAV("circuit_background.wav");
    if (cSound) {
        Mix_VolumeChunk(cSound, MIX_MAX_VOLUME);
        Mix_PlayChannel(-1, cSound, 0);
    }

    SDL_Window* window = SDL_CreateWindow("Circuit Puzzle Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* font    = TTF_OpenFont("arial.ttf", 24);

    SDL_Texture* background = IMG_LoadTexture(ren, "circuit_background.png");
    SDL_Texture* ledTex      = IMG_LoadTexture(ren, "led.png");
    SDL_Texture* compTex[COMP_COUNT];
    for (int i = 0; i < COMP_COUNT; ++i) {
        compTex[i] = IMG_LoadTexture(ren, fileNames[i]);
        if (!compTex[i]) std::cerr << "IMG_LoadTexture Error: " << IMG_GetError() << std::endl;
    }

    struct Comp { SDL_Rect rect; bool placed; };
    std::vector<Comp> comps(COMP_COUNT);
    for (int i = 0; i < COMP_COUNT; ++i) {
        comps[i].rect   = {100 + i * 100, 400, 64, 64};
        comps[i].placed = false;
    }

    bool quit = false;
    bool paused = false, solved = false;
    bool dragging = false;
    int dragged = -1;
    int offsetX = 0, offsetY = 0;

    Uint32 startTicks = SDL_GetTicks();
    Uint32 pausedTicks = 0;
    const int TIME_LIMIT = 60;
    std::string unlockMsg;

    SDL_Color white = {255,255,255,255};
    SDL_Color black = {0,0,0,255};

    std::srand((unsigned)std::time(nullptr));
    SDL_StartTextInput();

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { quit = true; break; }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE && !solved) {
                paused = !paused;
                if (paused) pausedTicks = SDL_GetTicks() - startTicks;
                else        startTicks  = SDL_GetTicks() - pausedTicks;
            }

            if (!paused && !solved) {
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                    int mx = e.button.x, my = e.button.y;
                    for (int i = 0; i < COMP_COUNT; ++i) {
                        SDL_Rect& r = comps[i].rect;
                        if (mx > r.x && mx < r.x + r.w && my > r.y && my < r.y + r.h) {
                            dragging = true;
                            dragged  = i;
                            offsetX  = mx - r.x;
                            offsetY  = my - r.y;
                            Mix_PlayChannel(-1, pickSound, 0);  // play pick-up sound
                            break;
                        }
                    }
                }
                if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                    if (dragged != -1) {
                        // if dropped near slot, play place sound
                        if (isNear(comps[dragged].rect.x, comps[dragged].rect.y,
                                   targetSlots[dragged].x, targetSlots[dragged].y)) {
                            Mix_PlayChannel(-1, placeSound, 0);
                        }
                    }
                    dragging = false;
                    dragged = -1;
                }
                if (e.type == SDL_MOUSEMOTION && dragging && dragged != -1) {
                    comps[dragged].rect.x = e.motion.x - offsetX;
                    comps[dragged].rect.y = e.motion.y - offsetY;
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        int secLeft;
        if (paused || solved) secLeft = TIME_LIMIT - (int)(pausedTicks / 1000);
        else {
            pausedTicks = now - startTicks;
            secLeft = TIME_LIMIT - (int)(pausedTicks / 1000);
        }

        if (!paused && !solved && secLeft <= 0) {
            solved = true;
            unlockMsg = "Time's up! Try again.";
            Mix_PlayChannel(-1, failSound, 0);  // play failure sound
        }

        if (!solved) {
            bool allPlaced = true;
            for (int i = 0; i < COMP_COUNT; ++i) {
                if (isNear(comps[i].rect.x, comps[i].rect.y,
                           targetSlots[i].x, targetSlots[i].y)) {
                    comps[i].placed = true;
                } else {
                    comps[i].placed = false;
                    allPlaced = false;
                }
            }
            if (allPlaced) {
                solved = true;
                int keyNum = 1000 + std::rand() % 9000;
                unlockMsg = "Puzzle Solved! Unlock Key: " + std::to_string(keyNum);
                Mix_PlayChannel(-1, successSound, 0);  // play success sound
            }
        }

        // Render everything
        SDL_SetRenderDrawColor(ren, 20,20,20,255);
        SDL_RenderClear(ren);

        if (background) {
            SDL_Rect bgRect = {0,0,WIN_W,WIN_H};
            SDL_RenderCopy(ren, background, NULL, &bgRect);
        }
        
        if (ledTex) {
            SDL_Rect lr = {378,43,60,60};
            SDL_SetTextureColorMod(ledTex, solved ? 0 : 100,
                                               solved ? 255 : 100,
                                               solved ? 0 : 100);
            SDL_RenderCopy(ren, ledTex, NULL, &lr);
        }

        for (int i = 0; i < COMP_COUNT; ++i) {
            if (compTex[i]) SDL_RenderCopy(ren, compTex[i], NULL, &comps[i].rect);
        }

        // Draw timer
        {
            std::stringstream ss;
            ss << "Time Left: " << (secLeft > 0 ? secLeft : 0) << "s";
            SDL_Surface* ts = TTF_RenderText_Blended(font, ss.str().c_str(), black);
            if (ts) {
                SDL_Texture* tt = SDL_CreateTextureFromSurface(ren, ts);
                SDL_Rect tr = {10,10,ts->w,ts->h};
                SDL_RenderCopy(ren, tt, NULL, &tr);
                SDL_FreeSurface(ts);
                SDL_DestroyTexture(tt);
            }
        }

        if (solved) {
            SDL_Surface* rs = TTF_RenderText_Blended(font, unlockMsg.c_str(), white);
            if (rs) {
                SDL_Texture* rt = SDL_CreateTextureFromSurface(ren, rs);
                SDL_Rect rr = {150,500,rs->w,rs->h};
                SDL_RenderCopy(ren, rt, NULL, &rr);
                SDL_FreeSurface(rs);
                SDL_DestroyTexture(rt);
            }
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    // Cleanup
    SDL_StopTextInput();
    Mix_FreeChunk(pickSound);
    Mix_FreeChunk(placeSound);
    Mix_FreeChunk(successSound);
    Mix_FreeChunk(failSound);
    Mix_FreeChunk(cSound);
    Mix_CloseAudio();
    TTF_CloseFont(font);
    for (int i = 0; i < COMP_COUNT; ++i) SDL_DestroyTexture(compTex[i]);
    if (ledTex)      SDL_DestroyTexture(ledTex);
    if (background)  SDL_DestroyTexture(background);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
