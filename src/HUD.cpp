#include "HUD.h"
#include "Config.h"
#include "GameState.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <GL/glut.h>

void iniciarHUD()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, LARGURA_JANELA, 0, ALTURA_JANELA);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST); // HUD sempre por cima de tudo que e 3D
}

void encerrarHUD()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Retangulo com cantos arredondados, preenchido com a cor atual
// (chame glColor3f/4f antes). (x1,y1) = canto inferior-esquerdo,
// (x2,y2) = canto superior-direito.
void desenharRetanguloArredondado(float x1, float y1, float x2, float y2, float raio)
{
    if(raio > (x2 - x1) / 2.0f) raio = (x2 - x1) / 2.0f;
    if(raio > (y2 - y1) / 2.0f) raio = (y2 - y1) / 2.0f;

    const int segmentos = 8; // suficiente pra cantos pequenos como os do HUD

    struct Canto { float cx, cy, anguloInicial; };
    Canto cantos[4] = {
        { x2 - raio, y1 + raio, 270.0f }, // inferior-direito
        { x2 - raio, y2 - raio,   0.0f }, // superior-direito
        { x1 + raio, y2 - raio,  90.0f }, // superior-esquerdo
        { x1 + raio, y1 + raio, 180.0f }, // inferior-esquerdo
    };

    glBegin(GL_POLYGON);
    for(int c = 0; c < 4; c++)
    {
        for(int i = 0; i <= segmentos; i++)
        {
            float angulo = (cantos[c].anguloInicial + (90.0f * i / segmentos)) * 3.14159265f / 180.0f;
            glVertex2f(cantos[c].cx + cosf(angulo) * raio,
                       cantos[c].cy + sinf(angulo) * raio);
        }
    }
    glEnd();
}

// Hexagono regular (pontas viradas pra esquerda/direita), preenchido com a
// cor atual. (cx,cy) = centro, raio = distancia do centro a cada vertice.
void desenharHexagono(float cx, float cy, float raio)
{
    glBegin(GL_POLYGON);
    for(int i = 0; i < 6; i++)
    {
        float angulo = (60.0f * i) * 3.14159265f / 180.0f;
        glVertex2f(cx + cosf(angulo) * raio, cy + sinf(angulo) * raio);
    }
    glEnd();
}

// Largura (em pixels) que uma string ocuparia se desenhada com a fonte
// bitmap informada -- usado pra centralizar texto manualmente, ja que o
// GLUT nao centraliza texto sozinho.
float larguraTextoBitmap(void* fonte, const std::string& texto)
{
    float total = 0.0f;
    for(char c : texto)
        total += (float)glutBitmapWidth(fonte, c);
    return total;
}

// Desenha texto na posicao (x,y) = canto inferior-esquerdo do texto.
// negrito=true desenha o texto duas vezes (a segunda 1px deslocada pra
// direita) pra simular um traco mais grosso, ja que as fontes bitmap do
// GLUT so vem numa unica espessura.
void desenharTextoHUD(float x, float y, const std::string& texto, void* fonte,
                       float r, float g, float b, bool negrito)
{
    glColor3f(r, g, b);

    glRasterPos2f(x, y);
    for(char c : texto)
        glutBitmapCharacter(fonte, c);

    if(negrito)
    {
        glRasterPos2f(x + 1.0f, y);
        for(char c : texto)
            glutBitmapCharacter(fonte, c);
    }
}

// Igual a desenharTextoHUD, mas centralizando horizontalmente em centroX.
void desenharTextoCentralizadoHUD(float centroX, float y, const std::string& texto, void* fonte,
                                   float r, float g, float b, bool negrito)
{
    float largura = larguraTextoBitmap(fonte, texto);
    desenharTextoHUD(centroX - largura / 2.0f, y, texto, fonte, r, g, b, negrito);
}

