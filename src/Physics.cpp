#include "Physics.h"
#include "Config.h"
#include "GameState.h"
#include "Audio.h"

#include <cmath>

// Retorna true se a bola esta dentro do raio de colisao do personagem em (px,py)
bool bolaEmContato(float px, float py)
{
    float dx = bolaX - px;
    float dy = bolaY - py;
    float dist2 = dx*dx + dy*dy;
    float raio = raioCabeca + raioBola;
    return dist2 < raio*raio && dist2 > 0.0001f*0.0001f;
}

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
            // ── Impulso no primeiro frame de contato ──
            // Calcula velocidade relativa na direcao normal
            float velNormal = velBolaX * nx + velBolaY * ny;

            // Forca de separacao minima garantida
            float forca = 0.20f;

            // Aplica impulso apenas se a bola nao ja esta saindo rapido o suficiente
            float impulsoNormal = (velNormal < forca) ? (forca - velNormal) : 0.0f;
            velBolaX += nx * impulsoNormal;
            velBolaY += ny * impulsoNormal;

            // Empurrao ofensivo (direcao do gol adversario)
            velBolaX += direcaoAtaque * 0.05f;
            // Influencia do movimento horizontal do jogador
            velBolaX += velMovimentoJogador * 0.30f;

            // Componente Y minima para a bola subir levemente no toque
            if(velBolaY < 0.05f) velBolaY = 0.05f;

            // Limita velocidade maxima para evitar tunelamento
            const float VEL_MAX = 0.55f;
            float speed = sqrt(velBolaX*velBolaX + velBolaY*velBolaY);
            if(speed > VEL_MAX)
            {
                velBolaX = velBolaX / speed * VEL_MAX;
                velBolaY = velBolaY / speed * VEL_MAX;
            }

            jaTocando = true;
        }

        // ── Separacao posicional completa ──
        // Margem de 15% garante que no proximo frame a sobreposicao
        // nao reative imediatamente, eliminando vibracao.
        float margem = raioColisao * 0.15f;
        float sobreposicao = raioColisao - dist + margem;
        bolaX += nx * sobreposicao;
        bolaY += ny * sobreposicao;

        // Nao deixa a bola ser empurrada para baixo do chao
        if(bolaY < raioBola) bolaY = raioBola;
    }
    else
    {
        jaTocando = false;
    }
}

// ── Resolver colisao dupla (bola entre dois jogadores) ──────────────────────
void resolverColisaoDupla()
{
    bool tocaJogador = bolaEmContato(jogadorX, jogadorY);
    bool tocaCpu     = bolaEmContato(cpuX,     cpuY);

    if(!tocaJogador || !tocaCpu)
        return; // nao e colisao dupla, nada a fazer

    // Vetor da bola em relacao ao ponto medio entre os dois personagens
    float meioPX = (jogadorX + cpuX) * 0.5f;
    float meioPY = (jogadorY + cpuY) * 0.5f;

    float dx = bolaX - meioPX;
    float dy = bolaY - meioPY;
    float dist = sqrt(dx*dx + dy*dy);

    float ex, ey; // direcao de ejecao
    if(dist > 0.001f)
    {
        ex = dx / dist;
        ey = dy / dist;
    }
    else
    {
        // Bola exatamente no meio: ejeta para cima
        ex = 0.0f;
        ey = 1.0f;
    }

    // Garante componente vertical positiva (para cima) para destravar
    if(ey < 0.4f) ey = 0.4f;
    float len = sqrt(ex*ex + ey*ey);
    ex /= len; ey /= len;

    // Impulso de ejecao mais forte para garantir separacao real
    float forcaEjecao = 0.28f;
    velBolaX = ex * forcaEjecao;
    velBolaY = ey * forcaEjecao;

    // Posiciona a bola a distancia segura do meio dos dois personagens
    // na direcao de ejecao, garantindo que saia das duas zonas de colisao
    float distSegura = raioCabeca + raioBola + 0.15f;
    bolaX = meioPX + ex * distSegura;
    bolaY = meioPY + ey * distSegura;

    // Reseta os flags de toque para que ambos possam tocar novamente
    jogadorTocandoBola = false;
    cpuTocandoBola     = false;
}

