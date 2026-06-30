#ifndef PHYSICS_H
#define PHYSICS_H

//=====================================
// FISICA -- colisao bola/jogador, gol, trave, parede, previsao
//=====================================

// Retorna true se a bola esta dentro do raio de colisao do personagem em (px,py)
bool bolaEmContato(float px, float py);

// velMovimentoJogador: velocidade horizontal de quem esta cabeceando
//                      (permite que andar + pular junto de mais forca no cabeceio)
// direcaoAtaque: +1 se esse personagem ataca o gol da direita, -1 se ataca o da esquerda
// jaTocando: estado (por personagem) que garante que o impulso so seja
//            aplicado uma vez por toque.
void colisaoJogador(float px, float py, float velMovimentoJogador, float direcaoAtaque, bool &jaTocando);

// Chamado DEPOIS de ambos os colisaoJogador(). Se a bola ainda estiver em
// contato com AMBOS simultaneamente, expele a bola para destravar.
void resolverColisaoDupla();

// Impede que jogador e CPU fiquem um dentro do outro.
void resolverColisaoPersonagens();

// direcao: +1 trata o gol da direita (jogador ataca), -1 trata o da esquerda (cpu ataca)
void tratarGol(float direcao);

// Parede no limite do campo (atras dos gols).
void tratarParedeFundo();

// Resultado de uma previsao de trajetoria da bola.
struct PrevisaoBola
{
    float x;
    float y;
};

// Simula a trajetoria da bola pra frente (sem alterar a bola de verdade),
// usada pela IA para reagir a pra ONDE a bola esta indo.
PrevisaoBola preverBola(int passos);

#endif // PHYSICS_H
