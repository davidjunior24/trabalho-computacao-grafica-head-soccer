#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

//=====================================
// VARIAVEIS
//=====================================

// Posicoes iniciais (usadas pra voltar tudo ao normal apos cada gol)
const float JOGADOR_X_INICIAL = -6.0f;
const float CPU_X_INICIAL     =  6.0f;
const float ALTURA_INICIAL    =  1.0f;

float jogadorX = JOGADOR_X_INICIAL;
float jogadorY = ALTURA_INICIAL;
float velJogadorY = 0.0f;

float cpuX = CPU_X_INICIAL;
float cpuY = ALTURA_INICIAL;
float velCpuY = 0.0f;

float bolaX = 0.0f;
float bolaY = 5.0f;

// Posicao da bola no frame anterior, usada so pra detectar o INSTANTE em que
// ela cruza a linha do gol (em vez de ficar checando "esta depois da linha?"
// a cada frame -- isso e o que evitava o gol ser dado certo, mas tambem
// deixava qualquer bola que ja tinha passado por cima da trave contar como
// gol so por estar baixa enquanto ainda estava la dentro da caixa).
float bolaXAnterior = 0.0f;

float velBolaX = 0.0f;
float velBolaY = 0.0f;

// Enquanto a bola estiver "em saque" ela so cai (linha reta), sem
// nenhuma velocidade horizontal. So depois que ela toca o chao uma vez
// e que ganha impulso lateral.
bool bolaEmSaque = true;

float gravidade = -0.005f;

int placarJogador = 0;
int placarCPU = 0;

const float raioCabeca = 1.0f;
const float raioBola = 0.5f;

// Velocidade de movimento por frame (~16ms)
const float velMovJogador = 0.15f;
const float velMovCpu     = 0.10f;

// Estado das teclas. Usar estado (em vez de reagir so ao evento de tecla)
// permite que o jogador ande e pule ao mesmo tempo, ganhando mais impulso.
bool teclaEsquerda = false;
bool teclaDireita  = false;
bool teclaCima     = false;

// Geometria do gol -- precisa bater com o que e desenhado em desenharGol().
// O gol e um cubo de lado 3 centrado em GOL_X, ou seja, vai de
// (GOL_X - 1.5) a (GOL_X + 1.5) no eixo X. O gol agora e tratado como uma
// caixa fechada (topo, fundo e laterais solidos) com uma unica abertura na
// frente, voltada pro campo -- exatamente como um gol de verdade.
const float GOL_X       = 13.0f; // centro do gol
const float GOL_META    = 1.5f;  // metade do lado do cubo do gol
const float GOL_FRENTE  = GOL_X - GOL_META; // linha do gol = abertura, voltada pro campo
const float GOL_FUNDO   = GOL_X + GOL_META; // fundo fechado da caixa do gol
const float GOL_TRAVE_Y = 3.0f;             // altura da trave (topo da abertura)

// Parede no limite do campo, atras dos gols. Agora ela cobre toda a largura
// do campo (eixo Z) e fica no proprio limite do campo (eixo X), servindo
// como acabamento visual e como rede de seguranca.
const float PAREDE_FUNDO_X = 15.0f;
const float CAMPO_LARGURA  = 5.0f; // campo vai de -5 a 5 em Z (ver desenharCampo)
const float PAREDE_ALTURA  = 8.0f;

// Os jogadores nao podem entrar dentro do gol: o limite e a linha do
// travessao/trave, descontando o raio da cabeca (senao o CENTRO do
// personagem ficava na linha, mas o corpo dele -- que tem 1 unidade de raio
// -- continuava entrando visualmente dentro da caixa do gol).
const float LIMITE_CAMPO_X = GOL_FRENTE - raioCabeca;

// Evita que a colisao bola x cabeca seja reaplicada a cada frame enquanto
// os dois estiverem sobrepostos -- sem isso, a bola fica sendo "bombeada"
// pra cima sem parar (o cabeceio so deve valer uma vez por toque).
bool jogadorTocandoBola = false;
bool cpuTocandoBola = false;

//=====================================
// TEXTO
//=====================================

void desenharTexto(float x, float y, std::string texto)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1280, 0, 720);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);

    glColor3f(1,1,1);
    glRasterPos2f(x,y);

    for(char c : texto)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);

    glEnable(GL_LIGHTING);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
}

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

