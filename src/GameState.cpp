#include "GameState.h"
#include "Config.h"

//=====================================
// GAME STATE -- definicoes
//=====================================

float jogadorX = JOGADOR_X_INICIAL;
float jogadorY = ALTURA_INICIAL;
float velJogadorY = 0.0f;

float cpuX = CPU_X_INICIAL;
float cpuY = ALTURA_INICIAL;
float velCpuY = 0.0f;

float bolaX = 0.0f;
float bolaY = 5.0f;
float bolaXAnterior = 0.0f;
float velBolaX = 0.0f;
float velBolaY = 0.0f;
bool  bolaEmSaque = true;

int   placarJogador = 0;
int   placarCPU = 0;
float tempoPartidaSegundos = 0.0f;
bool  jogoPausado = false;

float PLACAR_LARGURA = 250.0f;
float PLACAR_ALTURA  = 90.0f;
float PLACAR_POS_X   = LARGURA_JANELA / 2.0f;
float PLACAR_POS_Y   = 14.0f;

float MENU_LARGURA = 320.0f;
float MENU_ALTURA  = 360.0f;
float MENU_POS_X   = LARGURA_JANELA / 2.0f;
float MENU_POS_Y   = ALTURA_JANELA  / 2.0f;
float MENU_BOTAO_LARGURA     = 240.0f;
float MENU_BOTAO_ALTURA      = 46.0f;
float MENU_BOTAO_ESPACAMENTO = 18.0f;

bool teclaEsquerda = false;
bool teclaDireita  = false;
bool teclaCima     = false;

bool jogadorTocandoBola = false;
bool cpuTocandoBola = false;

Mix_Chunk* chunkSomGol   = nullptr;
Mix_Chunk* chunkSomApito = nullptr;
Mix_Music* musicaFundo   = nullptr;

Modelo3D modeloCampo;
Modelo3D modeloTrave;
Modelo3D modeloArquibancada;
Modelo3D modeloRefletor;
Modelo3D modeloBola;
Modelo3D modeloNuvem;

ModeloAnimado modeloJogador;
ModeloAnimado modeloJogadorIA;

float tempoAnimChuteJogador = -1.0f;
float tempoAnimChuteCPU     = -1.0f;

int tempoAnterior = 0;

//=====================================
// RESET
//=====================================

void resetarBola()
{
    bolaX = 0.0f;
    bolaY = 5.0f;
    bolaXAnterior = bolaX; // evita "cruzamento" falso logo apos o reset

    velBolaX = 0.0f;
    velBolaY = 0.0f;

    // Garante que ela caia reto antes de sair andando
    bolaEmSaque = true;
}

void resetarPosicoes()
{
    jogadorX = JOGADOR_X_INICIAL;
    jogadorY = ALTURA_INICIAL;
    velJogadorY = 0.0f;

    cpuX = CPU_X_INICIAL;
    cpuY = ALTURA_INICIAL;
    velCpuY = 0.0f;
}

// Reinicia a partida do zero: zera o placar e o cronometro, e devolve a
// bola e os jogadores as posicoes iniciais. Usado pelo botao "Reiniciar"
// do menu de pausa.
void reiniciarJogo()
{
    placarJogador = 0;
    placarCPU = 0;
    tempoPartidaSegundos = 0.0f;

    resetarBola();
    resetarPosicoes();

    tempoAnimChuteJogador = -1.0f;
    tempoAnimChuteCPU = -1.0f;
}
