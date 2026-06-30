#ifndef INPUT_H
#define INPUT_H

//=====================================
// TECLADO -- callbacks de entrada do GLUT
//=====================================

void tecladoEspecial(int key, int x, int y);
void tecladoEspecialSolto(int key, int x, int y);

// Teclado normal (letras/numeros) -- tecla Z dispara animacao de chute,
// P abre/fecha o menu de pausa, ESC sai do jogo.
void tecladoNormal(unsigned char key, int x, int y);

#endif // INPUT_H