//=====================================
// COLISAO BOLA x JOGADOR
//=====================================

// velMovimentoJogador: velocidade horizontal de quem esta cabeceando
//                      (permite que andar + pular junto de mais forca no cabeceio)
// direcaoAtaque: +1 se esse personagem ataca o gol da direita, -1 se ataca o da esquerda
// jaTocando: estado (por personagem) que garante que o impulso so seja
//            aplicado uma vez por toque -- sem isso, enquanto a bola e a
//            cabeca ficam sobrepostas por alguns frames, ela era "rebatida"
//            de novo a cada frame e ficava pulando pra cima sem parar.
void colisaoJogador(float px, float py, float velMovimentoJogador, float direcaoAtaque, bool &jaTocando)
{
    float dx = bolaX - px;
    float dy = bolaY - py;

    float dist = sqrt(dx*dx + dy*dy);

    float raioColisao = raioCabeca + raioBola;

    if(dist < raioColisao && dist > 0.0001f)
    {
        float nx = dx / dist;
        float ny = dy / dist;

        if(!jaTocando)
        {
            // A bola sempre sai na direcao REAL do toque (nx, ny) -- e isso, e
            // nao a intencao ofensiva do personagem, que decide pra onde ela
            // vai. Antes a direcao do gol adversario pesava mais que o toque
            // em si, e por isso a bola podia ser "puxada" pro lado contrario
            // de onde realmente bateu (ex: tocar nas costas da cabeca e ainda
            // assim ir pra frente). Agora isso nao acontece mais.
            float forca = 0.16f;
            velBolaX = nx * forca;
            velBolaY = fabs(ny) * forca + 0.05f;

            // A intencao ofensiva (mandar pro gol adversario) e a corrida de
            // quem cabeceou so reforcam essa direcao com um empurraozinho
            // extra -- nunca multiplicam o suficiente pra inverter o sentido
            // real do toque.
            velBolaX += direcaoAtaque * 0.03f;
            velBolaX += velMovimentoJogador * 0.25f;

            jaTocando = true;
        }

        // Empurra a bola pra fora da cabeca, pra ela nao ficar "colada"/atravessando
        float sobreposicao = raioColisao - dist;
        bolaX += nx * sobreposicao;
        bolaY += ny * sobreposicao;
    }
    else
    {
        jaTocando = false;
    }
}

//=====================================
// COLISAO JOGADOR x CPU
//=====================================

// Impede que os personagens fiquem um dentro do outro
void resolverColisaoPersonagens()
{
    float dx = cpuX - jogadorX;
    float dy = cpuY - jogadorY;

    float dist = sqrt(dx*dx + dy*dy);
    float minDist = raioCabeca * 2.0f;

    if(dist < minDist && dist > 0.0001f)
    {
        float nx = dx / dist;
        float ny = dy / dist;

        float sobreposicao = (minDist - dist) * 0.5f;

        jogadorX -= nx * sobreposicao;
        jogadorY -= ny * sobreposicao;

        cpuX += nx * sobreposicao;
        cpuY += ny * sobreposicao;

        // Cancela a componente vertical da velocidade que aponta para dentro
        // da colisao: impede que o personagem que esta em cima continue
        // "afundando" no de baixo frame a frame por causa do momentum de queda.
        // ny > 0 significa que a CPU esta acima do jogador:
        //   - A CPU foi empurrada pra cima (ny > 0): cancela velCpuY se estava caindo
        //   - O Jogador foi empurrado pra baixo (-ny): cancela velJogadorY se estava subindo
        // ny < 0 e o caso simetrico (jogador em cima da CPU).
        if(ny > 0)
        {
            // CPU esta acima: se estava caindo, cancela a queda
            if(velCpuY < 0) velCpuY = 0;
            // Jogador esta abaixo: se estava subindo em direcao a CPU, cancela
            if(velJogadorY > 0) velJogadorY = 0;
        }
        else
        {
            // Jogador esta acima: se estava caindo, cancela a queda
            if(velJogadorY < 0) velJogadorY = 0;
            // CPU esta abaixo: se estava subindo em direcao ao jogador, cancela
            if(velCpuY > 0) velCpuY = 0;
        }

        // Apos a resolucao, garante que nenhum personagem atravessou o chao.
        // Isso e necessario porque o empurrao vertical pode afundar o
        // personagem que estava embaixo pra dentro do solo.
        if(jogadorY < 1.0f)
        {
            jogadorY = 1.0f;
            if(velJogadorY < 0) velJogadorY = 0;
        }
        if(cpuY < 1.0f)
        {
            cpuY = 1.0f;
            if(velCpuY < 0) velCpuY = 0;
        }
    }
}

