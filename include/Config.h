#ifndef CONFIG_H
#define CONFIG_H

//=====================================
// CONFIG.H -- constantes do jogo
//=====================================
// Todas as constantes (posicoes iniciais, geometria do gol, dimensoes do
// HUD, caminhos de assets etc.) usadas pelos outros modulos. Nada aqui tem
// estado mutavel -- isso fica em GameState.h.

// ---- Posicoes iniciais ----
const float JOGADOR_X_INICIAL = -12.0f; // dentro do gol esquerdo (X vermelho da imagem)
const float CPU_X_INICIAL     =  12.0f; // dentro do gol direito  (X vermelho da imagem)

const float raioCabeca = 1.0f;
const float ALTURA_INICIAL = raioCabeca; // = 1.0f
const float raioBola = 0.5f;

// Velocidade de movimento por frame (~16ms)
const float velMovJogador = 0.15f;
const float velMovCpu     = velMovJogador; // mesma velocidade do jogador humano

const float gravidade = -0.005f;

// ---- HUD: dimensoes da janela ----
const float LARGURA_JANELA = 1280.0f;
const float ALTURA_JANELA  = 720.0f;

// ---- PLACAR ----
const float PLACAR_PROPORCAO_LINHA_MARCADOR = 0.46f;

// ---- Geometria do gol ----
// GOL_X = 16: distancia do centro do campo ate o centro da trave.
const float GOL_X       = 16.0f; // centro do gol
const float GOL_META    = 1.5f;  // metade do lado do cubo do gol
const float GOL_FRENTE  = GOL_X - GOL_META; // linha do gol = abertura, voltada pro campo
const float GOL_FUNDO   = GOL_X + GOL_META; // fundo fechado da caixa do gol
const float GOL_TRAVE_Y = 3.0f;             // altura da trave (topo da abertura)

// Teto invisivel: impede a bola de sair da area visivel da camera.
const float TETO_Y = 8.5f;

const float PAREDE_FUNDO_X = 16.0f;
const float CAMPO_LARGURA  = 5.0f; // campo vai de -5 a 5 em Z
const float PAREDE_ALTURA  = 20.0f;

// Os jogadores nao podem entrar dentro do gol.
const float LIMITE_CAMPO_X = GOL_FRENTE - raioCabeca;

// ---- IA da CPU ----
const float CPU_ZONA_MORTA = 0.25f;

// ---- Caminhos dos modelos 3D ----
const char* const CAMINHO_MODELO_CAMPO        = "assets/models/campo.glb";
const char* const CAMINHO_MODELO_TRAVE        = "assets/models/trave.glb";
const char* const CAMINHO_MODELO_ARQUIBANCADA = "assets/models/arquibancadas.glb";
const char* const CAMINHO_MODELO_REFLETOR     = "assets/models/refletor.glb";
const char* const CAMINHO_MODELO_BOLA         = "assets/models/bola.glb";
const char* const CAMINHO_MODELO_NUVEM        = "assets/models/nuvem.glb";
const char* const CAMINHO_MODELO_JOGADOR      = "assets/models/jogador.fbx";
const char* const CAMINHO_MODELO_CPU          = "assets/models/jogadorIA.fbx";

// ---- Caminhos de audio (SDL2_mixer) ----
const char* const CAMINHO_SOM_GOL      = "assets/models/sounds/gol.wav";
const char* const CAMINHO_SOM_APITO    = "assets/models/sounds/apito.wav";
const char* const CAMINHO_MUSICA_FUNDO = "assets/models/sounds/musicafundo.wav";

// ---- Ajustes visuais: campo ----
const float CAMPO_OFFSET_X = 0.0f;
const float CAMPO_OFFSET_Y = -10.5f;
const float CAMPO_OFFSET_Z = 0.0f;

// ---- Ajustes visuais: trave ----
const float TRAVE_OFFSET_X = 0.0f;
const float TRAVE_OFFSET_Y = -1.8f;
const float TRAVE_OFFSET_Z = 0.0f;
const float TRAVE_ROTACAO_BASE = 0.0f;

const bool PAREDE_FUNDO_VISIVEL = false;
const bool TRAVE_ANTIGA_VISIVEL = false;

// ---- Ajustes visuais: arquibancada ----
const float ARQUIBANCADA_OFFSET_X = -6.3f;
const float ARQUIBANCADA_OFFSET_Y = -2.1f;
const float ARQUIBANCADA_OFFSET_Z = 10.2f;
const float ARQUIBANCADA_ESCALA   = 1.0f;

// ---- Ajustes visuais: refletores ----
const float REFLETOR_X      =  17.1f;
const float REFLETOR_Y      =  -0.8f;
const float REFLETOR_Z      =  11.0f;
const float REFLETOR_ESCALA =  1.0f;

// ---- Ajustes visuais: nuvens ----
const float NUVEM_Y      = 6.4f;
const float NUVEM_Z      = -18.0f;
const float NUVEM_ESCALA =  0.8f;
const float NUVEM_CENTRO_X =  1.964f;
const float NUVEM_CENTRO_Y =  0.002f;
const float NUVEM_CENTRO_Z = -0.611f;

// ---- Modelo animado (jogador FBX) ----
// Indice das animacoes de chute no FBX
const unsigned int ANIM_CHUTE_CABECA = 0; // anima Object_3 (cabeca/cabelo)
const unsigned int ANIM_CHUTE_CORPO  = 2; // anima Object_3.001 (corpo/roupa)
const float ANIM_VELOCIDADE = 1.5f;       // velocidade da animacao (multiplo de tempo real)

#endif // CONFIG_H
