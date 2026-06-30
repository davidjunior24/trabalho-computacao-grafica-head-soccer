#include "Renderer.h"
#include "Config.h"
#include "GameState.h"
#include "HUD.h"

#include <GL/glut.h>

//=====================================
// CAMPO
//=====================================
// O campo desenhado na mao (glBegin(GL_QUADS)...) foi totalmente removido.
// Agora e so o modelo 3D do Blender (modeloCampo), carregado em init().
void desenharCampo()
{
    glPushMatrix();
    glTranslatef(CAMPO_OFFSET_X, CAMPO_OFFSET_Y, CAMPO_OFFSET_Z);
    modeloCampo.desenhar();
    glPopMatrix();
}

//=====================================
// ARQUIBANCADA
//=====================================
// So cenario (sem fisica/colisao): fica atras do campo, no lado oposto a
// camera.
void desenharArquibancada()
{
    glPushMatrix();
    glTranslatef(ARQUIBANCADA_OFFSET_X, ARQUIBANCADA_OFFSET_Y, ARQUIBANCADA_OFFSET_Z);

    if(ARQUIBANCADA_ESCALA != 1.0f)
        glScalef(ARQUIBANCADA_ESCALA, ARQUIBANCADA_ESCALA, ARQUIBANCADA_ESCALA);

    modeloArquibancada.desenhar();
    glPopMatrix();
}

//=====================================
// REFLETORES
//=====================================
// Dois refletores nos cantos da arquibancada (esquerdo e direito).
// direcaoX: +1 para refletor direito, -1 para esquerdo.
static void desenharRefletor(float direcaoX, float direcaoZ)
{
    glPushMatrix();

    // Posicao: canto da arquibancada
    glTranslatef(direcaoX * REFLETOR_X, REFLETOR_Y, direcaoZ * REFLETOR_Z);

    // Rotacao: cada canto gira para apontar para o centro do campo
    float rotY;

    if (direcaoX < 0.0f && direcaoZ > 0.0f)          // Frente esquerda
        rotY = -45.0f;
    else if (direcaoX > 0.0f && direcaoZ > 0.0f)     // Frente direita
        rotY = 45.0f;
    else if (direcaoX < 0.0f && direcaoZ < 0.0f)     // Fundo esquerda
        rotY = 45.0f;
    else                                             // Fundo direita
        rotY = -45.0f;

    glRotatef(rotY, 0.0f, 1.0f, 0.0f);
    if(REFLETOR_ESCALA != 1.0f)
        glScalef(REFLETOR_ESCALA, REFLETOR_ESCALA, REFLETOR_ESCALA);

    modeloRefletor.desenhar();

    glPopMatrix();
}

void desenharRefletores()
{
    // Canto esquerdo-frente  (-X, +Z)
    desenharRefletor(-1.0f,  1.0f);
    // Canto direito-frente   (+X, +Z)
    desenharRefletor( 1.0f,  1.0f);
    // Canto esquerdo-fundo   (-X, -Z)
    desenharRefletor(-1.0f, -1.0f);
    // Canto direito-fundo    (+X, -Z)
    desenharRefletor( 1.0f, -1.0f);
}

