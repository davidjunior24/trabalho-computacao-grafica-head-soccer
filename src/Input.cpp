#include "Input.h"
#include "GameState.h"

#include <cstdlib>
#include <GL/glut.h>

void tecladoEspecial(int key, int, int)
{
    switch(key)
    {
        case GLUT_KEY_LEFT:  teclaEsquerda = true; break;
        case GLUT_KEY_RIGHT: teclaDireita  = true; break;
        case GLUT_KEY_UP:    teclaCima     = true; break;
    }
}

void tecladoEspecialSolto(int key, int, int)
{
    switch(key)
    {
        case GLUT_KEY_LEFT:  teclaEsquerda = false; break;
        case GLUT_KEY_RIGHT: teclaDireita  = false; break;
        case GLUT_KEY_UP:    teclaCima     = false; break;
    }
}

// Teclado normal (letras/numeros) -- tecla Z dispara animacao de chute
void tecladoNormal(unsigned char key, int, int)
{
    switch(key)
    {
        case 'z':
        case 'Z':
            // So inicia se nao estiver ja animando, e so com o jogo
            // rodando (senao a animacao ficaria "presa" em 0 enquanto
            // pausado e tocaria do nada quando o jogador retomasse)
            if(!jogoPausado && tempoAnimChuteJogador < 0.0f)
                tempoAnimChuteJogador = 0.0f;
            break;
        case 'p':
        case 'P':
            // Abre/fecha o menu de pausa
            jogoPausado = !jogoPausado;
            break;
        case 27: // ESC
            exit(0);
            break;
    }
}
