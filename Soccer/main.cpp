#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

// Audio do gol (Windows Multimedia API -- nao precisa instalar nada extra)
#include <windows.h>
#include <mmsystem.h>
#include <thread>
#pragma comment(lib, "winmm.lib")

// ============================================================
// DEPENDENCIA EXTERNA: Assimp
// ============================================================
// O campo e a trave agora sao modelos 3D (.glb) feitos no Blender,
// carregados com a biblioteca Assimp, em vez de geometria desenhada
// na mao com glBegin/glEnd.
//
// Pra compilar (Linux/Ubuntu, exemplo):
//   sudo apt install libassimp-dev
//   g++ main.cpp -o jogo -lGL -lGLU -lglut -lassimp
//
// Os arquivos .glb precisam estar em "assets/models/" a partir da pasta de
// onde o executavel e executado. Se voce rodar de outro lugar (ex: de
// dentro de uma pasta build/), ajuste os caminhos em CAMINHO_MODELO_CAMPO,
// CAMINHO_MODELO_TRAVE e CAMINHO_MODELO_ARQUIBANCADA mais abaixo, ou copie
// a pasta assets pra junto do executavel.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/texture.h>

// ============================================================
// DEPENDENCIA EXTERNA: stb_image
// ============================================================
// Os modelos do Blender (gerados pelo Meshy AI) usam TEXTURA (imagem) no
// Base Color, nao cor solida de material nem vertex color em varias
// partes (grama do campo, arquibancada, etc.) -- por isso apareciam
// brancos/sem cor. O Assimp só nos dá os BYTES brutos da imagem (PNG/JPG)
// embutida no .glb; pra transformar isso em pixels que o OpenGL consiga
// usar como textura, precisamos decodificar essa imagem, e isso requer
// uma biblioteca de imagem. Usamos a stb_image.h, um header-only muito
// simples e usado em praticamente todo projeto C/C++ que precisa disso
// (sem precisar instalar nem linkar nenhuma lib nova).
//
// Baixe o arquivo (1 arquivo so) em:
//   https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
// e coloque na MESMA pasta do main.cpp.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//=====================================
// VARIAVEIS
//=====================================

// Posicoes iniciais (usadas pra voltar tudo ao normal apos cada gol)
const float JOGADOR_X_INICIAL = -12.0f; // dentro do gol esquerdo (X vermelho da imagem)
const float CPU_X_INICIAL     =  12.0f; // dentro do gol direito  (X vermelho da imagem)
// Com o pivo do campo no topo do gramado (y=0 = superficie), os personagens
// devem repousar com o centro em y = raioCabeca (1.0), nao em y=1.0 fixo
// (antes o chao logico era y=1, agora e y=0).

const float raioCabeca = 1.0f;

const float ALTURA_INICIAL    =  raioCabeca; // = 1.0f

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


const float raioBola = 0.5f;

// Velocidade de movimento por frame (~16ms) -- aumentadas pra deixar o jogo
// mais agil (valores anteriores eram 0.15 e 0.10, muito lentos).
const float velMovJogador = 0.15f;
const float velMovCpu     = velMovJogador; // mesma velocidade do jogador humano

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
//
// GOL_X = 14: distancia correta do centro do campo ate o centro da trave,
// conforme medido no modelo do Blender. Campo tem comprimento 35 (-17.5 a +17.5).
const float GOL_X       = 16.0f; // centro do gol (correto, medido no Blender)
const float GOL_META    = 1.5f;  // metade do lado do cubo do gol
const float GOL_FRENTE  = GOL_X - GOL_META; // linha do gol = abertura, voltada pro campo
const float GOL_FUNDO   = GOL_X + GOL_META; // fundo fechado da caixa do gol
const float GOL_TRAVE_Y = 3.0f;             // altura da trave (topo da abertura)

// Parede de seguranca no limite real do campo. O campo tem comprimento 35
// (de -17.5 a +17.5). As traves ficam centradas em x=14 e tem GOL_META=1.5
// de profundidade, indo ate x=15.5. A parede fica em x=17.5, na borda real
// do campo, totalmente atras das traves -- invisivel, so fisica.
// Teto invisivel: impede a bola de sair da area visivel da camera.
// Calculado geometricamente: com gluLookAt(0,10,20,0,0,0) e FOV=45,
// a bola (em x=0,z=0) sai do topo da tela em Y~9. O teto fica em
// Y=8.5 para manter a bola sempre visivel com uma margem segura.
const float TETO_Y = 8.5f;

const float PAREDE_FUNDO_X = 16.0f;
const float CAMPO_LARGURA  = 5.0f; // campo vai de -5 a 5 em Z (ver desenharCampo)
const float PAREDE_ALTURA  = 20.0f;

// Os jogadores nao podem entrar dentro do gol: o limite e a linha do
// travessao/trave, descontando o raio da cabeca (senao o CENTRO do
// personagem ficava na linha, mas o corpo dele -- que tem 1 unidade de raio
// -- continuava entrando visualmente dentro da caixa do gol).
const float LIMITE_CAMPO_X = GOL_FRENTE - raioCabeca;

// ============================================================
// IA DA CPU
// ============================================================
// A CPU defende o gol da DIREITA (x = +GOL_X) e ataca o da ESQUERDA
// (x = -GOL_X) -- ver direcaoAtaque=-1 passado pra colisaoJogador() mais
// abaixo, e tratarGol(-1) marcando ponto pra ela.

// Posicao "de casa": onde a CPU fica quando a bola esta fora de alcance
// imediato, em vez de sair correndo atras dela o jogo todo. Um pouco
// avancada da propria trave, pronta pra reagir nos dois sentidos.
const float CPU_POSICAO_BASE_X = 12.0f; // posicao de repouso da CPU (dentro do proprio gol)

// Quanto a CPU se "compromete" a ir ao encontro da bola quando ela esta ao
// alcance (1.0 = vai direto nela; valores menores misturam com a posicao
// base, deixando o posicionamento menos "cola-na-bola").
const float CPU_FATOR_ENGAJAR = 0.6f;

// A partir de quantas unidades (em X) a bola e considerada "fora de
// alcance imediato" -- nesse caso a CPU para de perseguir e volta pra
// posicao de casa, em vez de atravessar o campo todo atras dela.
const float CPU_LIMIAR_ENGAJAR = 30.0f; // CPU sempre tenta engajar
                                         // (campo tem ~27u, 30 > qualquer distancia)

// A partir de qual distancia (em X) do PROPRIO gol a bola e considerada
// uma ameaca -- nessa faixa a CPU prioriza defesa (vai direto interceptar
// a bola, sem se preocupar em "economizar" posicionamento).
const float CPU_LIMIAR_DEFESA_X = GOL_X - 7.0f;

// Zona-morta de movimento: evita que a CPU fique "tremendo"/trocando de
// direcao toda hora quando ja esta bem perto do alvo desejado.
const float CPU_ZONA_MORTA = 0.25f;

// Evita que a colisao bola x cabeca seja reaplicada a cada frame enquanto
// os dois estiverem sobrepostos -- sem isso, a bola fica sendo "bombeada"
// pra cima sem parar (o cabeceio so deve valer uma vez por toque).
bool jogadorTocandoBola = false;
bool cpuTocandoBola = false;

// ============================================================
// MODELOS 3D (CAMPO E TRAVE) -- AJUSTES VISUAIS
// ============================================================
// Nada aqui afeta fisica, colisao ou deteccao de gol -- e tudo so
// renderizacao. Os valores de offset comecam em uma estimativa razoavel
// (considerando "Origin to Geometry" no Blender) mas podem precisar de um
// pequeno ajuste fino depois de ver o resultado na tela.

const char* CAMINHO_MODELO_CAMPO = "assets/models/Campo_Oficial.glb";
const char* CAMINHO_MODELO_TRAVE = "assets/models/Trave_Oficial.glb";
const char* CAMINHO_MODELO_ARQUIBANCADA = "assets/models/Arquibancadas_Oficial.glb";
const char* CAMINHO_MODELO_REFLETOR     = "assets/models/low_poly_reflector.glb";
const char* CAMINHO_MODELO_BOLA         = "assets/models/bola.glb";
const char* CAMINHO_MODELO_NUVEM        = "assets/models/low_poly_cloud.glb";