// Impede que os personagens fiquem um dentro do outro. O ponto principal:
// quando um pula em cima do outro, quem esta no chao funciona como
// "ancora" -- entao quem esta no ar absorve a correcao quase toda.
void resolverColisaoPersonagens()
{
    float dx = cpuX - jogadorX;
    float dy = cpuY - jogadorY;

    float dist = sqrt(dx*dx + dy*dy);
    float minDist = raioCabeca * 2.0f;

    if(dist >= minDist)
        return;

    float nx, ny;

    if(dist > 0.0001f)
    {
        nx = dx / dist;
        ny = dy / dist;
    }
    else
    {
        // Centros exatamente coincidentes (praticamente impossivel, mas
        // evita divisao por zero): separa arbitrariamente na vertical
        nx = 0.0f;
        ny = 1.0f;
        dist = 0.0f;
    }

    float sobreposicao = minDist - dist;

    bool jogadorNoChao = jogadorY <= raioCabeca + 0.01f;
    bool cpuNoChao     = cpuY     <= raioCabeca + 0.01f;

    // Por padrao, divide a correcao igualmente. Quando só um dos dois esta
    // no chao, ele absorve 0% (fica parado) e o outro absorve 100%.
    float fatorJogador = 0.5f;
    float fatorCpu     = 0.5f;

    if(jogadorNoChao && !cpuNoChao)
    {
        fatorJogador = 0.0f;
        fatorCpu     = 1.0f;
    }
    else if(cpuNoChao && !jogadorNoChao)
    {
        fatorCpu     = 0.0f;
        fatorJogador = 1.0f;
    }

    jogadorX -= nx * sobreposicao * fatorJogador;
    jogadorY -= ny * sobreposicao * fatorJogador;

    cpuX += nx * sobreposicao * fatorCpu;
    cpuY += ny * sobreposicao * fatorCpu;

    // Zera a velocidade vertical de quem esta "pousando" em cima do outro
    if(ny > 0.3f && velCpuY < 0.0f)
        velCpuY = 0.0f;
    else if(ny < -0.3f && velJogadorY < 0.0f)
        velJogadorY = 0.0f;

    // Garante que ninguem atravessou o chao depois do empurrao
    if(jogadorY < raioCabeca)
    {
        jogadorY = raioCabeca;
        if(velJogadorY < 0) velJogadorY = 0;
    }
    if(cpuY < raioCabeca)
    {
        cpuY = raioCabeca;
        if(velCpuY < 0) velCpuY = 0;
    }
}

// O gol funciona como uma caixa com tres lados solidos (trave/topo, fundo e
// laterais) e uma abertura na frente, voltada pro campo.
//
// Gol e trave so sao decididos no INSTANTE em que a bola cruza a linha de
// frente (comparando a posicao desse frame com a do frame anterior).
void tratarGol(float direcao)
{
    float frente = direcao * GOL_FRENTE;
    float fundo  = direcao * GOL_FUNDO;

    // Borda da bola voltada para o gol (e nao o centro)
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
            tocarSomGol();
            tocarSomApito();
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
        // Senao, cruzou bem acima da trave: a bola passa livre por cima
        // do gol -- so vai bater quando alcancar o fundo fechado da caixa.
    }

    // Fundo fechado da caixa do gol
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
// campo. So rebate quando a borda da bola de fato alcanca o limite.
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

// Simula a trajetoria da bola pra frente, em uma copia local (sem alterar
// a bola de verdade). Ignora gol/trave/parede de proposito, porque pra
// decisao de posicionamento isso ja e precisao mais do que suficiente.
PrevisaoBola preverBola(int passos)
{
    float px = bolaX, py = bolaY;
    float vx = velBolaX, vy = velBolaY;

    for(int i = 0; i < passos; i++)
    {
        vy += gravidade;
        py += vy;
        px += vx;

        // Mesmo coeficiente de restituicao e atrito da fisica real
        if(py < raioBola)
        {
            py = raioBola;
            vy *= -0.85f;
            vx *= 0.88f;
            if(vy < 0.04f && fabsf(vx) > 0.03f) vy = 0.04f;
        }
    }

    return PrevisaoBola{px, py};
}