//=====================================
// GOL / TRAVE / PAREDE DE FUNDO
//=====================================

// O gol funciona como uma caixa com tres lados solidos (trave/topo, fundo e
// laterais) e uma abertura na frente, voltada pro campo.
//
// O ponto-chave: gol e trave so sao decididos no INSTANTE em que a bola
// cruza a linha de frente (comparando a posicao desse frame com a do frame
// anterior) -- nunca pela posicao "atual" sozinha. Sem isso, uma bola que
// passasse por cima da trave (sem bater nela) e depois caisse dentro da
// caixa contava como gol so por estar baixa enquanto ainda estava la
// dentro, mesmo tendo entrado por cima -- o que é errado, ela deveria ter
// rebatido na trave.
//
// direcao: +1 trata o gol da direita (jogador ataca), -1 trata o da esquerda (cpu ataca)
void tratarGol(float direcao)
{
    float frente = direcao * GOL_FRENTE;
    float fundo  = direcao * GOL_FUNDO;

    // Borda da bola voltada para o gol (e nao o centro -- e essa borda que
    // realmente toca a linha/trave/parede primeiro)
    float bordaAtual    = bolaX        + direcao * raioBola;
    float bordaAnterior = bolaXAnterior + direcao * raioBola;

    bool cruzouFrente = (direcao > 0)
        ? (bordaAnterior <= frente && bordaAtual > frente)
        : (bordaAnterior >= frente && bordaAtual < frente);

    if(cruzouFrente)
    {
        if(bolaY < GOL_TRAVE_Y - raioBola)
        {
            // Cruzou a linha por dentro da abertura: GOL!
            if(direcao > 0)
                placarJogador++;
            else
                placarCPU++;

            resetarBola();
            resetarPosicoes();
            return;
        }
        else if(bolaY <= GOL_TRAVE_Y + raioBola)
        {
            // Cruzou a linha na altura da trave: bateu nela, volta
            velBolaX *= -0.85f;
            return;
        }
        // Senao, cruzou bem acima da trave: nao ha contato real aqui, a
        // bola passa livre por cima do gol -- ela so vai bater quando
        // alcancar o fundo fechado da caixa, tratado abaixo.
    }

    // Fundo fechado da caixa do gol: se a bola ja estiver "por dentro" da
    // caixa (passou da linha de frente, geralmente por ter ido por cima da
    // trave) e cruzar o fundo, bate e volta -- nao importa a altura, porque
    // o fundo da caixa e totalmente solido.
    bool dentroDaCaixa = (direcao > 0) ? (bordaAtual > frente) : (bordaAtual < frente);

    if(dentroDaCaixa)
    {
        bool cruzouFundo = (direcao > 0)
            ? (bordaAnterior <= fundo && bordaAtual > fundo)
            : (bordaAnterior >= fundo && bordaAtual < fundo);

        if(cruzouFundo)
            velBolaX *= -0.85f;
    }

    // Face superior (topo) da caixa do gol.
    // Checada com as coordenadas absolutas da caixa, fora do bloco
    // dentroDaCaixa -- assim captura tanto a bola que ja esta dentro
    // quanto a que cai verticalmente de cima sobre o topo.
    //
    // xMin/xMax sao os limites horizontais reais da caixa no mundo.
    // A bola so e barrada se o seu centro X estiver dentro desse intervalo.
    // O gatilho e a borda INFERIOR da bola (bolaY - raioBola) descendo ate
    // GOL_TRAVE_Y, pois e essa borda que toca o topo primeiro.
    {
        float xMin = (direcao > 0) ? frente : fundo;
        float xMax = (direcao > 0) ? fundo  : frente;

        bool sobreACaixa = (bolaX > xMin && bolaX < xMax);

        if(sobreACaixa && velBolaY < 0 && (bolaY - raioBola) <= GOL_TRAVE_Y)
        {
            bolaY = GOL_TRAVE_Y + raioBola;
            velBolaY *= -0.85f;
        }
    }
}