// Caminho do audio de gol (WAV puro, sem compressao)
const char* CAMINHO_SOM_GOL = "assets/models/sounds/sound_goal.wav";

// Buffer que guarda o WAV inteiro na RAM apos carregarSomGol().
// PlaySound com SND_MEMORY toca direto da RAM, sem acesso ao disco,
// eliminando o delay das primeiras reproducoes.
static std::vector<char> bufferSomGol;

// O Origin do campo foi movido (no Blender) pra parte externa, no topo do
// gramado -- ou seja, o (0,0,0) do modelo ja e exatamente a superficie do
// gramado. Como a fisica do jogo tambem considera o chao em y=0, nao
// precisamos mais de nenhum offset vertical pra alinhar os dois.
const float CAMPO_OFFSET_X = 0.0f;
const float CAMPO_OFFSET_Y = -10.5f;
const float CAMPO_OFFSET_Z = 0.0f;

// O Origin da trave fica no centro da geometria (igual ao cubo antigo,
// glutWireCube(3) -- ver TRAVE_ANTIGA_VISIVEL abaixo). GOL_X agora ja e
// o valor correto (14), entao nao precisa de nenhum offset horizontal.
// O modelo visual e desenhado diretamente sobre o cubo logico, sem ajuste.
// TRAVE_OFFSET_Y = 0: o pivo do campo esta no topo do gramado (y=0), e
// o centro do cubo logico e posto em y=GOL_META (= 1.5), que e o meio
// da caixa do gol -- o modelo da trave deve ter o pivo no seu centro
// geometrico tambem, entao nenhum offset vertical e necessario.
const float TRAVE_OFFSET_X = 0.0f;
const float TRAVE_OFFSET_Y = -1.8f;
const float TRAVE_OFFSET_Z = 0.0f;

// O arquivo .glb tem só UM modelo de trave, usado nos dois gols (o da
// direita normal, o da esquerda espelhado com 180 graus). Esse valor é a
// rotacao adicional, em graus, pra fazer a ABERTURA do gol da direita
// ficar voltada pro centro do campo (eixo -X). Como nao sabemos de
// antemao pra que lado o modelo foi modelado no Blender, comece com 0 e,
// se a abertura aparecer voltada pro lado errado, troque por 90, 180 ou
// 270 até alinhar.
const float TRAVE_ROTACAO_BASE = 0.0f;

// A parede no limite do campo continua funcionando exatamente igual
// (fisica intacta, ver tratarParedeFundo), mas agora fica invisivel.
// Troque pra "true" se quiser voltar a ve-la (por exemplo, pra debugar).
const bool PAREDE_FUNDO_VISIVEL = false;

// A geometria antiga da trave (o cubo wireframe + os paineis da "rede",
// que existiam antes do modelo do Blender) tambem continua existindo no
// codigo, como referencia -- ela e desenhada com o centro de gravidade
// exatamente no meio do cubo (em GOL_X, 1.5, 0), igual a fisica sempre
// considerou, e e a partir desse mesmo centro que o modelo novo da trave e
// posicionado (com o ajuste de TRAVE_OFFSET_X acima por cima). Ela fica
// invisivel por padrao; troque pra "true" se quiser compara-la visualmente
// com o modelo novo (por exemplo, pra calibrar os offsets).
const bool TRAVE_ANTIGA_VISIVEL = false;

// ============================================================
// MODELO 3D (ARQUIBANCADA) -- AJUSTE VISUAL
// ============================================================
// Arquibancada (Arquibancadas_Oficial.glb) -- fica atras do campo (no lado
// oposto a camera, eixo Z negativo), so de fundo/cenario: nao tem fisica
// nem colisao nenhuma associada a ela.
//
// Dimensoes do modelo informadas (medidas no Blender): X=53, Y=70.6, Z=7.
// Como nao da pra saber de antemao exatamente onde o Blender colocou o
// "Origin" do objeto, X e Y abaixo comecam em 0 -- ajuste-os depois de ver
// o resultado na tela (ex: se a arquibancada aparecer flutuando ou
// enterrada no chao, mude ARQUIBANCADA_OFFSET_Y; se aparecer descentrada
// em relacao ao campo, mude ARQUIBANCADA_OFFSET_X).
const float ARQUIBANCADA_OFFSET_X = -6.3f;
const float ARQUIBANCADA_OFFSET_Y = -2.1f;

// REFLETOR -- Torres de iluminacao nos cantos da arquibancada
// BB do modelo: X~1.9  Y~8.0  Z~3.1 (unidades Blender = unidades do jogo)
// Ajuste REFLETOR_X para afastar/aproximar dos gols,
//        REFLETOR_Y para subir/descer,
//        REFLETOR_Z para afastar/aproximar da lateral.
const float REFLETOR_X      =  17.1f;   // alinhado com as traves (±16)
const float REFLETOR_Y      =  -0.8f;    // base no chao
const float REFLETOR_Z      =  11.0f;    // fora da lateral do campo
const float REFLETOR_ESCALA =  1.0f;    // mesma escala dos outros modelos


// Profundidade fixa: o suficiente pra deixar a arquibancada inteiramente
// atras da lateral do campo (CAMPO_LARGURA = 5), sem encostar nela.
const float ARQUIBANCADA_OFFSET_Z = 10.2f;

// Caso o modelo apareca desproporcional em relacao ao resto do cenario
// (as dimensoes informadas sao bem maiores que as do campo/trave), use
// essa escala uniforme pra redimensionar. 1.0 = tamanho original.
const float ARQUIBANCADA_ESCALA = 1.0f;

// ============================================================
// NUVENS -- ceu decorativo no fundo do estadio
// ============================================================
// 3 nuvens posicionadas nas areas marcadas (esquerda, centro, direita),
// bem acima e atras da arquibancada. Sem fisica nem colisao.
// O modelo tem BB ~6x2.4x5.7 unidades. Escala 3.5 -> ~21x8x20 unidades,
// suficiente para cobrir bem as regioes do ceu visiveis pela camera.
//
// Camera: gluLookAt(0,10,20, 0,0,0) => ceu visivel em Y alto, Z negativo.
// Arquibancada fica em Z ~+10. Nuvens devem ficar em Z negativo (fundo)
// e Y alto para aparecerem acima da arquibancada na projecao perspectiva.
const float NUVEM_Y      = 6.4f;   // altura no ceu
const float NUVEM_Z      = -18.0f;  // fundo, atras da arquibancada
const float NUVEM_ESCALA =  0.8f;   // tamanho das nuvens
// Centro geometrico do modelo (em coords de modelo, antes da escala)
// usado para centralizar o pivot de cada nuvem em sua posicao de mundo.
const float NUVEM_CENTRO_X =  1.964f;
const float NUVEM_CENTRO_Y =  0.002f;
const float NUVEM_CENTRO_Z = -0.611f;

//=====================================
// MODELO 3D (ASSIMP)
//=====================================

// Um grupo de triangulos que compartilham a mesma cor (vinda do material
// do Blender). Os vertices/normais ja saem da importacao em coordenadas
// "de mundo do modelo" (com a hierarquia de nodes e as transformacoes do
// Blender ja aplicadas, gracas a aiProcess_PreTransformVertices), entao
// na hora de desenhar nao precisamos lidar com matrizes de node nenhuma --
// so usar os vertices direto.
struct GrupoModelo3D
{
    std::vector<aiVector3D> vertices;
    std::vector<aiVector3D> normais;

    // Cor "de fallback" (do material), usada quando NAO ha textura nem
    // cor por vertice (ou se a textura falhar ao carregar)
    float cor[3];

    // Cor por vertice (vertex paint do Blender), se o mesh tiver.
    std::vector<aiVector3D> coresPorVertice;

    // Textura (Base Color / Diffuse) do material, se houver. 0 = sem
    // textura (usa coresPorVertice ou "cor" acima).
    GLuint textura = 0;

