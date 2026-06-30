#ifndef GAME_STATE_H
#define GAME_STATE_H

// SDL_MAIN_HANDLED: impede que o SDL2 redefina main() como SDL_main() no
// Windows -- sem isso o linker procura WinMain e nao acha o nosso main().
// Precisa ser definido antes do primeiro include de qualquer header SDL,
// em qualquer arquivo do projeto que inclua (direta ou indiretamente)
// SDL.h -- como este header e incluido por varios modulos, definimos aqui.
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL_mixer.h>
#include "Model3D.h"
#include "ModeloAnimado.h"

//=====================================
// GAME STATE -- variaveis mutaveis do jogo
//=====================================
// Declaracoes extern de todo o estado que muda durante a partida
// (posicoes, velocidades, placar, flags). As definicoes ficam em
// GameState.cpp.

// ---- Jogador ----
extern float jogadorX;
extern float jogadorY;
extern float velJogadorY;

// ---- CPU ----
extern float cpuX;
extern float cpuY;
extern float velCpuY;

// ---- Bola ----
extern float bolaX;
extern float bolaY;
extern float bolaXAnterior;
extern float velBolaX;
extern float velBolaY;
extern bool  bolaEmSaque;

// ---- Placar e cronometro ----
extern int   placarJogador;
extern int   placarCPU;
extern float tempoPartidaSegundos;
extern bool  jogoPausado;

// ---- HUD: placar (posicao/tamanho, podem ser ajustados em runtime) ----
extern float PLACAR_LARGURA;
extern float PLACAR_ALTURA;
extern float PLACAR_POS_X;
extern float PLACAR_POS_Y;

// ---- HUD: menu de pausa (posicao/tamanho) ----
extern float MENU_LARGURA;
extern float MENU_ALTURA;
extern float MENU_POS_X;
extern float MENU_POS_Y;
extern float MENU_BOTAO_LARGURA;
extern float MENU_BOTAO_ALTURA;
extern float MENU_BOTAO_ESPACAMENTO;

// ---- Estado das teclas ----
extern bool teclaEsquerda;
extern bool teclaDireita;
extern bool teclaCima;

// ---- Flags de toque bola x personagem ----
extern bool jogadorTocandoBola;
extern bool cpuTocandoBola;

// ---- Audio (SDL2_mixer) ----
extern Mix_Chunk* chunkSomGol;
extern Mix_Chunk* chunkSomApito;
extern Mix_Music* musicaFundo;

// ---- Modelos 3D estaticos (campo, trave, cenario) ----
extern Modelo3D modeloCampo;
extern Modelo3D modeloTrave;
extern Modelo3D modeloArquibancada;
extern Modelo3D modeloRefletor;
extern Modelo3D modeloBola;
extern Modelo3D modeloNuvem;

// ---- Modelos animados (jogador e CPU) ----
extern ModeloAnimado modeloJogador;
extern ModeloAnimado modeloJogadorIA;

// ---- Animacao de chute ----
extern float tempoAnimChuteJogador; // -1 = nao animando
extern float tempoAnimChuteCPU;

// ---- Controle de tempo entre frames ----
extern int tempoAnterior;

// ---- Funcoes de reset/reinicio da partida ----
void resetarBola();
void resetarPosicoes();
void reiniciarJogo();

#endif // GAME_STATE_H
