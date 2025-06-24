#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <vector>
#include <ctime>

#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 600
#define BLOCK_SIZE 30
#define GRID_WIDTH (SCREEN_WIDTH / BLOCK_SIZE)
#define GRID_HEIGHT (SCREEN_HEIGHT / BLOCK_SIZE)

// Tetromino shapes represented as 4x4 matrices (I, O, T, S, Z, L, J)
const int tetrominoShapes[7][4][4] = {
    {{1,1,1,1},{0,0,0,0},{0,0,0,0},{0,0,0,0}},
    {{1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
    {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}}
};

SDL_Window*   window           = nullptr;
SDL_Renderer* renderer         = nullptr;
TTF_Font*     m_pFont          = nullptr;
Mix_Music*    music            = nullptr;
Mix_Chunk*    moveSound        = nullptr;
Mix_Chunk*    rotateSound      = nullptr;
Mix_Chunk*    lineClearSound   = nullptr;
SDL_Texture*  backgroundTex    = nullptr;

int grid[GRID_HEIGHT][GRID_WIDTH] = {0};
int score = 0;
bool isPaused = false;
int speed = 500;

struct Tetromino {
    int shape[4][4];
    int x, y, color;
};

Tetromino generateTetromino() {
    Tetromino t;
    int idx = rand() % 7;
    t.color = idx+1;
    t.x = GRID_WIDTH/2 - 2; t.y = 0;
    for(int i=0;i<4;i++) for(int j=0;j<4;j++)
        t.shape[i][j] = tetrominoShapes[idx][i][j];
    return t;
}

SDL_Color getColor(int c) {
    static SDL_Color cols[8] = {
        {0,0,0,255},{0,255,255,255},{255,255,0,255},
        {255,0,0,255},{0,255,0,255},{255,165,0,255},
        {0,0,255,255},{128,0,128,255}
    };
    return cols[c<0||c>7?0:c];
}

bool init() {
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)!=0){std::cerr<<SDL_GetError();return false;}
    if(!(IMG_Init(IMG_INIT_PNG)&IMG_INIT_PNG)){std::cerr<<IMG_GetError();return false;}
    window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
    if(TTF_Init()==-1){std::cerr<<TTF_GetError();return false;}
    m_pFont = TTF_OpenFont("arial.ttf",24);
    if(!m_pFont){std::cerr<<TTF_GetError();return false;}
    if(Mix_Init(MIX_INIT_MP3)==0){std::cerr<<Mix_GetError();return false;}
    if(Mix_OpenAudio(22050,MIX_DEFAULT_FORMAT,2,4096)==-1){std::cerr<<Mix_GetError();return false;}

    // Load background image
    SDL_Surface* bgSurf = IMG_Load("tetris_background.png");
    if(!bgSurf){ std::cerr<<"IMG_Load: "<<IMG_GetError(); return false; }
    backgroundTex = SDL_CreateTextureFromSurface(renderer,bgSurf);
    SDL_FreeSurface(bgSurf);
    if(!backgroundTex) {
        std::cerr << "SDL_CreateTextureFromSurface: " << SDL_GetError() << std::endl;
        return false;
    }

    // Load music & sounds
    music = Mix_LoadMUS("tetris_background.mp3");
    moveSound = Mix_LoadWAV("move.mp3");
    rotateSound = Mix_LoadWAV("rotate.mp3");
    lineClearSound = Mix_LoadWAV("line_clear.mp3");
    if(!music||!moveSound||!rotateSound||!lineClearSound){
        std::cerr<<"Mix_Load: "<<Mix_GetError(); return false;
    }
    Mix_PlayMusic(music, -1);
    return true;
}

void drawBlock(int x,int y,int c){
    SDL_Color col = getColor(c);
    SDL_SetRenderDrawColor(renderer,col.r,col.g,col.b,col.a);
    SDL_Rect r={x*BLOCK_SIZE,y*BLOCK_SIZE,BLOCK_SIZE,BLOCK_SIZE};
    SDL_RenderFillRect(renderer,&r);
}

void drawTetromino(const Tetromino& t){
    for(int i=0;i<4;i++)for(int j=0;j<4;j++)
        if(t.shape[i][j]) drawBlock(t.x+j,t.y+i,t.color);
}

bool checkCollision(const Tetromino& t){
    for(int i=0;i<4;i++)for(int j=0;j<4;j++) if(t.shape[i][j]){
        int x=t.x+j, y=t.y+i;
        if(x<0||x>=GRID_WIDTH||y>=GRID_HEIGHT|| grid[y][x]!=0) return true;
    }
    return false;
}