    // Coordenadas UV por vertice (so' tem sentido se "textura" != 0).
    // Reaproveitamos aiVector3D so' pra guardar (u, v, 0).
    std::vector<aiVector3D> uv;
};

// Carrega um arquivo .glb (ou qualquer formato suportado pelo Assimp) e
// guarda a geometria pronta numa display list do OpenGL, pra desenhar com
// uma unica chamada (glCallList). Pensado pra cenario estatico (campo,
// trave) -- nao lida com animacao, esqueleto, etc.
class Modelo3D
{
public:
    // Retorna true se conseguiu carregar. Em caso de falha, imprime o
    // motivo no console e o modelo simplesmente nao desenha nada (nao
    // existe mais nenhuma geometria antiga pra desenhar no lugar).
    bool carregar(const std::string &caminho)
    {
        Assimp::Importer importer;

        const aiScene* cena = importer.ReadFile(
            caminho,
            aiProcess_Triangulate          |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenSmoothNormals      |
            aiProcess_PreTransformVertices  | // achata a hierarquia de nodes
            aiProcess_ValidateDataStructure
        );

        if(!cena || cena->mNumMeshes == 0)
        {
            fprintf(stderr, "[Modelo3D] Nao foi possivel carregar \"%s\": %s\n",
                    caminho.c_str(), importer.GetErrorString());
            return false;
        }

        grupos.clear();

        for(unsigned int m = 0; m < cena->mNumMeshes; m++)
            processarMesh(cena, cena->mMeshes[m]);

        construirListaDeExibicao();
        return listaDeExibicao != 0;
    }

    void desenhar() const
    {
        if(listaDeExibicao != 0)
            glCallList(listaDeExibicao);
    }

    ~Modelo3D()
    {
        if(listaDeExibicao != 0)
            glDeleteLists(listaDeExibicao, 1);

        for(GLuint tex : texturasCarregadas)
            glDeleteTextures(1, &tex);
    }

private:
    std::vector<GrupoModelo3D> grupos;
    std::vector<GLuint> texturasCarregadas;
    GLuint listaDeExibicao = 0;

    // Carrega a textura Base Color (PBR) ou Diffuse (legado) de um
    // material, se houver. Retorna 0 se o material nao tiver textura (caso
    // em que o grupo cai pra cor solida / vertex color).
    GLuint carregarTexturaDoMaterial(const aiScene* cena, const aiMaterial* mat)
    {
        aiString caminho;

        bool temCaminho =
            (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &caminho) == AI_SUCCESS) ||
            (mat->GetTexture(aiTextureType_DIFFUSE, 0, &caminho) == AI_SUCCESS);

        if(!temCaminho)
            return 0;

        // Texturas embutidas no proprio .glb tem caminho no formato "*N",
        // onde N e o indice em cena->mTextures. Textura externa (arquivo
        // separado) nao e suportada por agora -- .glb normalmente embute
        // tudo, entao isso deve ser raro.
        const aiTexture* texturaEmbutida = nullptr;

        if(caminho.length > 0 && caminho.C_Str()[0] == '*')
        {
            int indice = atoi(caminho.C_Str() + 1);
            if(indice >= 0 && (unsigned int)indice < cena->mNumTextures)
                texturaEmbutida = cena->mTextures[indice];
        }

        if(!texturaEmbutida)
        {
            fprintf(stderr, "[Modelo3D] Textura externa nao suportada: %s\n", caminho.C_Str());
            return 0;
        }

        int largura = 0, altura = 0, canais = 0;
        unsigned char* pixels = nullptr;
        bool decodificadoPeloStb = false;
        std::vector<unsigned char> bufferConvertido;

        if(texturaEmbutida->mHeight == 0)
        {
            // Dados comprimidos (PNG/JPG) -- decodifica com stb_image
            pixels = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(texturaEmbutida->pcData),
                (int)texturaEmbutida->mWidth,
                &largura, &altura, &canais, 4
            );
            decodificadoPeloStb = (pixels != nullptr);
        }
        else
        {
            // Dados brutos (ja descomprimidos, formato aiTexel = BGRA) --
            // bem raro em .glb, mas tratamos por seguranca
            largura = (int)texturaEmbutida->mWidth;
            altura  = (int)texturaEmbutida->mHeight;

            bufferConvertido.resize((size_t)largura * (size_t)altura * 4);

            for(int i = 0; i < largura * altura; i++)
            {
                const aiTexel &t = texturaEmbutida->pcData[i];
                bufferConvertido[i*4+0] = t.r;
                bufferConvertido[i*4+1] = t.g;
                bufferConvertido[i*4+2] = t.b;
                bufferConvertido[i*4+3] = t.a;
            }

            pixels = bufferConvertido.data();
        }

        if(!pixels)
        {
            fprintf(stderr, "[Modelo3D] Falha ao decodificar textura \"%s\": %s\n",
                    caminho.C_Str(), stbi_failure_reason());
            return 0;
        }

        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, largura, altura, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        if(decodificadoPeloStb)
            stbi_image_free(pixels);

        texturasCarregadas.push_back(id);
        return id;
    }

    void processarMesh(const aiScene* cena, const aiMesh* mesh)
    {
        GrupoModelo3D grupo;

        // Cor padrao (cinza neutro), usada so se nao houver textura, cor
        // de material nem cor por vertice
        grupo.cor[0] = 0.8f;
        grupo.cor[1] = 0.8f;
        grupo.cor[2] = 0.8f;

        bool corDoMaterialEncontrada = false;

        if(mesh->mMaterialIndex < cena->mNumMaterials)
        {
            const aiMaterial* mat = cena->mMaterials[mesh->mMaterialIndex];

            // 1) Textura Base Color/Diffuse -- e o caso real desses
            //    modelos (gerados pelo Meshy AI), tem prioridade sobre
            //    qualquer cor solida
            grupo.textura = carregarTexturaDoMaterial(cena, mat);

            // Guardamos a cor solida tambem, mesmo com textura -- serve de
            // fallback automatico se a textura nao carregar por algum motivo
            aiColor4D corBase(0.8f, 0.8f, 0.8f, 1.0f);
            if(mat->Get(AI_MATKEY_BASE_COLOR, corBase) == AI_SUCCESS)
            {
                grupo.cor[0] = corBase.r;
                grupo.cor[1] = corBase.g;
                grupo.cor[2] = corBase.b;
                corDoMaterialEncontrada = true;
            }

            if(!corDoMaterialEncontrada)
            {
                aiColor3D corDiffuse(0.8f, 0.8f, 0.8f);
                if(mat->Get(AI_MATKEY_COLOR_DIFFUSE, corDiffuse) == AI_SUCCESS)
                {
                    grupo.cor[0] = corDiffuse.r;
                    grupo.cor[1] = corDiffuse.g;
                    grupo.cor[2] = corDiffuse.b;
                }
            }
        }

        bool temCorPorVertice = mesh->HasVertexColors(0);
        bool temUV = mesh->HasTextureCoords(0) && grupo.textura != 0;

        // aiProcess_Triangulate garante que toda face aqui e um triangulo
        for(unsigned int f = 0; f < mesh->mNumFaces; f++)
        {
            const aiFace& face = mesh->mFaces[f];

            for(unsigned int v = 0; v < face.mNumIndices; v++)
            {
                unsigned int idx = face.mIndices[v];

                aiVector3D pos = mesh->mVertices[idx];
                aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[idx] : aiVector3D(0.0f,1.0f,0.0f);

                grupo.vertices.push_back(pos);
                grupo.normais.push_back(normal);

                if(temCorPorVertice)
                {
                    const aiColor4D& c = mesh->mColors[0][idx];
                    grupo.coresPorVertice.push_back(aiVector3D(c.r, c.g, c.b));
                }

                if(temUV)
                {
                    const aiVector3D& uvOriginal = mesh->mTextureCoords[0][idx];
                    grupo.uv.push_back(aiVector3D(uvOriginal.x, uvOriginal.y, 0.0f));
                }
            }
        }

        if(!grupo.vertices.empty())
            grupos.push_back(grupo);
    }

    void construirListaDeExibicao()
    {
        listaDeExibicao = glGenLists(1);

        glNewList(listaDeExibicao, GL_COMPILE);

        for(const GrupoModelo3D &grupo : grupos)
        {
            bool temTextura = (grupo.textura != 0) && (grupo.uv.size() == grupo.vertices.size());
            bool temCorPorVertice = !grupo.coresPorVertice.empty();

            if(temTextura)
            {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, grupo.textura);
                // Cor branca: deixa a textura aparecer com as cores dela
                // mesma, so modulada pela luz (nao multiplicada por outra cor)
                glColor3f(1.0f, 1.0f, 1.0f);
            }
            else
            {
                glDisable(GL_TEXTURE_2D);

                // Se nao tem cor por vertice, a cor e a mesma pro grupo
                // inteiro, entao so precisamos definir uma vez antes do glBegin
                if(!temCorPorVertice)
                    glColor3f(grupo.cor[0], grupo.cor[1], grupo.cor[2]);
            }

            glBegin(GL_TRIANGLES);
            for(size_t i = 0; i < grupo.vertices.size(); i++)
            {
                if(temCorPorVertice)
                {
                    const aiVector3D &c = grupo.coresPorVertice[i];
                    glColor3f(c.x, c.y, c.z);
                }

                if(temTextura)
                    glTexCoord2f(grupo.uv[i].x, grupo.uv[i].y);

                glNormal3f(grupo.normais[i].x, grupo.normais[i].y, grupo.normais[i].z);
                glVertex3f(grupo.vertices[i].x, grupo.vertices[i].y, grupo.vertices[i].z);
            }
            glEnd();
        }

        // Garante que a textura nao "vaza" pro resto da cena (personagens,
        // bola, texto), que nao usam coordenadas de textura
        glDisable(GL_TEXTURE_2D);

        glEndList();

        // Os dados de vertice ja estao na display list (na GPU); as
        // texturas ja estao em texturasCarregadas (pra limpeza no
        // destrutor) -- nao precisamos mais guardar nada disso na CPU
        grupos.clear();
    }
};

