#ifndef HUD_H
#define HUD_H

#include <string>

//=====================================
// HUD -- placar, menu de pausa, e ferramentas de texto/formas 2D
//=====================================

// Entra/sai do modo HUD (projecao 2D, sem luz, sem teste de profundidade).
void iniciarHUD();
void encerrarHUD();

// Retangulo com cantos arredondados, preenchido com a cor atual.
void desenharRetanguloArredondado(float x1, float y1, float x2, float y2, float raio);

// Hexagono regular preenchido com a cor atual.
void desenharHexagono(float cx, float cy, float raio);

// Largura (em pixels) que uma string ocuparia com a fonte bitmap informada.
float larguraTextoBitmap(void* fonte, const std::string& texto);

// Desenha texto na posicao (x,y) = canto inferior-esquerdo do texto.
void desenharTextoHUD(float x, float y, const std::string& texto, void* fonte,
                       float r, float g, float b, bool negrito = false);

// Igual a desenharTextoHUD, mas centralizando horizontalmente em centroX.
void desenharTextoCentralizadoHUD(float centroX, float y, const std::string& texto, void* fonte,
                                   float r, float g, float b, bool negrito = false);

// Desenha o placar fixo no topo central da tela.
void desenharPlacar();

// Calcula o retangulo do botao "indice" (0=Continuar, 1=Reiniciar, 2=Sair)
// do menu de pausa. Usado tanto pra desenhar quanto pra detectar cliques.
void calcularRetanguloBotaoMenu(int indice, float& x1, float& y1, float& x2, float& y2);

// Desenha o menu de pausa (fundo escurecido + painel com botoes).
void desenharMenuPausa();

// Callback de clique do mouse (GLUT) -- trata cliques nos botoes do
// menu de pausa.
void mouseClique(int botao, int estado, int x, int y);

#endif // HUD_H