void placeTetromino(const Tetromino& t){
    for(int i=0;i<4;i++)for(int j=0;j<4;j++) if(t.shape[i][j]){
        grid[t.y+i][t.x+j] = t.color;
    }
}

void clearLines(){
    for(int y=GRID_HEIGHT-1;y>=0;y--){
        bool full=true; for(int x=0;x<GRID_WIDTH;x++) if(!grid[y][x]){ full=false; break; }
        if(full){
            Mix_PlayChannel(-1, lineClearSound, 0);
            score += 100;
            for(int i=y;i>0;i--) for(int x=0;x<GRID_WIDTH;x++) grid[i][x]=grid[i-1][x];
            for(int x=0;x<GRID_WIDTH;x++) grid[0][x]=0;
            y++; 
        }
    }
}

void handleInput(Tetromino& cur){
    SDL_Event e;
    while(SDL_PollEvent(&e)){
        if(e.type==SDL_QUIT) exit(0);
        if(e.type==SDL_KEYDOWN){
            Tetromino tmp = cur;
            switch(e.key.keysym.sym){
                case SDLK_LEFT:
                    cur.x--; if(checkCollision(cur)) cur.x++;
                    Mix_PlayChannel(-1, moveSound, 0);
                    break;
                case SDLK_RIGHT:
                    cur.x++; if(checkCollision(cur)) cur.x--;
                    Mix_PlayChannel(-1, moveSound, 0);
                    break;
                case SDLK_DOWN:
                    cur.y++; if(checkCollision(cur)){ cur.y--; placeTetromino(cur); clearLines(); cur=generateTetromino(); }
                    Mix_PlayChannel(-1, moveSound, 0);
                    break;
                case SDLK_UP:
                    Mix_PlayChannel(-1, rotateSound, 0);
                    for(int i=0;i<4;i++) for(int j=0;j<4;j++)
                        tmp.shape[i][j] = cur.shape[3-j][i];
                    if(!checkCollision(tmp)) cur = tmp;
                    break;
                case SDLK_p:
                    isPaused = !isPaused;
                    break;
            }
        }
    }
}

void showGameOver(){
    SDL_Color white={255,255,255,255};
    SDL_Surface* s = TTF_RenderText_Solid(m_pFont,"Game Over",white);
    SDL_Texture* t = SDL_CreateTextureFromSurface(renderer,s);
    SDL_Rect r={(SCREEN_WIDTH-s->w)/2,(SCREEN_HEIGHT-s->h)/2,s->w,s->h};
    SDL_FreeSurface(s);
    SDL_RenderCopy(renderer,t,nullptr,&r);
    SDL_DestroyTexture(t);
    SDL_RenderPresent(renderer);
    SDL_Delay(3000);
}

int main(){
    srand(time(0));
    if(!init()) return -1;

    Tetromino cur = generateTetromino();
    bool running=true;
    Uint32 last = SDL_GetTicks();

    while(running){
        if(!isPaused && SDL_GetTicks()-last >= (Uint32)speed){
            cur.y++;
            if(checkCollision(cur)){
                cur.y--; placeTetromino(cur); clearLines();
                cur = generateTetromino();
                if(checkCollision(cur)) { running=false; }
            }
            last = SDL_GetTicks();
        }

        handleInput(cur);

        // draw background
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, backgroundTex, nullptr, nullptr);

        // draw grid contents
        for(int y=0;y<GRID_HEIGHT;y++)for(int x=0;x<GRID_WIDTH;x++)
            if(grid[y][x]) drawBlock(x,y,grid[y][x]);

        drawTetromino(cur);

        // draw score
        SDL_Color white={255,255,255,255};
        char buf[32]; sprintf(buf,"Score: %d",score);
        SDL_Surface* s = TTF_RenderText_Solid(m_pFont,buf,white);
        SDL_Texture* t = SDL_CreateTextureFromSurface(renderer,s);
        SDL_Rect r={(SCREEN_WIDTH - s->w)/2,5,s->w,s->h};
        SDL_FreeSurface(s);
        SDL_RenderCopy(renderer,t,nullptr,&r);
        SDL_DestroyTexture(t);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    showGameOver();

    // cleanup
    Mix_FreeChunk(moveSound);
    Mix_FreeChunk(rotateSound);
    Mix_FreeChunk(lineClearSound);
    Mix_FreeMusic(music);
    SDL_DestroyTexture(backgroundTex);
    TTF_CloseFont(m_pFont);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}

// g++ -std=c++17 tetris.cpp -o tetris \
  $(pkg-config --cflags --libs sdl2 SDL2_image SDL2_ttf SDL2_mixer) \
  -Wno-deprecated-declarations
  
//./tetris