#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>

// Modular exponentiation
long long mod_exp(long long base, long long exp, long long mod) {
    long long result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) result = (result * base) % mod;
        exp >>= 1;
        base = (base * base) % mod;
    }
    return result;
}

std::string decryptRSA(const std::string& encryptedStr, long long d, long long n) {
    std::stringstream ss(encryptedStr);
    std::string token, result;
    while (ss >> token) {
        long long cipher = std::stoll(token);
        char decryptedChar = static_cast<char>(mod_exp(cipher, d, n));
        result += decryptedChar;
    }
    return result;
}

// Text rendering
void renderText(SDL_Renderer* renderer, TTF_Font* font,
                const std::string& text, SDL_Color color, int x, int y) {
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_FreeSurface(surf);
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

int main() {
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    if (TTF_Init() < 0) { SDL_Quit(); return 1; }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) { TTF_Quit(); SDL_Quit(); return 1; }

    // SDL_mixer init
    if ((Mix_Init(MIX_INIT_MP3) & MIX_INIT_MP3) != MIX_INIT_MP3) {
        std::cerr << "Mix_Init failed: " << Mix_GetError() << "\n";
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Mix_OpenAudio failed: " << Mix_GetError() << "\n";
    }

    SDL_Window* window = SDL_CreateWindow("RSA GUI Decryptor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 900, 600, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load font & background
    TTF_Font* font = TTF_OpenFont("DejaVuSans.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Surface* bgSurf = IMG_Load("rsa_background.png");
    if (!bgSurf) {
        std::cerr << "Failed to load background image: " << IMG_GetError() << std::endl;
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Texture* bgTex = SDL_CreateTextureFromSurface(renderer, bgSurf);
    SDL_FreeSurface(bgSurf);
    if (!bgTex) {
        std::cerr << "Failed to create background texture: " << SDL_GetError() << std::endl;
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load audio assets
    Mix_Music* bgm        = Mix_LoadMUS("rsa_background.mp3");
    if (!bgm) {
        std::cerr << "Failed to load music: " << Mix_GetError() << std::endl;
    }
    Mix_Chunk* sfxFail    = Mix_LoadWAV("buzzer.mp3");
    if (!sfxFail) {
        std::cerr << "Failed to load fail sound: " << Mix_GetError() << std::endl;
    }
    Mix_Chunk* sfxSuccess = Mix_LoadWAV("successed.mp3");
    if (!sfxSuccess) {
        std::cerr << "Failed to load success sound: " << Mix_GetError() << std::endl;
    }
    if (bgm) Mix_PlayMusic(bgm, -1);

    std::string inputN, inputE, inputEnc, result;
    enum Focus { FOCUS_N, FOCUS_E, FOCUS_ENC } currentFocus = FOCUS_N;

    SDL_StartTextInput();
    bool running = true;
    float anim = 0;
    SDL_Event e;

    // Layout
    const int padH = 14, padV = 8;
    int yN = 40, yE = 100, yEncLabel = 160, yEncBox = 190;
    SDL_Rect rectN   = {200, yN,       500, 38};
    SDL_Rect rectE   = {200, yE,       500, 38};
    SDL_Rect rectEnc = {200, yEncBox,  500, 38};
    SDL_Rect btn     = {50, 260, 120, 40};

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x, my = e.button.y;
                // Decrypt button
                if (mx>=btn.x && mx<=btn.x+btn.w && my>=btn.y && my<=btn.y+btn.h) {
                    try {
                        long long n = std::stoll(inputN);
                        long long d = std::stoll(inputE);
                        if (n==2537 && d==13 && inputEnc=="2081 2182 2024") {
                            result = "Curzon is haunted";
                            Mix_PlayChannel(-1, sfxSuccess, 0);
                        } else {
                            result = "Access Denied. Try again.";
                            Mix_PlayChannel(-1, sfxFail, 0);
                        }
                    } catch (...) {
                        result = "Invalid input";
                        Mix_PlayChannel(-1, sfxFail, 0);
                    }
                }
                // Focus boxes
                else if (mx>=rectN.x && mx<=rectN.x+rectN.w && my>=rectN.y && my<=rectN.y+rectN.h)
                    currentFocus = FOCUS_N;
                else if (mx>=rectE.x && mx<=rectE.x+rectE.w && my>=rectE.y && my<=rectE.y+rectE.h)
                    currentFocus = FOCUS_E;
                else if (mx>=rectEnc.x && mx<=rectEnc.x+rectEnc.w && my>=rectEnc.y && my<=rectEnc.y+rectEnc.h)
                    currentFocus = FOCUS_ENC;
            }

            else if (e.type == SDL_TEXTINPUT) {
                if (currentFocus==FOCUS_N)   inputN   += e.text.text;
                if (currentFocus==FOCUS_E)   inputE   += e.text.text;
                if (currentFocus==FOCUS_ENC) inputEnc += e.text.text;
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym==SDLK_BACKSPACE) {
                if (currentFocus==FOCUS_N && !inputN.empty())   inputN.pop_back();
                if (currentFocus==FOCUS_E && !inputE.empty())   inputE.pop_back();
                if (currentFocus==FOCUS_ENC && !inputEnc.empty()) inputEnc.pop_back();
            }
        }

        // Animate background
        anim += 0.03f;
        int offsetY = int(std::sin(anim)*5);
        SDL_SetRenderDrawColor(renderer, 30,30,30,255);
        SDL_RenderClear(renderer);
        SDL_Rect bgDst = {0, offsetY, 900, 600};
        SDL_RenderCopy(renderer, bgTex, nullptr, &bgDst);

        // Labels
        renderText(renderer, font, "Enter n:",           {255,255,255,255}, 50, yN);
        renderText(renderer, font, "Enter d:",           {255,255,255,255}, 50, yE);
        renderText(renderer, font, "Encrypted Text:",    {255,255,255,255}, 50, yEncLabel);
        renderText(renderer, font, "Result:",            {255,255,255,255}, 50, 320);

        // Boxes
        SDL_SetRenderDrawColor(renderer, 180,180,180,200);
        SDL_RenderDrawRect(renderer, &rectN);
        SDL_RenderDrawRect(renderer, &rectE);
        SDL_RenderDrawRect(renderer, &rectEnc);
        SDL_RenderDrawRect(renderer, &rectN); // twice for bold
        SDL_RenderDrawRect(renderer, &rectE);
        SDL_RenderDrawRect(renderer, &rectEnc);

        // Highlight
        SDL_SetRenderDrawColor(renderer, 50,200,50,150);
        SDL_Rect hl = (currentFocus==FOCUS_N?rectN:
                       currentFocus==FOCUS_E?rectE:rectEnc);
        SDL_RenderDrawRect(renderer, &hl);
        hl.x++; hl.y++;
        SDL_RenderDrawRect(renderer, &hl);

        // Text in boxes
        renderText(renderer, font, inputN,   {255,255,255,255}, rectN.x+padH,   rectN.y+padV);
        renderText(renderer, font, inputE,   {255,255,255,255}, rectE.x+padH,   rectE.y+padV);
        renderText(renderer, font, inputEnc, {255,255,255,255}, rectEnc.x+padH, rectEnc.y+padV);

        // Decrypt button
        SDL_SetRenderDrawColor(renderer, 50,200,50,255);
        SDL_RenderFillRect(renderer, &btn);
        renderText(renderer, font, "Decrypt", {30,30,30,255},
                   btn.x+20, btn.y+7);

        // Result
        SDL_Color col = (result.rfind("Access Denied",0)==0 || result=="Invalid input")
                        ? SDL_Color{255,60,60,255}
                        : SDL_Color{50,255,100,255};
        renderText(renderer, font, result, col, 50, 360);

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_StopTextInput();
    Mix_HaltMusic();
    Mix_FreeMusic(bgm);
    Mix_FreeChunk(sfxFail);
    Mix_FreeChunk(sfxSuccess);
    Mix_CloseAudio();
    Mix_Quit();

    SDL_DestroyTexture(bgTex);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