// Instancias globais dos modelos. Sao carregadas uma unica vez, em
// init(), e reutilizadas em todo frame.
Modelo3D modeloCampo;
Modelo3D modeloTrave;
Modelo3D modeloArquibancada;
Modelo3D modeloRefletor;
Modelo3D modeloBola;
Modelo3D modeloNuvem;

// ============================================================
// MODELO ANIMADO (jogador FBX com animacao de chute)
// ============================================================
//
// ESTRUTURA DO FBX (jogador2.fbx):
//   Node Object_3     -> meshes 0 (head, mat=skin) e 1 (hair, mat=hair)
//   Node Object_3.001 -> mesh 2 (body/clothes, mat=Scene-Root, branco)
//
// ANIMACOES:
//   Anim 0: Object_3    com 'Object_3.001Action' (chute, 12 ticks @ 24tps)
//   Anim 2: Object_3.001 com 'Object_3.001Action' (chute, 12 ticks @ 24tps)
//   -> Para o chute completo, animamos Object_3 com anim[0] E
//      Object_3.001 com anim[2] simultaneamente.
//
// CORES:
//   Mat[0] head: DIFFUSE (0.41, 0.20, 0.11) -- tom de pele marrom
//   Mat[1] hair: DIFFUSE (0.02, 0.01, 0.01) -- cabelo preto
//   Mat[2] body: DIFFUSE (1.0,  1.0,  1.0)  -- roupa branca (e a cor real)
//   Sem texturas embutidas. Usamos glMaterialfv para garantir que a
//   cor do material seja respeitada mesmo com lighting ativo.
//
// BOUNDING BOX GLOBAL (pose de repouso, com todas as transformacoes de no):
//   X[-65.17..11.76]  Y[-3.93..176.93]  Z[-133.11..9.99]
//   Centro XZ: (-26.71, -61.56)   Altura Y: 180.87 unidades Blender
//   -> Escala = 2.0 / 180.87 = ~0.01106
//   -> offsetX = -(-26.71)*escala  para centralizar em X
//   -> offsetZ = -(-61.56)*escala  para centralizar em Z
//   -> offsetY = -(-3.93)*escala   para base ficar em y=0
//
// POSICIONAMENTO:
//   Desenhamos com glTranslatef(jogadorX, jogadorY - raioCabeca, 0)
//   e dentro da classe aplicamos:
//     glTranslatef(offsetX, offsetY, offsetZ)  <- centraliza
//     glScalef(escala, escala, escala)
//   Assim o modelo fica centrado em X/Z com a base no chao.

const char* CAMINHO_MODELO_JOGADOR = "assets/models/jogador2.fbx";
const char* CAMINHO_MODELO_CPU     = "assets/models/jogadorIA.fbx";

// Indice das animacoes de chute no FBX
// Object_3     animado por anim[0]: 'Object_3|Object_3.001Action'
// Object_3.001 animado por anim[2]: 'Object_3.001|Object_3.001Action'
const unsigned int ANIM_CHUTE_CABECA = 0; // anima Object_3 (cabeca/cabelo)
const unsigned int ANIM_CHUTE_CORPO  = 2; // anima Object_3.001 (corpo/roupa)

// Velocidade da animacao (multiplo de tempo real)
const float ANIM_VELOCIDADE = 1.5f;

class ModeloAnimado {
public:
    // Offsets de centralizacao (calculados em calcularEscala, usados no desenho)
    float escala   = 1.0f;
    float offsetX  = 0.0f; // translacao para centralizar em X
    float offsetY  = 0.0f; // translacao para base do modelo em y=0
    float offsetZ  = 0.0f; // translacao para centralizar em Z

    bool carregar(const std::string& caminho) {
        const aiScene* cena = importer.ReadFile(
            caminho,
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_GenSmoothNormals |
            aiProcess_ValidateDataStructure
        );
        if (!cena || cena->mNumMeshes == 0) {
            fprintf(stderr, "[ModeloAnimado] Falha ao carregar '%s': %s\n",
                    caminho.c_str(), importer.GetErrorString());
            return false;
        }
        cenaGlobal = cena;

        // Pre-carrega cores dos materiais uma unica vez
        carregarCoresMateriais();

        // Calcula escala e offsets de centralizacao
        calcularEscala();

        printf("[ModeloAnimado] Carregado: %u meshes, %u anims\n",
               cena->mNumMeshes, cena->mNumAnimations);
        printf("[ModeloAnimado] escala=%.5f offsetX=%.3f offsetY=%.3f offsetZ=%.3f\n",
               escala, offsetX, offsetY, offsetZ);
        return true;
    }

    // Desenha na pose de repouso
    void desenharRepouso() const {
        if (!cenaGlobal) return;
        aplicarOffsetEscala();
        desenharNo(cenaGlobal->mRootNode, -1.0, nullptr, nullptr);
        glPopMatrix();
    }

    // Desenha o chute: anima Object_3 com anim[animCabeca] e
    // Object_3.001 com anim[animCorpo] no mesmo tempo em ticks.
    void desenharChute(float tempoTicks) const {
        if (!cenaGlobal) return;
        const aiAnimation* animCabeca =
            (ANIM_CHUTE_CABECA < cenaGlobal->mNumAnimations)
            ? cenaGlobal->mAnimations[ANIM_CHUTE_CABECA] : nullptr;
        const aiAnimation* animCorpo  =
            (ANIM_CHUTE_CORPO  < cenaGlobal->mNumAnimations)
            ? cenaGlobal->mAnimations[ANIM_CHUTE_CORPO]  : nullptr;
        aplicarOffsetEscala();
        desenharNo(cenaGlobal->mRootNode, (double)tempoTicks, animCabeca, animCorpo);
        glPopMatrix();
    }

    float getDuracaoChute() const {
        if (!cenaGlobal || ANIM_CHUTE_CABECA >= cenaGlobal->mNumAnimations) return 0.5f;
        const aiAnimation* a = cenaGlobal->mAnimations[ANIM_CHUTE_CABECA];
        float tps = (a->mTicksPerSecond > 0) ? (float)a->mTicksPerSecond : 24.0f;
        return (float)a->mDuration / tps;
    }

