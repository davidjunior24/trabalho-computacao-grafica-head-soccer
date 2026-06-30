#include "Audio.h"
#include "Config.h"
#include "GameState.h"

#include <cstdio>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

// Inicializa o SDL2_mixer e carrega os dois WAVs. Chamado uma unica vez
// no init(). SDL_MAIN_HANDLED ja foi definido antes dos includes, entao
// o SDL nao interfere com o main() do GLUT.
void carregarSomGol()
{
    SDL_SetMainReady(); // necessario quando SDL_MAIN_HANDLED esta definido
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        fprintf(stderr, "[Audio] SDL_Init falhou: %s\n", SDL_GetError());
        return;
    }
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        fprintf(stderr, "[Audio] Mix_OpenAudio falhou: %s\n", Mix_GetError());
        return;
    }
    Mix_AllocateChannels(8);

    chunkSomGol = Mix_LoadWAV(CAMINHO_SOM_GOL);
    if(!chunkSomGol)
        fprintf(stderr, "[Audio] Nao foi possivel carregar %s: %s\n",
                CAMINHO_SOM_GOL, Mix_GetError());

    // Musica de fundo: Mix_Music com loop infinito (-1).
    // Toca em canal dedicado separado dos efeitos, entao nao conflita
    // com gol nem apito.
    musicaFundo = Mix_LoadMUS(CAMINHO_MUSICA_FUNDO);
    if(musicaFundo)
    {
        Mix_PlayMusic(musicaFundo, -1); // -1 = loop infinito
        Mix_VolumeMusic(20); // 0 = mudo, 128 = maximo
    }
    else
        fprintf(stderr, "[Audio] Nao foi possivel carregar %s: %s\n",
                CAMINHO_MUSICA_FUNDO, Mix_GetError());
}

void carregarSomApito()
{
    chunkSomApito = Mix_LoadWAV(CAMINHO_SOM_APITO);
    if(!chunkSomApito)
        fprintf(stderr, "[Audio] Nao foi possivel carregar %s: %s\n",
                CAMINHO_SOM_APITO, Mix_GetError());
}

void tocarSomGol()
{
    if(chunkSomGol)
        Mix_PlayChannel(-1, chunkSomGol, 0);
}

void tocarSomApito()
{
    if(chunkSomApito)
        Mix_PlayChannel(-1, chunkSomApito, 0);
}
