#ifndef RENDERER_H
#define RENDERER_H

//=====================================
// RENDERER -- desenho da cena 3D (campo, gols, cenario, personagens, bola)
//=====================================

void desenharCampo();
void desenharArquibancada();
void desenharRefletores();
void desenharGol(float x, float direcao);
void desenharParedeFundo(float x);
void desenharNuvens();

// Callback de desenho do GLUT: monta a cena completa de um frame.
void display();

// Callback de redimensionamento de janela do GLUT.
void reshape(int w, int h);

#endif // RENDERER_H