    float getTicksPerSecondChute() const {
        if (!cenaGlobal || ANIM_CHUTE_CABECA >= cenaGlobal->mNumAnimations) return 24.0f;
        const aiAnimation* a = cenaGlobal->mAnimations[ANIM_CHUTE_CABECA];
        return (a->mTicksPerSecond > 0) ? (float)a->mTicksPerSecond : 24.0f;
    }

private:
    Assimp::Importer importer;
    const aiScene*   cenaGlobal = nullptr;

    // Cores pre-carregadas por indice de material
    struct CorMaterial { float r, g, b; };
    std::vector<CorMaterial> coresMateriais;

    // ---- Pre-carrega cores dos materiais ----
    void carregarCoresMateriais() {
        if (!cenaGlobal) return;
        coresMateriais.resize(cenaGlobal->mNumMaterials);
        for (unsigned i = 0; i < cenaGlobal->mNumMaterials; i++) {
            const aiMaterial* mat = cenaGlobal->mMaterials[i];
            // Tenta BASE_COLOR (PBR) primeiro, depois DIFFUSE (legado)
            aiColor4D c4(0.8f, 0.8f, 0.8f, 1.0f);
            aiColor3D c3(0.8f, 0.8f, 0.8f);
            if (mat->Get(AI_MATKEY_BASE_COLOR, c4) == AI_SUCCESS) {
                coresMateriais[i] = {c4.r, c4.g, c4.b};
            } else if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, c3) == AI_SUCCESS) {
                coresMateriais[i] = {c3.r, c3.g, c3.b};
            } else {
                coresMateriais[i] = {0.8f, 0.8f, 0.8f};
            }
            printf("[ModeloAnimado] Mat[%u]: (%.2f, %.2f, %.2f)\n",
                   i, coresMateriais[i].r, coresMateriais[i].g, coresMateriais[i].b);
        }
    }

    // ---- Aplica pushMatrix + offset de centralizacao + escala ----
    void aplicarOffsetEscala() const {
        glPushMatrix();
        // Primeiro translada para centralizar o modelo em X/Z
        // e colocar a base em y=0
        glTranslatef(offsetX, offsetY, offsetZ);
        glScalef(escala, escala, escala);
    }

    // ---- Busca canal de animacao por nome de no ----
    aiNodeAnim* buscarCanal(const aiAnimation* anim, const std::string& nome) const {
        if (!anim) return nullptr;
        for (unsigned i = 0; i < anim->mNumChannels; i++)
            if (anim->mChannels[i]->mNodeName.C_Str() == nome)
                return anim->mChannels[i];
        return nullptr;
    }

    aiVector3D interpolarPos(const aiNodeAnim* ch, double t) const {
        if (ch->mNumPositionKeys == 1) return ch->mPositionKeys[0].mValue;
        for (unsigned i = 0; i + 1 < ch->mNumPositionKeys; i++) {
            if (t < ch->mPositionKeys[i+1].mTime) {
                double dt = ch->mPositionKeys[i+1].mTime - ch->mPositionKeys[i].mTime;
                float f = (dt > 0) ? (float)((t - ch->mPositionKeys[i].mTime) / dt) : 0.0f;
                const aiVector3D& a = ch->mPositionKeys[i].mValue;
                const aiVector3D& b = ch->mPositionKeys[i+1].mValue;
                return a + (b - a) * f;
            }
        }
        return ch->mPositionKeys[ch->mNumPositionKeys-1].mValue;
    }

    aiQuaternion interpolarRot(const aiNodeAnim* ch, double t) const {
        if (ch->mNumRotationKeys == 1) return ch->mRotationKeys[0].mValue;
        for (unsigned i = 0; i + 1 < ch->mNumRotationKeys; i++) {
            if (t < ch->mRotationKeys[i+1].mTime) {
                double dt = ch->mRotationKeys[i+1].mTime - ch->mRotationKeys[i].mTime;
                float f = (dt > 0) ? (float)((t - ch->mRotationKeys[i].mTime) / dt) : 0.0f;
                aiQuaternion out;
                aiQuaternion::Interpolate(out, ch->mRotationKeys[i].mValue,
                                               ch->mRotationKeys[i+1].mValue, f);
                return out.Normalize();
            }
        }
        return ch->mRotationKeys[ch->mNumRotationKeys-1].mValue;
    }

    aiVector3D interpolarEscala(const aiNodeAnim* ch, double t) const {
        if (ch->mNumScalingKeys == 1) return ch->mScalingKeys[0].mValue;
        for (unsigned i = 0; i + 1 < ch->mNumScalingKeys; i++) {
            if (t < ch->mScalingKeys[i+1].mTime) {
                double dt = ch->mScalingKeys[i+1].mTime - ch->mScalingKeys[i].mTime;
                float f = (dt > 0) ? (float)((t - ch->mScalingKeys[i].mTime) / dt) : 0.0f;
                const aiVector3D& a = ch->mScalingKeys[i].mValue;
                const aiVector3D& b = ch->mScalingKeys[i+1].mValue;
                return a + (b - a) * f;
            }
        }
        return ch->mScalingKeys[ch->mNumScalingKeys-1].mValue;
    }

    aiMatrix4x4 transformacaoDoNo(const aiNode* node, double t, const aiAnimation* anim) const {
        aiNodeAnim* ch = buscarCanal(anim, node->mName.C_Str());
        if (!ch) return node->mTransformation;

        aiVector3D   pos = interpolarPos(ch, t);
        aiQuaternion rot = interpolarRot(ch, t);
        aiVector3D   sc  = interpolarEscala(ch, t);

        aiMatrix4x4 mPos, mRot, mSc;
        aiMatrix4x4::Translation(pos, mPos);
        mRot = aiMatrix4x4(rot.GetMatrix());
        aiMatrix4x4::Scaling(sc, mSc);
        return mPos * mRot * mSc;
    }

    // Desenha recursivamente.
    // animCabeca: animacao para nos que NAO sao Object_3.001 (cabeca/cabelo)
    // animCorpo:  animacao para Object_3.001 (corpo/roupa)
    // t < 0 => pose de repouso (sem animacao)
    void desenharNo(const aiNode* node, double t,
                    const aiAnimation* animCabeca,
                    const aiAnimation* animCorpo) const
    {
        // Escolhe qual animacao usar para ESTE no
        const aiAnimation* animParaEsteNo = nullptr;
        if (t >= 0.0) {
            std::string nome = node->mName.C_Str();
            if (nome == "Object_3.001")
                animParaEsteNo = animCorpo;
            else
                animParaEsteNo = animCabeca;
        }

        aiMatrix4x4 local = transformacaoDoNo(node, t, animParaEsteNo);

        // Assimp usa row-major; OpenGL usa column-major. Transpomos:
        float m[16];
        m[0]=local.a1; m[1]=local.b1; m[2]=local.c1; m[3]=local.d1;
        m[4]=local.a2; m[5]=local.b2; m[6]=local.c2; m[7]=local.d2;
        m[8]=local.a3; m[9]=local.b3; m[10]=local.c3; m[11]=local.d3;
        m[12]=local.a4; m[13]=local.b4; m[14]=local.c4; m[15]=local.d4;

        glPushMatrix();
        glMultMatrixf(m);

        for (unsigned mi = 0; mi < node->mNumMeshes; mi++)
            desenharMesh(cenaGlobal->mMeshes[node->mMeshes[mi]]);

        for (unsigned i = 0; i < node->mNumChildren; i++)
            desenharNo(node->mChildren[i], t, animCabeca, animCorpo);

        glPopMatrix();
    }

    void desenharMesh(const aiMesh* mesh) const {
        // Usa o MESMO padrao do Modelo3D (campo/trave): apenas glColor3f.
        // O init() habilita GL_COLOR_MATERIAL com GL_AMBIENT_AND_DIFFUSE,
        // entao glColor3f alimenta automaticamente diffuse e ambient do
        // material OpenGL. Chamar glMaterialfv QUEBRA esse mecanismo pois
        // o valor fixo sobrescreve o tracking de glColor3f, causando as
        // cores erradas e flashes pretos.
        float r = 0.8f, g = 0.8f, b = 0.8f;
        if (mesh->mMaterialIndex < (unsigned)coresMateriais.size()) {
            r = coresMateriais[mesh->mMaterialIndex].r;
            g = coresMateriais[mesh->mMaterialIndex].g;
            b = coresMateriais[mesh->mMaterialIndex].b;
        }

        // glColor3f -> GL_COLOR_MATERIAL -> alimenta GL_DIFFUSE + GL_AMBIENT
        glColor3f(r, g, b);

        // O FBX tem Sketchfab_model.001 com escala Y negativa, o que inverte
        // as normais de alguns meshes. Desabilitar culling garante que ambas
        // as faces sejam renderizadas (GL_LIGHT_MODEL_TWO_SIDE cuida da
        // iluminacao correta). Sera reabilitado apos o desenho do jogador.
        glDisable(GL_CULL_FACE);

        glBegin(GL_TRIANGLES);
        for (unsigned f = 0; f < mesh->mNumFaces; f++) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned v = 0; v < face.mNumIndices; v++) {
                unsigned idx = face.mIndices[v];
                if (mesh->HasNormals())
                    glNormal3f(mesh->mNormals[idx].x,
                               mesh->mNormals[idx].y,
                               mesh->mNormals[idx].z);
                glVertex3f(mesh->mVertices[idx].x,
                           mesh->mVertices[idx].y,
                           mesh->mVertices[idx].z);
            }
        }
        glEnd();
    }

    // Calcula escala e offsets para centralizar o modelo e
    // colocar sua base exatamente em y=0.
    void calcularEscala() {
        if (!cenaGlobal) return;

        float xmin=1e9f, xmax=-1e9f;
        float ymin=1e9f, ymax=-1e9f;
        float zmin=1e9f, zmax=-1e9f;
        bool tem = false;

        calcularBBNo(cenaGlobal->mRootNode, aiMatrix4x4(),
                     xmin,xmax,ymin,ymax,zmin,zmax,tem);

        if (!tem) { escala = 0.01f; return; }

        float altura = ymax - ymin;
        if (altura < 0.001f) { escala = 0.01f; return; }

        // Altura desejada: 2 * raioCabeca (personagem inteiro)
        float alturaDesejada = 2.0f * raioCabeca;
        escala = alturaDesejada / altura;

        // Offsets em espaco do modelo (ANTES da escala): centralizamos
        // o modelo em XZ e colocamos a base (ymin) em y=0.
        // No desenho fazemos: translate(offsetX, offsetY, offsetZ) * scale(escala)
        // Entao os offsets ja estao em unidades do JOGO (apos a escala).
        // Calculamos como: offset_mundo = -centro_modelo * escala
        float centroX = (xmin + xmax) * 0.5f;
        float centroZ = (zmin + zmax) * 0.5f;
        offsetX = -centroX * escala;
        offsetY = -ymin   * escala; // base do modelo vai para y=0
        offsetZ = -centroZ * escala;

        printf("[ModeloAnimado] BB global: X[%.1f..%.1f] Y[%.1f..%.1f] Z[%.1f..%.1f]\n",
               xmin,xmax,ymin,ymax,zmin,zmax);
        printf("[ModeloAnimado] escala=%.5f offsets=(%.3f, %.3f, %.3f)\n",
               escala, offsetX, offsetY, offsetZ);
    }

    void calcularBBNo(const aiNode* node, aiMatrix4x4 parentGlobal,
                      float& xmin, float& xmax,
                      float& ymin, float& ymax,
                      float& zmin, float& zmax, bool& tem) const {
        aiMatrix4x4 global = parentGlobal * node->mTransformation;
        for (unsigned mi = 0; mi < node->mNumMeshes; mi++) {
            const aiMesh* mesh = cenaGlobal->mMeshes[node->mMeshes[mi]];
            for (unsigned v = 0; v < mesh->mNumVertices; v++) {
                aiVector3D p = global * mesh->mVertices[v];
                if(p.x<xmin)xmin=p.x; if(p.x>xmax)xmax=p.x;
                if(p.y<ymin)ymin=p.y; if(p.y>ymax)ymax=p.y;
                if(p.z<zmin)zmin=p.z; if(p.z>zmax)zmax=p.z;
                tem = true;
            }
        }
        for (unsigned i = 0; i < node->mNumChildren; i++)
            calcularBBNo(node->mChildren[i], global,
                         xmin,xmax,ymin,ymax,zmin,zmax,tem);
    }
};