//=====================================
// PLACAR (HUD)
//=====================================
// Desenha o placar fixo no topo central da tela: linha de cima com o
// marcador do jogador (azul), um "X" central (hexagono cinza) e o marcador
// da CPU (vermelho); linha de baixo com o cronometro da partida (mm:ss).
// Todas as dimensoes/posicoes vem das variaveis PLACAR_* em GameState.
void desenharPlacar()
{
    iniciarHUD();

    float topo     = ALTURA_JANELA - PLACAR_POS_Y;
    float base     = topo - PLACAR_ALTURA;
    float esquerda = PLACAR_POS_X - PLACAR_LARGURA / 2.0f;
    float direita  = PLACAR_POS_X + PLACAR_LARGURA / 2.0f;

    float alturaMarcador = PLACAR_ALTURA * PLACAR_PROPORCAO_LINHA_MARCADOR;
    float topoMarcador   = topo;
    float baseMarcador   = topo - alturaMarcador;

    float topoCronometro = baseMarcador;
    float baseCronometro = base;

    const float raioCantos = 8.0f;

    // ---- Linha de cima: placar do jogador, "X" central, placar da CPU ----
    float larguraCaixaPlacar = PLACAR_LARGURA * 0.40f;

    glColor3f(0.13f, 0.69f, 0.92f); // azul (jogador)
    desenharRetanguloArredondado(esquerda, baseMarcador, esquerda + larguraCaixaPlacar, topoMarcador, raioCantos);

    glColor3f(0.93f, 0.30f, 0.30f); // vermelho (CPU)
    desenharRetanguloArredondado(direita - larguraCaixaPlacar, baseMarcador, direita, topoMarcador, raioCantos);

    glColor3f(0.45f, 0.45f, 0.45f); // hexagono cinza central
    desenharHexagono(PLACAR_POS_X, (topoMarcador + baseMarcador) / 2.0f, alturaMarcador * 0.68f);

    float centroYMarcador = (topoMarcador + baseMarcador) / 2.0f - 6.0f;

    desenharTextoCentralizadoHUD(esquerda + larguraCaixaPlacar / 2.0f, centroYMarcador,
        std::to_string(placarJogador), GLUT_BITMAP_HELVETICA_18, 1.0f, 1.0f, 1.0f, true);

    desenharTextoCentralizadoHUD(direita - larguraCaixaPlacar / 2.0f, centroYMarcador,
        std::to_string(placarCPU), GLUT_BITMAP_HELVETICA_18, 1.0f, 1.0f, 1.0f, true);

    desenharTextoCentralizadoHUD(PLACAR_POS_X, centroYMarcador,
        "X", GLUT_BITMAP_HELVETICA_18, 1.0f, 0.78f, 0.04f, true);

    // ---- Linha de baixo: cronometro (mm:ss) ----
    glColor3f(0.85f, 0.85f, 0.85f);
    desenharRetanguloArredondado(esquerda, baseCronometro, direita, topoCronometro, raioCantos);

    int segundosTotais = (int)tempoPartidaSegundos;
    int minutos = segundosTotais / 60;
    int segundos = segundosTotais % 60;

    char bufferTempo[8];
    snprintf(bufferTempo, sizeof(bufferTempo), "%02d:%02d", minutos, segundos);

    float centroYCronometro = (topoCronometro + baseCronometro) / 2.0f - 8.0f;

    desenharTextoCentralizadoHUD(PLACAR_POS_X, centroYCronometro,
        std::string(bufferTempo), GLUT_BITMAP_TIMES_ROMAN_24, 0.12f, 0.12f, 0.12f, true);

    encerrarHUD();
}

//=====================================
// MENU DE PAUSA
//=====================================

void calcularRetanguloBotaoMenu(int indice, float& x1, float& y1, float& x2, float& y2)
{
    float topoBotoes = MENU_POS_Y + MENU_ALTURA / 2.0f - 90.0f; // espaco reservado pro titulo "MENU"

    x1 = MENU_POS_X - MENU_BOTAO_LARGURA / 2.0f;
    x2 = MENU_POS_X + MENU_BOTAO_LARGURA / 2.0f;

    y2 = topoBotoes - indice * (MENU_BOTAO_ALTURA + MENU_BOTAO_ESPACAMENTO);
    y1 = y2 - MENU_BOTAO_ALTURA;
}

