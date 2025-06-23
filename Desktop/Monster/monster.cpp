// main.cpp
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <cmath>

// screen
const int SCREEN_W = 800;
const int SCREEN_H = 600;

// simple 2D vector
struct Vec2 { float x, y; };

// Entities
struct Entity { Vec2 pos; int health; };

// Bullet
struct Bullet {
    Vec2 pos, vel;
    bool active;
};

Entity player  = {{121,0}, 100};
Entity monster = {{569,0}, 100};

std::vector<Bullet> playerBullets, monsterBullets;
float monsterTimer = 0, monsterInterval = 2.0f;
float monsterMoveTimer = 0, monsterMoveInterval = 1.0f;

// helper to draw health bar
void DrawBar(SDL_Renderer* R, Vec2 p, int health, SDL_Color col) {
    SDL_Rect bg = { int(p.x), int(p.y), 100, 10 };
    SDL_SetRenderDrawColor(R, 100,100,100,255);
    SDL_RenderFillRect(R, &bg);
    SDL_Rect fg = { int(p.x), int(p.y), health, 10 };
    SDL_SetRenderDrawColor(R, col.r, col.g, col.b, col.a);
    SDL_RenderFillRect(R, &fg);
}

// clamp helper
float clamp(float v, float lo, float hi) {
    if (v<lo) return lo;
    if (v>hi) return hi;
    return v;
}

// bullet logic
void Shoot(std::vector<Bullet>& B, Vec2 pos, Vec2 vel) {
    B.push_back({pos, vel, true});
}
void UpdateBullets(std::vector<Bullet>& B, float dt) {
    for(auto &b: B) if(b.active) {
        b.pos.x += b.vel.x * dt;
        b.pos.y += b.vel.y * dt;
        if (b.pos.x<0||b.pos.x>SCREEN_W||b.pos.y<0||b.pos.y>SCREEN_H)
            b.active = false;
    }
}

// simple circle-rect collision
bool CircleRect(Vec2 c, float r, SDL_Rect R) {
    float nearestX = clamp(c.x, R.x, R.x+R.w);
    float nearestY = clamp(c.y, R.y, R.y+R.h);
    float dx = c.x - nearestX;
    float dy = c.y - nearestY;
    return (dx*dx+dy*dy) < r*r;
}