// Instancia do modelo do jogador humano
ModeloAnimado modeloJogador;
// Instancia do modelo da CPU (jogadorIA.fbx -- mesma estrutura, visual diferente)
ModeloAnimado modeloJogadorIA;

// ---- Estado de animacao ----
// O jogador humano tem animacao de chute na tecla Z
float tempoAnimChuteJogador = -1.0f; // -1 = nao animando
float tempoAnimChuteCPU     = -1.0f;

// Offset Y para alinhar a base do modelo com o chao do jogo.
float jogadorModeloOffsetY = 0.0f;
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
// Retorna true se a bola esta dentro do raio de colisao do personagem em (px,py)
static bool bolaEmContato(float px, float py)
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
// Chamado DEPOIS de ambos os colisaoJogador(). Se a bola ainda estiver em
// contato com AMBOS simultaneamente, os dois push-outs anteriores se
// anularam parcialmente e a bola ficou presa. Detectamos isso aqui e
// expelimos a bola para cima com impulso minimo garantido.
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

//=====================================
// COLISAO JOGADOR x CPU
//=====================================

// Impede que os personagens fiquem um dentro do outro. O ponto principal:
// quando um pula em cima do outro, quem esta no chao funciona como um
// "ancora" (a gravidade ja mantem ele lá, ele nao tem motivo pra se mover)
// -- entao quem esta no ar deve absorver a correcao quase toda, em vez da
// divisao 50/50 de antes. Com 50/50, se um dos dois ja estava encostado no
// chao (e portanto nao "sobrava" deslocamento real pra ele), a metade da
// correcao que devia ir pra ele se perdia, e o personagem em cima ficava
// permanentemente afundado/atravessando.
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

    // Por padrao, divide a correcao igualmente (caso normal: os dois no
    // chao, lado a lado, ou os dois no ar). Quando só um dos dois esta no
    // chao, ele absorve 0% (fica parado) e o outro absorve 100% -- assim a
    // separacao final e SEMPRE exatamente minDist, sem sobra nem falta.
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
    // -- sem isso, no proximo frame a gravidade continuaria empurrando ele
    // pra dentro do outro de novo, e a correcao toda vira teria que
    // acontecer outra vez, causando tremor.
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

//=====================================
// GOL / TRAVE / PAREDE DE FUNDO
//=====================================

// Carrega o WAV inteiro na RAM uma unica vez (chamado no init).
// A partir dai tocarSomGol() toca direto do buffer, sem tocar no disco.
void carregarSomGol()
{
    FILE* f = fopen(CAMINHO_SOM_GOL, "rb");
    if(!f)
    {
        fprintf(stderr, "[Audio] Nao foi possivel abrir %s\n", CAMINHO_SOM_GOL);
        return;
    }
    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    rewind(f);
    bufferSomGol.resize((size_t)tamanho);
    fread(bufferSomGol.data(), 1, (size_t)tamanho, f);
    fclose(f);
}

// Toca o som de gol em uma thread separada, eliminando qualquer bloqueio
// que o PlaySound possa causar na thread principal do jogo.
void tocarSomGol()
{
    if(bufferSomGol.empty()) return;
    std::thread([](){
        PlaySound(bufferSomGol.data(), NULL, SND_MEMORY | SND_SYNC | SND_NODEFAULT);
    }).detach();
}

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
            tocarSomGol();
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
// IA: PREVISAO DE TRAJETORIA
//=====================================