//=====================================
// GOLS
//=====================================
// Geometria original da trave (cubo wireframe + paineis da "rede" feitos a
// mao), mantida no codigo como referencia/debug. Invisivel por padrao --
// troque TRAVE_ANTIGA_VISIVEL pra true se quiser compara-la visualmente
// com o modelo novo do Blender.
static void desenharTraveAntiga(float direcao)
{
    if(!TRAVE_ANTIGA_VISIVEL)
        return;

    glColor3f(1,1,1);
    glutWireCube(3);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f,1.0f,1.0f,0.25f);

    float m = GOL_META;

    glBegin(GL_QUADS);
        // fundo (face oposta a abertura)
        glVertex3f(direcao*m,-m,-m);
        glVertex3f(direcao*m, m,-m);
        glVertex3f(direcao*m, m, m);
        glVertex3f(direcao*m,-m, m);

        // topo
        glVertex3f(-m, m,-m);
        glVertex3f( m, m,-m);
        glVertex3f( m, m, m);
        glVertex3f(-m, m, m);

        // lateral esquerda
        glVertex3f(-m,-m,-m);
        glVertex3f( m,-m,-m);
        glVertex3f( m, m,-m);
        glVertex3f(-m, m,-m);

        // lateral direita
        glVertex3f(-m,-m, m);
        glVertex3f( m,-m, m);
        glVertex3f( m, m, m);
        glVertex3f(-m, m, m);
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// direcao: +1 (gol da direita) ou -1 (gol da esquerda).
//
// Existem agora DUAS geometrias de gol, desenhadas no mesmo lugar:
// 1) A geometria antiga (cubo wireframe + "rede"), invisivel por padrao.
// 2) O modelo novo do Blender (modeloTrave), desenhado no MESMO ponto de
//    ancoragem do cubo logico (sem offset adicional).
//
// Nenhuma constante de fisica/colisao (GOL_X, GOL_META, GOL_FRENTE,
// GOL_FUNDO, GOL_TRAVE_Y) foi tocada -- so a renderizacao.
void desenharGol(float x, float direcao)
{
    glPushMatrix();

    // Ancora: centro geometrico do cubo logico do gol.
    glTranslatef(x, GOL_META, 0.0f);

    // 1) Geometria antiga, como referencia (invisivel por padrao)
    desenharTraveAntiga(direcao);

    // 2) Modelo novo do Blender, com o ajuste fino por cima da mesma ancora
    glPushMatrix();

    glTranslatef(TRAVE_OFFSET_X, TRAVE_OFFSET_Y, TRAVE_OFFSET_Z);

    float rotacaoY = TRAVE_ROTACAO_BASE + (direcao < 0.0f ? 180.0f : 0.0f);
    glRotatef(rotacaoY, 0.0f, 1.0f, 0.0f);

    modeloTrave.desenhar();

    glPopMatrix();

    glPopMatrix();
}

// Parede no limite do campo, atras dos gols. A FISICA continua igual
// (ver tratarParedeFundo) -- so a parte visual foi desligada
// (PAREDE_FUNDO_VISIVEL = false), pois o modelo 3D da trave ja fecha
// visualmente o fundo do gol.
void desenharParedeFundo(float x)
{
    if(!PAREDE_FUNDO_VISIVEL)
        return;

    glPushMatrix();

    glColor3f(0.55f,0.55f,0.6f);

    glBegin(GL_QUADS);
        glNormal3f((x>0)?-1.0f:1.0f, 0, 0);
        glVertex3f(x, 0.0f,         -CAMPO_LARGURA);
        glVertex3f(x, PAREDE_ALTURA,-CAMPO_LARGURA);
        glVertex3f(x, PAREDE_ALTURA, CAMPO_LARGURA);
        glVertex3f(x, 0.0f,          CAMPO_LARGURA);
    glEnd();

    // Linhas horizontais, só pra dar um acabamento de "painel" na parede
    glDisable(GL_LIGHTING);
    glColor3f(0.35f,0.35f,0.4f);

    glBegin(GL_LINES);
        for(float h = 2.0f; h < PAREDE_ALTURA; h += 2.0f)
        {
            glVertex3f(x + (x>0?-0.02f:0.02f), h, -CAMPO_LARGURA);
            glVertex3f(x + (x>0?-0.02f:0.02f), h,  CAMPO_LARGURA);
        }
    glEnd();

    glEnable(GL_LIGHTING);

    glPopMatrix();
}