// Desenha o menu de pausa: fundo escurecido cobrindo a tela toda e o
// painel com o titulo e os 3 botoes (Continuar / Reiniciar / Sair).
void desenharMenuPausa()
{
    iniciarHUD();

    // Fundo escurecido (semi-transparente) cobrindo toda a tela
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(LARGURA_JANELA, 0.0f);
        glVertex2f(LARGURA_JANELA, ALTURA_JANELA);
        glVertex2f(0.0f, ALTURA_JANELA);
    glEnd();
    glDisable(GL_BLEND);

    // Painel do menu (nao ocupa a tela inteira -- so a area MENU_LARGURA x MENU_ALTURA)
    float esquerda = MENU_POS_X - MENU_LARGURA / 2.0f;
    float direita  = MENU_POS_X + MENU_LARGURA / 2.0f;
    float topo     = MENU_POS_Y + MENU_ALTURA / 2.0f;
    float base     = MENU_POS_Y - MENU_ALTURA / 2.0f;

    glColor3f(0.85f, 0.85f, 0.85f);
    desenharRetanguloArredondado(esquerda, base, direita, topo, 14.0f);

    desenharTextoCentralizadoHUD(MENU_POS_X, topo - 50.0f, "MENU",
        GLUT_BITMAP_TIMES_ROMAN_24, 0.45f, 0.45f, 0.45f, true);

    const char* rotulos[3] = { "CONTINUAR", "REINICIAR", "SAIR" };

    for(int i = 0; i < 3; i++)
    {
        float bx1, by1, bx2, by2;
        calcularRetanguloBotaoMenu(i, bx1, by1, bx2, by2);

        glColor3f(0.45f, 0.45f, 0.45f);
        desenharRetanguloArredondado(bx1, by1, bx2, by2, 6.0f);

        desenharTextoCentralizadoHUD((bx1 + bx2) / 2.0f, (by1 + by2) / 2.0f - 6.0f,
            rotulos[i], GLUT_BITMAP_HELVETICA_18, 1.0f, 0.78f, 0.04f, true);
    }

    encerrarHUD();
}

// Clique do mouse: so faz alguma coisa enquanto o menu de pausa esta
// aberto, verificando se o clique caiu dentro do retangulo de algum dos
// 3 botoes (os mesmos retangulos usados pra desenhar).
void mouseClique(int botao, int estado, int x, int y)
{
    if(!jogoPausado) return;
    if(botao != GLUT_LEFT_BUTTON || estado != GLUT_DOWN) return;

    // O GLUT entrega (x,y) em pixels da janela REAL, que nem sempre tem
    // exatamente LARGURA_JANELA x ALTURA_JANELA. Como o nosso HUD sempre
    // desenha num espaco fixo de LARGURA_JANELA x ALTURA_JANELA (esticado
    // pra ocupar a janela real), precisamos converter (x,y) pra esse mesmo
    // espaco fixo e inverter o eixo Y (GLUT conta Y crescendo pra baixo;
    // o HUD conta Y crescendo pra cima, igual ao gluOrtho2D de iniciarHUD()).
    int larguraReal = glutGet(GLUT_WINDOW_WIDTH);
    int alturaReal  = glutGet(GLUT_WINDOW_HEIGHT);
    if(larguraReal <= 0) larguraReal = (int)LARGURA_JANELA;
    if(alturaReal  <= 0) alturaReal  = (int)ALTURA_JANELA;

    float xHUD = (float)x * (LARGURA_JANELA / (float)larguraReal);
    float yHUD = ((float)alturaReal - (float)y) * (ALTURA_JANELA / (float)alturaReal);

    for(int i = 0; i < 3; i++)
    {
        float x1, y1, x2, y2;
        calcularRetanguloBotaoMenu(i, x1, y1, x2, y2);

        if(xHUD >= x1 && xHUD <= x2 && yHUD >= y1 && yHUD <= y2)
        {
            if(i == 0)      // CONTINUAR
                jogoPausado = false;
            else if(i == 1) // REINICIAR
            {
                reiniciarJogo();
                jogoPausado = false;
            }
            else            // SAIR
                exit(0);

            glutPostRedisplay();
            return;
        }
    }
}