// Simula a trajetoria da bola pra frente, em uma copia local (sem alterar
// a bola de verdade), pra IA conseguir reagir a pra ONDE ela esta indo,
// nao so a onde ela esta agora. Usa a mesma fisica simplificada do chao
// (gravidade + quique) -- ignora gol/trave/parede de proposito, porque
// pra decisao de posicionamento isso ja e precisao mais do que suficiente
// e mantem a previsao simples e rapida.
struct PrevisaoBola
{
    float x;
    float y;
};

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

//=====================================
// UPDATE
//=====================================

// Guarda o instante (em ms, desde o inicio do programa) do ultimo update(),
// pra calcular quanto tempo realmente passou entre um frame e outro.
int tempoAnterior = 0;

void update(int)
{
    // ---- Controle de tempo (fisica independente de frame rate) ----
    // Antes, cada incremento de posicao/velocidade assumia que update()
    // sempre era chamado a cada 16ms exatos. Quando o jogo roda mais devagar
    // (por exemplo, por desenhar os modelos 3D novos), o timer ainda tenta
    // chamar update() a cada 16ms, mas como o loop do GLUT fica ocupado
    // desenhando, as chamadas efetivamente acontecem com menos frequencia --
    // e como cada chamada so andava "um pouco", o jogo todo parecia mais
    // lento (personagens, bola, tudo). Agora medimos o tempo real passado e
    // escalamos os movimentos por ele (dt = 1.0 representa os 16ms
    // originais), entao a velocidade do jogo fica a mesma independente do
    // desempenho da maquina.
    int tempoAtual = glutGet(GLUT_ELAPSED_TIME);
    float dt = (tempoAnterior == 0) ? 1.0f : (tempoAtual - tempoAnterior) / 16.0f;
    tempoAnterior = tempoAtual;

    // Limita o dt pra nao dar "saltos" grandes se o jogo gaguejar por um
    // instante (ex: ao carregar os modelos, ou uma travada pontual)
    if(dt > 4.0f) dt = 4.0f;
    if(dt < 0.0f) dt = 0.0f;

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
            // Campo aberto: restituicao mais alta (0.72 -> 0.85) para
            // quiques mais vivos; atrito horizontal reduzido (0.88) para
            // a bola nao parar bruscamente no chao.
            velBolaY *= -0.85f;
            velBolaX *= powf(0.88f, dt);

            // Anti-morte: se a bola quase parou verticalmente mas ainda
            // tem velocidade horizontal relevante, garante um salto minimo
            // para nao ficar rastejando eternamente no chao.
            if(velBolaY < 0.04f && fabsf(velBolaX) > 0.03f)
                velBolaY = 0.04f;

            // Se a bola ficou quase completamente parada (sem ninguem por
            // perto para interagir), aplica um micro-impulso para cima para
            // destravar a partida sem parecer artificial.
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

    // Chao logico agora e y=0 (pivo do campo no topo do gramado).
    // Personagens repousam com o centro em y=raioCabeca (= 1.0).
    // Threshold de pulo: raioCabeca + 0.05 (pequena margem de tolerancia)
    if(teclaCima && jogadorY <= raioCabeca + 0.05f)
        velJogadorY = 0.12f;

    // Isso representa a VELOCIDADE de quem ta cabeceando (nao um
    // deslocamento), entao nao escalamos por dt aqui -- so o deslocamento
    // em si (acima) precisa disso.
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
    //   4. Decisao de pulo baseada em quando a bola ficara no alcance,
    //      nao so em onde ela esta agora.
    //   5. Direcao de cabeceio controlada: tenta sempre bater em direcao
    //      ao gol adversario (-X), nunca jogando a bola para o proprio gol.
    //   6. Velocidade de movimento ligeiramente maior que o jogador,
    //      compensada por decisoes mais conservadoras (nao fica oscilando).

    // ---- Previsoes de trajetoria ----
    // Simula a bola para frente em varios horizontes para a CPU
    // tomar decisoes diferentes conforme a urgencia.
    PrevisaoBola prev6  = preverBola(6);   // iminente  : pulo reativo
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
    // Limiar mais cedo (GOL_X - 9) para dar mais tempo de reagir
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
        // Prioridade absoluta, nao pondera com nada mais.
        alvoCpuX = bolaX;
    }
    else if(bolaAmeacandoGol)
    {
        // DEFESA: intercept a trajetoria prevista no curto/medio prazo.
        // Se a bola esta proxima usa previsao curta (mais precisa),
        // se esta longe usa previsao media para chegar antes.
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
        // "Atras" em relacao ao gol adversario = X menor que bolaX.
        // Isso faz a cabeca empurrar a bola para a esquerda (gol adv.).
        alvoCpuX = bolaX + 0.3f; // fica levemente mais perto do proprio gol
                                   // que a bola -> normal da colisao aponta para -X
    }
    else if(bolaNoLadoJogador)
    {
        // POSICIONAMENTO: bola no lado do jogador, sem ameaca imediata.
        // CPU avanca ate uma posicao de pressao levemente agressiva,
        // misturando previsao longa (onde a bola deve chegar) com
        // um ponto de pressao fixo um pouco no lado adversario.
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
        // Antecipa a previsao de medio prazo.
        // Quando a bola esta caindo e proxima, vai buscar direto.
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
    // Zona-morta adaptativa: mais apertada em situacoes de urgencia
    // para a CPU reagir mais rapido sem ficar oscilando no neutro.
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
    // Em vez de apenas checar se a bola esta perto AGORA, calculamos
    // em qual frame (dentro de N passos) a bola ficara no raio de toque
    // e verificamos se um pulo iniciado AGORA chegaria a essa altura
    // nesse mesmo frame. Isso elimina pulos atrasados e pulos desperdicados.

    bool devePular = false;

    if(cpuNoChaoAgora)
    {
        // Simula o pulo da CPU: a partir do chao (cpuY = raioCabeca),
        // com velCpuY = 0.12, quanto tempo leva pra atingir cada altura?
        // A altura da CPU no frame F do pulo e:
        //   hCpu(F) = raioCabeca + 0.12*F + 0.5*gravidade*F*F
        // A bola no frame F tem posicao prev(F).
        // Queremos: dist(bola_F, cpu_F) < raioToque

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
                // Ha uma janela de toque: vale pular agora?
                // So pula se a bola estiver acima do chao (nao adianta
                // pular para bater em bola rasteira) e se a CPU estara
                // realmente em posicao de toque (nao so passando perto).
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
        // (e melhor ficar no chao para interceptar no nivel certo)
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

// Teclado normal (letras/numeros) -- tecla Z dispara animacao de chute
void tecladoNormal(unsigned char key, int, int)
{
    switch(key)
    {
        case 'z':
        case 'Z':
            // So inicia se nao estiver ja animando
            if(tempoAnimChuteJogador < 0.0f)
                tempoAnimChuteJogador = 0.0f;
            break;
        case 27: // ESC
            exit(0);
            break;
    }
}


//=====================================
// CAMPO
//=====================================

// O campo desenhado na mao (glBegin(GL_QUADS)...) foi totalmente removido.
// Agora e so o modelo 3D do Blender (modeloCampo), carregado em init().
// A logica do jogo continua considerando a area jogavel de 30x10 (ver
// LIMITE_CAMPO_X, CAMPO_LARGURA etc.) -- so a aparencia mudou.
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
// camera. X e Y sao livres pra ajuste fino (ver ARQUIBANCADA_OFFSET_X/Y
// acima); Z fica fixo, garantindo que ela sempre fique atras da lateral
// do campo.
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
// O modelo tem a base em Y=0 e sobe ~8 unidades -- ficam como torres
// de iluminacao nos cantos laterais do estadio.
// direcaoX: +1 para refletor direito, -1 para esquerdo.
void desenharRefletor(float direcaoX, float direcaoZ)
{
    glPushMatrix();

    // Posicao: canto da arquibancada
    glTranslatef(direcaoX * REFLETOR_X, REFLETOR_Y, direcaoZ * REFLETOR_Z);

    // Rotacao: refletor esquerdo (direcaoX < 0) gira 180 graus em Y
    // para espelhar corretamente; refletor do lado -Z gira em Y tambem
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
// mao), mantida no codigo como referencia/debug. Fica invisivel por padrao
// -- troque TRAVE_ANTIGA_VISIVEL pra true se quiser compara-la visualmente
// com o modelo novo do Blender. E desenhada considerando que o cubo tem o
// centro de gravidade exatamente no seu meio (ou seja, chamada de dentro
// de desenharGol(), ja posicionada em x=GOL_X, y=1.5, z=0 -- a mesma
// ancora que a fisica usa).
void desenharTraveAntiga(float direcao)
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
//
// 1) A geometria antiga (cubo wireframe + paineis da "rede"), com centro de
//    gravidade exatamente no meio do cubo -- em (x, GOL_META, 0), com
//    x = GOL_X = 14. Fica invisivel por padrao (TRAVE_ANTIGA_VISIVEL).
//
// 2) O modelo novo do Blender (modeloTrave), desenhado no MESMO ponto de
//    ancoragem do cubo logico (sem offset adicional). O cubo logico funciona
//    como "esqueleto" invisivel; o modelo visual e sua representacao grafica
//    exata, sobreposta diretamente.
//
// Nenhuma constante de fisica/colisao (GOL_X, GOL_META, GOL_FRENTE,
// GOL_FUNDO, GOL_TRAVE_Y) foi tocada -- so a renderizacao.
void desenharGol(float x, float direcao)
{
    glPushMatrix();

    // Ancora: centro geometrico do cubo logico do gol.
    // Com o pivo do campo em y=0 (topo do gramado), a base do cubo fica
    // em y=0 e o centro em y=GOL_META (= 1.5).
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

// Parede no limite do campo, atras dos gols. A FISICA continua exatamente
// igual (ver tratarParedeFundo, que nao foi alterada) -- so a parte visual
// foi desligada (PAREDE_FUNDO_VISIVEL = false), porque agora o modelo 3D da
// trave ja fecha visualmente o fundo do gol. Pra debugar, basta trocar
// PAREDE_FUNDO_VISIVEL pra true e a parede (com o desenho antigo, em
// paineis cinza) volta a aparecer.
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
// DISPLAY
//=====================================

void desenharNuvens()
{
    // Posicoes X das 3 nuvens (esquerda, centro, direita),
    // correspondendo as 3 areas marcadas no screenshot.
    // O modelo e centralizado em seu pivot antes de ser transladado
    // para a posicao de mundo, garantindo que cada nuvem fique
    // exatamente onde indicado.
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
    desenharNuvens();

    // Usa GOL_X (= 14) -- antes estava hardcoded como 13, desalinhando a trave esquerda
    desenharGol(-GOL_X, -1.0f);
    desenharGol( GOL_X,  1.0f);

    desenharParedeFundo(-PAREDE_FUNDO_X);
    desenharParedeFundo(PAREDE_FUNDO_X);

    // ---- Jogador (modelo FBX, lado esquerdo, ataca para +X / direita) ----
    // jogadorY e o centro logico do personagem (mesma posicao que a esfera usava).
    // O modelo ja cuida de colocar sua base em y=0 internamente (via offsetY),
    // entao so precisamos transladar para (jogadorX, jogadorY - raioCabeca, 0):
    // assim a base do modelo fica em jogadorY - raioCabeca (= chao quando parado).
    //
    // Rotacao: o FBX foi exportado com a frente do personagem em uma direcao
    // arbitraria. Ajustamos com glRotatef para ele olhar para +X (direita).
    // Se parecer virado ao contrario, troque -90 por 90.
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
    // A translacao de centralizacao e aplicada no espaco do modelo (antes da escala).
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

    // GL_COLOR_MATERIAL faz com que glColor3f alimente o material difuso E
    // ambiente simultaneamente. Sem glColorMaterial explicito, o default e
    // GL_FRONT_AND_BACK / GL_AMBIENT_AND_DIFFUSE -- mas e mais seguro
    // declarar explicitamente pra garantir que funcione com todos os drivers.
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    // GL_NORMALIZE: reescala normais apos glScalef, necessario para
    // iluminacao correta no modelo do jogador (escala nao-uniforme do FBX).
    glEnable(GL_NORMALIZE);

    // GL_LIGHT_MODEL_TWO_SIDE: ilumina ambas as faces dos triangulos.
    // Necessario porque o FBX tem normais invertidas em alguns meshes
    // (escala Y negativa no node Sketchfab_model.001).
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    // Luz ambiente global: sem isso, faces voltadas pra longe da luz ficam
    // completamente pretas, escondendo a cor dos modelos do Blender.
    // 0.4 e um valor moderado -- suficiente pra revelar a cor em todas as
    // faces sem "lavar" a iluminacao direcional da GL_LIGHT0.
    GLfloat luzAmbienteGlobal[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, luzAmbienteGlobal);

    // Componente difusa da GL_LIGHT0 (a luz direcional principal)
    GLfloat difusa[]  = { 0.9f, 0.9f, 0.9f, 1.0f };
    GLfloat especular[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  difusa);
    glLightfv(GL_LIGHT0, GL_SPECULAR, especular);

    glClearColor(
        0.4f,
        0.7f,
        1.0f,
        1.0f
    );

    srand((unsigned int)time(nullptr));

    // Pre-carrega o WAV do gol na RAM para eliminar delay na primeira reproducao
    carregarSomGol();

    resetarBola();
    resetarPosicoes();

    // Carrega os modelos 3D (campo e trave). Se algum nao for encontrado,
    // um aviso e impresso no console e o jogo continua rodando -- só que
    // sem desenhar nada no lugar daquele elemento (a fisica/colisao nao
    // dependem do modelo visual, entao o jogo continua jogavel mesmo
    // assim).
    if(!modeloCampo.carregar(CAMINHO_MODELO_CAMPO))
        fprintf(stderr, "[Aviso] Campo 3D nao carregado -- nada sera desenhado no lugar do campo.\n");

    if(!modeloTrave.carregar(CAMINHO_MODELO_TRAVE))
        fprintf(stderr, "[Aviso] Trave 3D nao carregada -- nada sera desenhado no lugar do gol.\n");

    if(!modeloArquibancada.carregar(CAMINHO_MODELO_ARQUIBANCADA))
        fprintf(stderr, "[Aviso] Arquibancada 3D nao carregada -- nada sera desenhado atras do campo.\n");

    if(!modeloRefletor.carregar(CAMINHO_MODELO_REFLETOR))
        fprintf(stderr, "[Aviso] Refletor 3D nao carregado.\n");

    if(!modeloBola.carregar(CAMINHO_MODELO_BOLA))
        fprintf(stderr, "[Aviso] Modelo da bola nao carregado.\n");

    if(!modeloNuvem.carregar(CAMINHO_MODELO_NUVEM))
        fprintf(stderr, "[Aviso] Modelo da nuvem nao carregado.\n");

    if(!modeloJogador.carregar(CAMINHO_MODELO_JOGADOR))
        fprintf(stderr, "[Aviso] Modelo do jogador nao carregado.\n");

    // Carrega o modelo da CPU e forca os mesmos valores de escala e offset
    // do jogador humano -- ambos os FBX tem a mesma estrutura e proporcoes,
    // entao devem ocupar exatamente o mesmo espaco fisico na cena.
    if(!modeloJogadorIA.carregar(CAMINHO_MODELO_CPU))
        fprintf(stderr, "[Aviso] Modelo da CPU nao carregado.\n");
    modeloJogadorIA.escala  = modeloJogador.escala;
    modeloJogadorIA.offsetX = modeloJogador.offsetX;
    modeloJogadorIA.offsetY = modeloJogador.offsetY;
    modeloJogadorIA.offsetZ = modeloJogador.offsetZ;
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

    glutKeyboardFunc(
        tecladoNormal
    );

    glutTimerFunc(
        16,
        update,
        0
    );

    glutMainLoop();

    return 0;
}