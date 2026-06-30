#include "Update.h"
#include "Config.h"
#include "GameState.h"
#include "Physics.h"

#include <cmath>
#include <GL/glut.h>

void update(int)
{
    // ---- Controle de tempo (fisica independente de frame rate) ----
    // Medimos o tempo real passado e escalamos os movimentos por ele
    // (dt = 1.0 representa os 16ms originais), entao a velocidade do
    // jogo fica a mesma independente do desempenho da maquina.
    int tempoAtual = glutGet(GLUT_ELAPSED_TIME);
    float dt = (tempoAnterior == 0) ? 1.0f : (tempoAtual - tempoAnterior) / 16.0f;
    tempoAnterior = tempoAtual;

    // Limita o dt pra nao dar "saltos" grandes se o jogo gaguejar por um
    // instante (ex: ao carregar os modelos, ou uma travada pontual)
    if(dt > 4.0f) dt = 4.0f;
    if(dt < 0.0f) dt = 0.0f;

    // ---- Pausa (menu) ----
    // Enquanto o menu de pausa estiver aberto, nao avancamos NADA do
    // estado do jogo (cronometro, fisica da bola, jogadores, IA,
    // animacoes) -- preserva a partida pra quando o jogador retomar.
    if(jogoPausado)
    {
        glutPostRedisplay();
        glutTimerFunc(16, update, 0);
        return;
    }

    // Cronometro da partida -- so avanca enquanto o jogo nao esta pausado
    tempoPartidaSegundos += dt * (16.0f / 1000.0f);

    // ---- Fisica da bola ----
    velBolaY += gravidade * dt;
    bolaY += velBolaY * dt;

    if(!bolaEmSaque)
        bolaX += velBolaX * dt;

    // Atrito do ar: leve decaimento horizontal, mantido independente de dt
    velBolaX *= powf(0.999f, dt);

    // Chao
    if(bolaY < raioBola)
    {
        bolaY = raioBola;

        bool dentroGolDireito  = (bolaX >  GOL_FRENTE);
        bool dentroGolEsquerdo = (bolaX < -GOL_FRENTE);

        if(dentroGolDireito || dentroGolEsquerdo)
        {
            // Dentro do gol: quique forte para a bola conseguir sair
            velBolaY *= -0.95f;
            velBolaX += (dentroGolDireito ? -1.0f : 1.0f) * 0.025f * dt;
            if(velBolaY < 0.12f)
                velBolaY = 0.12f;
        }
        else
        {
            // Campo aberto: restituicao mais alta para quiques mais vivos;
            // atrito horizontal reduzido para a bola nao parar bruscamente.
            velBolaY *= -0.85f;
            velBolaX *= powf(0.88f, dt);

            // Anti-morte: se a bola quase parou verticalmente mas ainda
            // tem velocidade horizontal relevante, garante um salto minimo
            // para nao ficar rastejando eternamente no chao.
            if(velBolaY < 0.04f && fabsf(velBolaX) > 0.03f)
                velBolaY = 0.04f;

            // Se a bola ficou quase completamente parada, aplica um
            // micro-impulso para cima para destravar a partida.
            if(fabsf(velBolaX) < 0.02f && velBolaY < 0.02f)
                velBolaY = 0.06f;
        }

        if(bolaEmSaque)
        {
            velBolaX = 0.0f;
            bolaEmSaque = false;
        }
    }

    // Teto invisivel: bola nao pode sair da area visivel da camera
    if(bolaY + raioBola > TETO_Y && velBolaY > 0.0f)
    {
        bolaY    = TETO_Y - raioBola;
        velBolaY *= -0.75f; // rebate com leve amortecimento
    }

    // ---- Jogador (andar e pular juntos, pra pegar mais impulso) ----
    if(teclaEsquerda) jogadorX -= velMovJogador * dt;
    if(teclaDireita)  jogadorX += velMovJogador * dt;

    // Chao logico e y=0 (pivo do campo no topo do gramado).
    // Personagens repousam com o centro em y=raioCabeca.
    if(teclaCima && jogadorY <= raioCabeca + 0.05f)
        velJogadorY = 0.12f;

    // Isso representa a VELOCIDADE de quem ta cabeceando (nao um
    // deslocamento), entao nao escalamos por dt aqui.
    float movJogadorAtual = (teclaDireita ? velMovJogador : 0.0f) - (teclaEsquerda ? velMovJogador : 0.0f);

    velJogadorY += gravidade * dt;
    jogadorY += velJogadorY * dt;

    if(jogadorY < raioCabeca)
    {
        jogadorY = raioCabeca;
        velJogadorY = 0;
    }

    // ================================================================
    // IA DA CPU -- versao competitiva
    // ================================================================
    //
    // Filosofia:
    //   1. Previsao de trajetoria em varios horizontes de tempo.
    //   2. Classificacao clara da situacao (defesa / ataque / neutro).
    //   3. Alvo de posicionamento que antecipa a bola, nao apenas segue.
    //   4. Decisao de pulo baseada em quando a bola ficara no alcance.
    //   5. Direcao de cabeceio controlada: tenta sempre bater em direcao
    //      ao gol adversario (-X), nunca jogando a bola para o proprio gol.
    //   6. Velocidade de movimento ligeiramente maior que o jogador,
    //      compensada por decisoes mais conservadoras.

    // ---- Previsoes de trajetoria ----
    PrevisaoBola prev15 = preverBola(15);  // curto     : onde chegar logo
    PrevisaoBola prev30 = preverBola(30);  // medio     : posicionamento
    PrevisaoBola prev55 = preverBola(55);  // longo     : onde a bola vai cair

    // ---- Classificacao da situacao ----
    float distCpuBola  = sqrtf((bolaX-cpuX)*(bolaX-cpuX) + (bolaY-cpuY)*(bolaY-cpuY));
    bool  cpuNoChaoAgora = (cpuY <= raioCabeca + 0.05f);

    // Bola vindo em direcao ao gol da CPU (velX positiva)
    bool bolaVindoParaCpu  = (velBolaX > 0.01f);
    // Bola no lado da CPU (metade direita do campo)
    bool bolaNoLadoCpu     = (bolaX > 0.5f);
    // Bola no lado do jogador (metade esquerda)
    bool bolaNoLadoJogador = (bolaX < -2.0f);
    // Bola descendo
    bool bolaCaindo        = (velBolaY < -0.005f);

    // AMEACA: bola se aproximando do gol da CPU com velocidade relevante
    bool bolaAmeacandoGol = bolaNoLadoCpu && bolaVindoParaCpu
                            && (bolaX > GOL_X - 9.0f);

    // PERIGO IMEDIATO: bola muito perto do gol, qualquer altura
    bool perigoImediato = (bolaX > GOL_X - 4.0f);

    // CHANCE DE TOQUE: bola ao alcance do cabecio agora ou em breve
    float raioToque = raioCabeca + raioBola + 0.8f;
    bool bolaNaZonaDeToque = (distCpuBola < raioToque)
                              && (bolaY >= raioBola)
                              && (bolaY < cpuY + 3.5f);

    // ================================================================
    // ALVO HORIZONTAL
    // ================================================================
    float alvoCpuX;

    if(perigoImediato)
    {
        // PERIGO: bola quase no gol -- vai DIRETO nela, sem previsao.
        alvoCpuX = bolaX;
    }
    else if(bolaAmeacandoGol)
    {
        // DEFESA: intercepta a trajetoria prevista no curto/medio prazo.
        if(distCpuBola < 4.0f)
            alvoCpuX = prev15.x;
        else
            alvoCpuX = prev30.x;

        // Nunca recua mais do que a linha de frente do proprio gol
        if(alvoCpuX > LIMITE_CAMPO_X) alvoCpuX = LIMITE_CAMPO_X;
    }
    else if(bolaNaZonaDeToque)
    {
        // ATAQUE IMEDIATO: bola ao alcance -- posiciona-se ligeiramente
        // ATRAS da bola (em X) para que o cabeceio saia em direcao a -X.
        alvoCpuX = bolaX + 0.3f;
    }
    else if(bolaNoLadoJogador)
    {
        // POSICIONAMENTO: bola no lado do jogador, sem ameaca imediata.
        float alvoPressao  = -1.5f;  // ligeiramente no meio-campo adversario
        float alvoPrevisao = prev55.x;
        alvoCpuX = alvoPrevisao * 0.65f + alvoPressao * 0.35f;
        // Nao avanca mais do que 35% do campo adversario
        if(alvoCpuX < -LIMITE_CAMPO_X * 0.35f)
            alvoCpuX = -LIMITE_CAMPO_X * 0.35f;
    }
    else
    {
        // NEUTRO: bola no centro ou lado da CPU sem urgencia.
        if(bolaNoLadoCpu && bolaCaindo && distCpuBola < 4.5f)
            alvoCpuX = prev15.x;
        else
            alvoCpuX = prev30.x;
    }

    // Clamp ao campo
    if(alvoCpuX >  LIMITE_CAMPO_X) alvoCpuX =  LIMITE_CAMPO_X;
    if(alvoCpuX < -LIMITE_CAMPO_X) alvoCpuX = -LIMITE_CAMPO_X;

    // ================================================================
    // MOVIMENTO HORIZONTAL
    // ================================================================
    // Zona-morta adaptativa: mais apertada em situacoes de urgencia.
    float zonaMorta;
    if(perigoImediato)
        zonaMorta = 0.05f;
    else if(bolaAmeacandoGol)
        zonaMorta = CPU_ZONA_MORTA * 0.35f;
    else
        zonaMorta = CPU_ZONA_MORTA;

    float distAoAlvo  = alvoCpuX - cpuX;
    float movCpuAtual = 0.0f;

    // Velocidade de movimento: 10% maior que o jogador para compensar
    // o tempo de reacao. Esse valor e LOCAL (nao altera velMovCpu global).
    const float VEL_CPU_EFETIVA = velMovCpu * 1.10f;

    if(distAoAlvo > zonaMorta)
    {
        cpuX += VEL_CPU_EFETIVA * dt;
        movCpuAtual = VEL_CPU_EFETIVA;
    }
    else if(distAoAlvo < -zonaMorta)
    {
        cpuX -= VEL_CPU_EFETIVA * dt;
        movCpuAtual = -VEL_CPU_EFETIVA;
    }

    // ================================================================
    // DECISAO DE PULO -- baseada em "quando a bola estara no alcance"
    // ================================================================
    //
    // Em vez de apenas checar se a bola esta perto AGORA, calculamos em
    // qual frame (dentro de N passos) a bola ficara no raio de toque e
    // verificamos se um pulo iniciado AGORA chegaria a essa altura nesse
    // mesmo frame. Isso elimina pulos atrasados e pulos desperdicados.

    bool devePular = false;

    if(cpuNoChaoAgora)
    {
        // Simulamos bola e pulo juntos por ate 35 frames
        float simBolaX = bolaX, simBolaY = bolaY;
        float simVX = velBolaX, simVY = velBolaY;
        float simCpuY = raioCabeca;
        float simVCpuY = 0.12f; // velocidade do pulo

        for(int f = 1; f <= 35; f++)
        {
            // Avanca bola
            simVY += gravidade;
            simBolaY += simVY;
            simBolaX += simVX;
            if(simBolaY < raioBola)
            {
                simBolaY = raioBola;
                simVY *= -0.85f;
                simVX *= 0.88f;
            }

            // Avanca pulo da CPU (posicao X atual + movimento horizontal)
            float simCpuX = cpuX + movCpuAtual * f;
            simVCpuY += gravidade;
            simCpuY  += simVCpuY;
            if(simCpuY < raioCabeca) simCpuY = raioCabeca;

            // Checa se nesse frame a bola estaria no alcance
            float ddx = simBolaX - simCpuX;
            float ddy = simBolaY - simCpuY;
            float dd  = sqrtf(ddx*ddx + ddy*ddy);

            if(dd < raioToque + 0.3f)
            {
                // Ha uma janela de toque: vale pular agora? So pula se a
                // bola estiver acima do chao (nao adianta pular para
                // bater em bola rasteira).
                if(simBolaY > raioBola + 0.2f)
                    devePular = true;
                break;
            }
        }

        // Caso adicional: perigo imediato com bola alta -- pula mesmo
        // que a simulacao nao tenha detectado janela perfeita.
        if(!devePular && perigoImediato && bolaY > cpuY + 0.3f
           && distCpuBola < 4.0f)
            devePular = true;

        // Nao pula se a bola esta rasteira e vindo pelo chao
        if(devePular && bolaY < raioBola + 0.4f && bolaCaindo)
            devePular = false;
    }

    if(devePular)
        velCpuY = 0.12f;

    velCpuY += gravidade * dt;
    cpuY += velCpuY * dt;

    if(cpuY < raioCabeca)
    {
        cpuY = raioCabeca;
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
    bool cpuTocavaAntes = cpuTocandoBola;
    colisaoJogador(jogadorX, jogadorY, movJogadorAtual, 1.0f, jogadorTocandoBola);
    colisaoJogador(cpuX, cpuY, movCpuAtual, -1.0f, cpuTocandoBola);

    // Dispara animacao de chute da CPU no instante em que ela toca a bola
    if(!cpuTocavaAntes && cpuTocandoBola && tempoAnimChuteCPU < 0.0f)
        tempoAnimChuteCPU = 0.0f;

    // Caso especial: bola ainda entre os dois apos os dois push-outs
    resolverColisaoDupla();

    // Gol e trave dos dois lados
    tratarGol(1.0f);
    tratarGol(-1.0f);

    // Parede no limite do campo (seguranca + visual)
    tratarParedeFundo();

    // Guarda a posicao desse frame pra detectar o cruzamento de linha no proximo
    bolaXAnterior = bolaX;

    // ---- Animacao de chute do jogador (tecla Z) ----
    if(tempoAnimChuteJogador >= 0.0f)
    {
        float dtAnimSeg = (16.0f * dt) / 1000.0f;
        tempoAnimChuteJogador += dtAnimSeg * ANIM_VELOCIDADE;
        float duracaoChute = modeloJogador.getDuracaoChute();
        if(duracaoChute > 0.0f && tempoAnimChuteJogador >= duracaoChute)
            tempoAnimChuteJogador = -1.0f;
    }

    // ---- Animacao de chute da CPU ----
    if(tempoAnimChuteCPU >= 0.0f)
    {
        float dtAnimSeg = (16.0f * dt) / 1000.0f;
        tempoAnimChuteCPU += dtAnimSeg * ANIM_VELOCIDADE;
        float duracaoChute = modeloJogadorIA.getDuracaoChute();
        if(duracaoChute > 0.0f && tempoAnimChuteCPU >= duracaoChute)
            tempoAnimChuteCPU = -1.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}