//=====================================
// NUVENS
//=====================================
void desenharNuvens()
{
    // Posicoes X das 3 nuvens (esquerda, centro, direita).
    const float posX[3] = { -14.0f, 0.0f, 14.0f };

    for(int i = 0; i < 3; i++)
    {
        glPushMatrix();

        // 1) Vai para a posicao de mundo da nuvem
        glTranslatef(posX[i], NUVEM_Y, NUVEM_Z);

        // 2) Escala uniforme
        glScalef(NUVEM_ESCALA, NUVEM_ESCALA, NUVEM_ESCALA);

        // 3) Centraliza o modelo no pivot (subtrai o centro geometrico)
        glTranslatef(-NUVEM_CENTRO_X, -NUVEM_CENTRO_Y, -NUVEM_CENTRO_Z);

        modeloNuvem.desenhar();

        glPopMatrix();
    }
}

//=====================================
// DISPLAY
//=====================================
void display()
{
    glClear(GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    gluLookAt(
        0,10,20,
        0,0,0,
        0,1,0
    );

    GLfloat pos[] =
    {
        0,
        20,
        10,
        1
    };

    glLightfv(
        GL_LIGHT0,
        GL_POSITION,
        pos
    );

    desenharCampo();
    desenharArquibancada();
    desenharRefletores();

    // Usa GOL_X (= 16)
    desenharGol(-GOL_X, -1.0f);
    desenharGol( GOL_X,  1.0f);

    desenharParedeFundo(-PAREDE_FUNDO_X);
    desenharParedeFundo(PAREDE_FUNDO_X);

    // ---- Jogador (modelo FBX, lado esquerdo, ataca para +X / direita) ----
    // jogadorY e o centro logico do personagem. O modelo ja cuida de
    // colocar sua base em y=0 internamente (via offsetY), entao so
    // precisamos transladar para (jogadorX, jogadorY - raioCabeca, 0).
    glPushMatrix();
    glTranslatef(jogadorX, jogadorY - raioCabeca, 0.0f);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    if (tempoAnimChuteJogador >= 0.0f) {
        float tps = modeloJogador.getTicksPerSecondChute();
        modeloJogador.desenharChute(tempoAnimChuteJogador * tps);
    } else {
        modeloJogador.desenharRepouso();
    }
    glPopMatrix();

    // ---- CPU (modelo jogadorIA.fbx, lado direito, ataca para -X / esquerda) ----
    glPushMatrix();
    glTranslatef(cpuX, cpuY - raioCabeca, 0.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    if (tempoAnimChuteCPU >= 0.0f) {
        float tps = modeloJogadorIA.getTicksPerSecondChute();
        modeloJogadorIA.desenharChute(tempoAnimChuteCPU * tps);
    } else {
        modeloJogadorIA.desenharRepouso();
    }
    glPopMatrix();

    // Bola (modelo bola.glb substituindo glutSolidSphere de raio raioBola=0.5)
    // O modelo tem raio ~21.417 unidades apos os transforms do glTF, e centro
    // deslocado em (-8.051, 14.307, 2.009). Escala = raioBola / 21.417.
    {
        const float bolaModeRadius = 21.4167f;
        const float bolaEscala     = raioBola / bolaModeRadius;
        const float bolaCentroX    = -8.0509f;
        const float bolaCentroY    = 14.3074f;
        const float bolaCentroZ    =  2.0092f;

        glPushMatrix();
        glTranslatef(bolaX, bolaY, 0.0f);
        glScalef(bolaEscala, bolaEscala, bolaEscala);
        glTranslatef(-bolaCentroX, -bolaCentroY, -bolaCentroZ);
        modeloBola.desenhar();
        glPopMatrix();
    }

    desenharPlacar();

    if(jogoPausado)
        desenharMenuPausa();

    glutSwapBuffers();
}

void reshape(int w,int h)
{
    if(h == 0) h = 1;

    glViewport(0,0,w,h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        45,
        (float)w/h,
        0.1,
        100
    );

    glMatrixMode(GL_MODELVIEW);
}