// Parede no limite do campo (atras dos gols), cobrindo toda a largura do
// campo. So rebate quando a borda da bola de fato alcanca o limite, e so
// enquanto ela ainda estiver indo pra fora do campo -- nada de rebote
// antecipado nem de ficar batendo repetidas vezes no mesmo toque.
void tratarParedeFundo()
{
    if(bolaX + raioBola > PAREDE_FUNDO_X && velBolaX > 0)
    {
        velBolaX *= -0.9f;
    }
    else if(bolaX - raioBola < -PAREDE_FUNDO_X && velBolaX < 0)
    {
        velBolaX *= -0.9f;
    }
}

//=====================================
// UPDATE
//=====================================

void update(int)
{
    // ---- Fisica da bola ----
    velBolaY += gravidade;
    bolaY += velBolaY;

    if(!bolaEmSaque)
        bolaX += velBolaX;

    // Leve atrito do ar: a bola perde um pouco de energia horizontal com o
    // tempo, em vez de quicar pra sempre com a mesma forca (deixa menos artificial)
    velBolaX *= 0.999f;

    // Chao
    if(bolaY < raioBola)
    {
        bolaY = raioBola;
        velBolaY *= -0.8f;

        if(bolaEmSaque)
        {
            // So ganha velocidade horizontal depois de cair reto e tocar o chao
            velBolaX = (rand()%2==0) ? 0.08f : -0.08f;
            bolaEmSaque = false;
        }
    }

    // ---- Jogador (andar e pular juntos, pra pegar mais impulso) ----
    if(teclaEsquerda) jogadorX -= velMovJogador;
    if(teclaDireita)  jogadorX += velMovJogador;

    if(teclaCima && jogadorY <= 1.05f)
        velJogadorY = 0.12f;

    float movJogadorAtual = (teclaDireita ? velMovJogador : 0.0f) - (teclaEsquerda ? velMovJogador : 0.0f);

    velJogadorY += gravidade;
    jogadorY += velJogadorY;

    if(jogadorY < 1)
    {
        jogadorY = 1;
        velJogadorY = 0;
    }

    // ---- CPU ----
    float movCpuAtual = 0.0f;

    if(bolaX > cpuX + 0.1f)
    {
        cpuX += velMovCpu;
        movCpuAtual = velMovCpu;
    }
    else if(bolaX < cpuX - 0.1f)
    {
        cpuX -= velMovCpu;
        movCpuAtual = -velMovCpu;
    }

    // IA agora pula pra cabecear quando a bola esta de fato a seu alcance
    // (distancia real, nao so uma faixa larga de X/Y), e somente quando a
    // bola esta acima dela. Isso evita que a CPU saia pulando toda vez que
    // a bola so passa perto, e -- combinado com o toque unico por contato --
    // evita que ela fique "cabeceando" a bola pra cima sem parar.
    float distCpuBola = sqrt((bolaX-cpuX)*(bolaX-cpuX) + (bolaY-cpuY)*(bolaY-cpuY));

    if(distCpuBola < 2.6f && bolaY > cpuY && cpuY <= 1.05f)
        velCpuY = 0.12f;

    velCpuY += gravidade;
    cpuY += velCpuY;

    if(cpuY < 1)
    {
        cpuY = 1;
        velCpuY = 0;
    }

    // ---- Limites do campo (jogadores nao podem entrar dentro do gol) ----
    if(jogadorX < -LIMITE_CAMPO_X) jogadorX = -LIMITE_CAMPO_X;
    if(jogadorX >  LIMITE_CAMPO_X) jogadorX =  LIMITE_CAMPO_X;
    if(cpuX     < -LIMITE_CAMPO_X) cpuX     = -LIMITE_CAMPO_X;
    if(cpuX     >  LIMITE_CAMPO_X) cpuX     =  LIMITE_CAMPO_X;

    // Jogador e CPU nao podem se atravessar
    resolverColisaoPersonagens();

    // Colisoes com a bola (uma vez por toque, ver colisaoJogador)
    colisaoJogador(jogadorX, jogadorY, movJogadorAtual, 1.0f, jogadorTocandoBola);
    colisaoJogador(cpuX, cpuY, movCpuAtual, -1.0f, cpuTocandoBola);

    // Gol e trave dos dois lados
    tratarGol(1.0f);
    tratarGol(-1.0f);

    // Parede no limite do campo (seguranca + visual)
    tratarParedeFundo();

    // Guarda a posicao desse frame pra detectar o cruzamento de linha no proximo
    bolaXAnterior = bolaX;

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

//=====================================
// TECLADO
//=====================================

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

//=====================================
// CAMPO
//=====================================

void desenharCampo()
{
    glColor3f(0,0.7f,0);

    glBegin(GL_QUADS);

    glNormal3f(0,1,0);

    glVertex3f(-15,0,-5);
    glVertex3f(15,0,-5);
    glVertex3f(15,0,5);
    glVertex3f(-15,0,5);

    glEnd();
}

//=====================================
// GOLS
//=====================================

// direcao: +1 (gol da direita) ou -1 (gol da esquerda). A caixa do gol fica
// fechada no topo, no fundo e nas duas laterais -- a unica abertura e a
// face voltada pro centro do campo, que e por onde a bola precisa passar
// pra valer gol (ver tratarGol).
void desenharGol(float x, float direcao)
{
    glPushMatrix();

    glTranslatef(x,1.5f,0);

    // Estrutura da trave/postes, em wireframe
    glColor3f(1,1,1);
    glutWireCube(3);

    // "Rede": fecha topo, fundo e as duas laterais. A face da frente
    // (voltada pro campo, por onde a bola entra) fica de propósito sem
    // nenhum painel, pra representar a abertura do gol.
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f,1.0f,1.0f,0.25f);

    float m = GOL_META;

    glBegin(GL_QUADS);
        // fundo (face oposta a abertura, "fecha" o gol por tras)
        glVertex3f(direcao*m,-m,-m);
        glVertex3f(direcao*m, m,-m);
        glVertex3f(direcao*m, m, m);
        glVertex3f(direcao*m,-m, m);

        // topo (trave solida, e o que faz a bola bater e voltar)
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

    glPopMatrix();
}

