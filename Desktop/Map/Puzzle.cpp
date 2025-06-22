#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

const int SCREEN_WIDTH      = 1024;
const int SCREEN_HEIGHT     = 768;
const int PUZZLE_TIME_LIMIT = 30;

struct Puzzle {
    string question;
    string answer;
};

SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font,
                        const string& text, SDL_Color color,
                        SDL_Rect& rectOut, int wrapLength = 800)
{
    SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(font,
        text.c_str(), color, wrapLength);
    if (!surface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    rectOut = { 0, 0, surface->w, surface->h };
    SDL_FreeSurface(surface);
    return texture;
}

SDL_Texture* loadTexture(SDL_Renderer* renderer, const string& path) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        cerr << "Failed to load image: " << IMG_GetError() << endl;
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

int main(int argc, char* argv[]) {
    // Initialize SDL, TTF, IMG
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 ||
        TTF_Init() < 0 ||
        (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0))
    {
        cerr << "Failed to initialize SDL/TTF/IMG\n";
        return 1;
    }

    // Initialize SDL_mixer
    int mixFlags = MIX_INIT_OGG | MIX_INIT_MP3;
    if ((Mix_Init(mixFlags) & mixFlags) != mixFlags) {
        cerr << "Mix_Init failed: " << Mix_GetError() << "\n";
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        cerr << "Mix_OpenAudio failed: " << Mix_GetError() << "\n";
    }

    // Create window + renderer
    SDL_Window*   window   = SDL_CreateWindow("Deadline Decoder",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        cerr << "Failed to create window/renderer\n";
        return 1;
    }

    // Load font
    TTF_Font* font = TTF_OpenFont("impact.ttf", 24);
    if (!font) {
        cerr << "Failed to load font\n";
        return 1;
    }

    // Load background image
    SDL_Texture* bgTexture = loadTexture(renderer, "puzzleimage.png");

    // Load audio assets
    Mix_Music* bgm         = Mix_LoadMUS("puzzleGame.wav");
    Mix_Chunk* correctSfx  = Mix_LoadWAV("correct.mp3");
    Mix_Chunk* wrongSfx    = Mix_LoadWAV("wrong.mp3");
    if (!bgm)        cerr << "bgm load error: " << Mix_GetError() << "\n";
    if (!correctSfx) cerr << "correctSfx load error: " << Mix_GetError() << "\n";
    if (!wrongSfx)   cerr << "wrongSfx load error: " << Mix_GetError() << "\n";

    // Start background music
    Mix_PlayMusic(bgm, -1);

    SDL_Color white = {255,255,255,255};
    SDL_Event e;

    // --- Name entry ---
    string playerName;
    bool enteringName = true;
    SDL_StartTextInput();
    while (enteringName) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                SDL_StopTextInput();
                return 0;
            }
            if (e.type == SDL_TEXTINPUT) {
                playerName += e.text.text;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_BACKSPACE && !playerName.empty())
                    playerName.pop_back();
                else if (e.key.keysym.sym == SDLK_RETURN && !playerName.empty())
                    enteringName = false;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0,0,0,255);
        SDL_RenderClear(renderer);

        SDL_Rect r;
        SDL_Texture* t1 = renderText(renderer, font,
            "Enter your name to begin:", white, r);
        r.x = (SCREEN_WIDTH - r.w)/2; r.y = 250;
        SDL_RenderCopy(renderer, t1, nullptr, &r);
        SDL_DestroyTexture(t1);

        SDL_Texture* t2 = renderText(renderer, font,
            playerName, white, r);
        r.x = (SCREEN_WIDTH - r.w)/2; r.y = 320;
        SDL_RenderCopy(renderer, t2, nullptr, &r);
        SDL_DestroyTexture(t2);

        SDL_RenderPresent(renderer);
    }
    SDL_StopTextInput();

    // --- Puzzle data ---
    vector<Puzzle> puzzles = {
        {"I have keys but no locks, I have space but no room. What am I?", "keyboard"},
        {"What has to be broken before you use it?", "egg"},
        {"The more you take, the more you leave behind. What am I?", "footsteps"}
    };

    int   currentPuzzle  = -1;
    bool  running        = true;
    bool  puzzleStarted  = false;
    bool  puzzleSolved   = false;
    bool  puzzleFailed   = false;
    Uint32 puzzleStartTime = 0;
    string userInput;

    SDL_Rect monitorTouchArea = {320,256,512,320};

    // --- Main loop ---
    while (running) {
        // Start text input when puzzle starts
        if (puzzleStarted && !SDL_IsTextInputActive()) {
            SDL_StartTextInput();
        }
        if ((!puzzleStarted || puzzleSolved || puzzleFailed) && SDL_IsTextInputActive()) {
            SDL_StopTextInput();
        }

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            // Start puzzle on click
            if (!puzzleStarted && e.type == SDL_MOUSEBUTTONDOWN) {
                int mx,my; SDL_GetMouseState(&mx,&my);
                if (mx>=monitorTouchArea.x && mx<=monitorTouchArea.x + monitorTouchArea.w &&
                    my>=monitorTouchArea.y && my<=monitorTouchArea.y + monitorTouchArea.h)
                {
                    currentPuzzle = 0;
                    puzzleStarted = true;
                    puzzleSolved  = false;
                    puzzleFailed  = false;
                    userInput.clear();
                    puzzleStartTime = SDL_GetTicks();
                }
            }

            // Answer entry (text input)
            if (puzzleStarted && !puzzleSolved && !puzzleFailed) {
                if (e.type == SDL_TEXTINPUT) {
                    userInput += e.text.text;
                }
                if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_BACKSPACE && !userInput.empty())
                        userInput.pop_back();
                    else if (e.key.keysym.sym == SDLK_RETURN) {
                        if (userInput == puzzles[currentPuzzle].answer) {
                            Mix_PlayChannel(-1, correctSfx, 0);
                            puzzleSolved = true;
                        }
                    }
                }
            }

            // Next puzzle or end
            if ((puzzleSolved || puzzleFailed) && e.type==SDL_KEYDOWN
                && e.key.keysym.sym == SDLK_SPACE)
            {
                currentPuzzle++;
                if (currentPuzzle < (int)puzzles.size()) {
                    puzzleStarted = true;
                    puzzleSolved  = false;
                    puzzleFailed  = false;
                    userInput.clear();
                    puzzleStartTime = SDL_GetTicks();
                } else {
                    running = false;
                }
            }
        }

        // Timeout check
        if (puzzleStarted && !puzzleSolved && !puzzleFailed) {
            Uint32 now = SDL_GetTicks();
            int secondsLeft = PUZZLE_TIME_LIMIT - (now - puzzleStartTime)/1000;
            if (secondsLeft <= 0) {
                Mix_PlayChannel(-1, wrongSfx, 0);
                puzzleFailed = true;
            }
        }

        // Rendering
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderClear(renderer);
        if (bgTexture) SDL_RenderCopy(renderer, bgTexture, nullptr, nullptr);

        SDL_Rect r; 
        SDL_Texture* txt = nullptr;

        if (!puzzleStarted) {
            txt = renderText(renderer, font,
                "Click the screen to start the puzzle...,You can only use small hand letters to answer the questions", white, r);
            r.x = (SCREEN_WIDTH - r.w)/2; r.y = SCREEN_HEIGHT - 100;
        }
        else if (puzzleSolved) {
            txt = renderText(renderer, font,
                "Correct! Press SPACE for next puzzle.", white, r);
            r.x = (SCREEN_WIDTH - r.w)/2; r.y = 100;
        }
        else if (puzzleFailed) {
            txt = renderText(renderer, font,
                "Time's up! Press SPACE to try next puzzle.", white, r);
            r.x = (SCREEN_WIDTH - r.w)/2; r.y = 100;
        }
        else {
            Uint32 now = SDL_GetTicks();
            int secondsLeft = PUZZLE_TIME_LIMIT - (now - puzzleStartTime)/1000;
            string full = puzzles[currentPuzzle].question
                        + "\n\nYour Answer: " + userInput
                        + "\n\nTime Left: " + to_string(secondsLeft);
            txt = renderText(renderer, font, full, white, r);
            r.x = (SCREEN_WIDTH - r.w)/2; r.y = 100;
        }

        if (txt) {
            SDL_RenderCopy(renderer, txt, nullptr, &r);
            SDL_DestroyTexture(txt);
        }

        // Welcome banner
        SDL_Texture* wtxt = renderText(renderer, font,
            "Welcome, " + playerName + "!", white, r);
        r.x = SCREEN_WIDTH - r.w - 20;
        r.y = 20;
        SDL_RenderCopy(renderer, wtxt, nullptr, &r);
        SDL_DestroyTexture(wtxt);

        SDL_RenderPresent(renderer);
    }

    // Cleanup game screen
    SDL_DestroyTexture(bgTexture);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Decryptor unlocked screen
    SDL_Window*   winWindow   = SDL_CreateWindow("Decryptor Unlocked",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800,600,SDL_WINDOW_SHOWN);
    SDL_Renderer* winRenderer = SDL_CreateRenderer(winWindow, -1,
        SDL_RENDERER_ACCELERATED);
    SDL_Texture*  winImage    = loadTexture(winRenderer, "decryptor.png");

    bool showing = true;
    while (showing) {
        while (SDL_PollEvent(&e)) {
            if (e.type==SDL_QUIT || e.type==SDL_KEYDOWN) showing = false;
        }
        SDL_SetRenderDrawColor(winRenderer,0,0,0,255);
        SDL_RenderClear(winRenderer);
        if (winImage) SDL_RenderCopy(winRenderer, winImage, nullptr, nullptr);
        SDL_RenderPresent(winRenderer);
    }

    // Cleanup decryptor screen
    SDL_DestroyTexture(winImage);
    SDL_DestroyRenderer(winRenderer);
    SDL_DestroyWindow(winWindow);

    // SDL_mixer cleanup
    Mix_HaltMusic();
    Mix_FreeMusic(bgm);
    Mix_FreeChunk(correctSfx);
    Mix_FreeChunk(wrongSfx);
    Mix_CloseAudio();
    Mix_Quit();

    // Other SDL cleanup
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}