int main(){
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    SDL_Window* win = SDL_CreateWindow("Mechanoid SDL2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);

    // load textures
    SDL_Texture* texBG    = IMG_LoadTexture(ren,"monster_background.png");
    SDL_Texture* texHero  = IMG_LoadTexture(ren,"hero.png");
    SDL_Texture* texEnem  = IMG_LoadTexture(ren,"enemy.png");
    SDL_Texture* texPB    = IMG_LoadTexture(ren,"bullet_player.png");
    SDL_Texture* texEB    = IMG_LoadTexture(ren,"bullet_enemy.png");

    // Check texture loading
    if (!texBG || !texHero || !texEnem || !texPB || !texEB) {
        SDL_Log("Failed to load one or more textures: %s", SDL_GetError());
        return 1;
    }

    // load sounds
    Mix_Music* bgm       = Mix_LoadMUS("starwars.wav");
    Mix_Chunk* sfxShootP = Mix_LoadWAV("shoot_player.mp3");
    Mix_Chunk* sfxShootE = Mix_LoadWAV("shoot_enemy.mp3");

    // Check sound loading
    if (!bgm || !sfxShootP || !sfxShootE) {
        SDL_Log("Failed to load one or more sounds: %s", Mix_GetError());
        return 1;
    }

    Mix_PlayMusic(bgm, -1);

    // scale factors to match your Raylib
    float heroScale =  (100.0f / 64.0f) * 2.5f;  // assume hero.png is 64px tall
    float enemScale =  (100.0f / 64.0f) * 2.5f;

    // initial positions
    player.pos.y  = 400 + 30*sinf(player.pos.x*0.02f);
    monster.pos.y = SCREEN_H - 100 - 64*enemScale;

    bool running=true;
    Uint32 last=SDL_GetTicks();
    while(running){
        Uint32 now = SDL_GetTicks();
        float dt = (now-last)/1000.0f;
        last=now;

        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT) running=false;
            if(e.type==SDL_KEYDOWN){
                if(e.key.keysym.sym==SDLK_SPACE){
                    Mix_PlayChannel(-1, sfxShootP,0);
                    Shoot(playerBullets, player.pos, {0,-300});
                }
            }
        }
        // movement
        const Uint8* ks = SDL_GetKeyboardState(NULL);
        if(ks[SDL_SCANCODE_A]) player.pos.x -= 200*dt;
        if(ks[SDL_SCANCODE_D]) player.pos.x += 200*dt;
        player.pos.x = clamp(player.pos.x, 0, SCREEN_W - 64*heroScale);
        player.pos.y = 400 + 30*sinf(player.pos.x*0.02f);

        // monster AI
        monsterTimer += dt;
        if(monsterTimer>=monsterInterval){
            Mix_PlayChannel(-1, sfxShootE,0);
            float angle = (rand()%360)*3.14159f/180.0f;
            Shoot(monsterBullets, monster.pos, {cosf(angle)*300, sinf(angle)*300});
            monsterTimer=0;
        }
        monsterMoveTimer += dt;
        if(monsterMoveTimer>=monsterMoveInterval){
            monster.pos.x = rand()%(SCREEN_W - int(64*enemScale));
            monster.pos.y = rand()%(SCREEN_H - int(64*enemScale));
            monsterMoveTimer=0;
        }

        UpdateBullets(playerBullets, dt);
        UpdateBullets(monsterBullets, dt);

        // collisions
        SDL_Rect mR = {int(monster.pos.x),int(monster.pos.y),
                       int(64*enemScale),int(64*enemScale)};
        for(auto &b: playerBullets) if(b.active && CircleRect(b.pos,5,mR)){
            monster.health -= 10; b.active=false;
        }
        SDL_Rect pR = {int(player.pos.x),int(player.pos.y),
                       int(64*heroScale),int(64*heroScale)};
        for(auto &b: monsterBullets) if(b.active && CircleRect(b.pos,5,pR)){
            player.health -= 10;
            if (player.health < 0) player.health = 0;
            b.active=false;
        }

        // render
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, texBG, nullptr, nullptr);

        // hero
        SDL_Rect dstH = {int(player.pos.x), int(player.pos.y),
                         int(64*heroScale), int(64*heroScale)};
        SDL_RenderCopyEx(ren, texHero, nullptr, &dstH,0,nullptr,SDL_FLIP_NONE);
        DrawBar(ren,{player.pos.x-30,player.pos.y-20},player.health,{0,255,0,255});

        // monster
        SDL_Rect dstM = {int(monster.pos.x), int(monster.pos.y),
                         int(64*enemScale), int(64*enemScale)};
        SDL_RenderCopyEx(ren, texEnem, nullptr, &dstM,0,nullptr,SDL_FLIP_NONE);
        DrawBar(ren,{monster.pos.x-30,monster.pos.y-20},monster.health,{255,165,0,255});

        // bullets
        for(auto &b: playerBullets) if(b.active){
            SDL_Rect d={int(b.pos.x),int(b.pos.y),16,16};
            SDL_RenderCopyEx(ren,texPB,nullptr,&d,0,nullptr,SDL_FLIP_NONE);
        }
        for(auto &b: monsterBullets) if(b.active){
            SDL_Rect d={int(b.pos.x),int(b.pos.y),16,16};
            SDL_RenderCopyEx(ren,texEB,nullptr,&d,0,nullptr,SDL_FLIP_NONE);
        }

        SDL_RenderPresent(ren);
    }

    // cleanup
    SDL_DestroyTexture(texBG);
    SDL_DestroyTexture(texHero);
    SDL_DestroyTexture(texEnem);
    SDL_DestroyTexture(texPB);
    SDL_DestroyTexture(texEB);
    Mix_FreeMusic(bgm);
    Mix_FreeChunk(sfxShootP);
    Mix_FreeChunk(sfxShootE);
    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