// Parede no limite do campo, atras dos gols. Cobre toda a largura do campo
// (eixo Z) e fica alta, servindo de acabamento visual pro cenario e de rede
// de seguranca (a bola so chegaria ali numa situacao de bug, ja que o gol
// agora e uma caixa fechada).
void desenharParedeFundo(float x)
{
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

    desenharGol(-13, -1.0f);
    desenharGol(13, 1.0f);

    desenharParedeFundo(-PAREDE_FUNDO_X);
    desenharParedeFundo(PAREDE_FUNDO_X);

    // Jogador
    glPushMatrix();

    glColor3f(0,0,1);

    glTranslatef(
        jogadorX,
        jogadorY,
        0
    );

    glutSolidSphere(
        raioCabeca,
        30,
        30
    );

    glPopMatrix();

    // CPU
    glPushMatrix();

    glColor3f(1,0,0);

    glTranslatef(
        cpuX,
        cpuY,
        0
    );

    glutSolidSphere(
        raioCabeca,
        30,
        30
    );

    glPopMatrix();

    // Bola
    glPushMatrix();

    glColor3f(1,1,1);

    glTranslatef(
        bolaX,
        bolaY,
        0
    );

    glutSolidSphere(
        raioBola,
        30,
        30
    );

    glPopMatrix();

    desenharTexto(
        550,
        680,
        std::to_string(placarJogador)
        + " x " +
        std::to_string(placarCPU)
    );

    glutSwapBuffers();
}

//=====================================
// INIT
//=====================================

void init()
{
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glEnable(GL_COLOR_MATERIAL);

    glClearColor(
        0.4f,
        0.7f,
        1.0f,
        1.0f
    );

    srand((unsigned int)time(nullptr));

    resetarBola();
    resetarPosicoes();
}

//=====================================
// RESHAPE
//=====================================

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

//=====================================
// MAIN
//=====================================

int main(int argc,char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(
        GLUT_DOUBLE |
        GLUT_RGB |
        GLUT_DEPTH
    );

    glutInitWindowSize(
        1280,
        720
    );

    glutCreateWindow(
        "Head Soccer 3D"
    );

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    glutSpecialFunc(
        tecladoEspecial
    );

    glutSpecialUpFunc(
        tecladoEspecialSolto
    );

    glutTimerFunc(
        16,
        update,
        0
    );

    glutMainLoop();

    return 0;
}