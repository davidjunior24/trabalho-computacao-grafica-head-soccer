#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

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
// dentro de uma pasta build/), ajuste os caminhos em CAMINHO_MODELO_CAMPO
// e CAMINHO_MODELO_TRAVE mais abaixo, ou copie a pasta assets pra junto do
// executavel.
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

//=====================================
// VARIAVEIS
//=====================================

// Posicoes iniciais (usadas pra voltar tudo ao normal apos cada gol)
const float JOGADOR_X_INICIAL = -6.0f;
const float CPU_X_INICIAL     =  6.0f;
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
//
// GOL_X = 14: distancia correta do centro do campo ate o centro da trave,
// conforme medido no modelo do Blender. Campo tem comprimento 35 (-17.5 a +17.5).
const float GOL_X       = 14.0f; // centro do gol (correto, medido no Blender)
const float GOL_META    = 1.5f;  // metade do lado do cubo do gol
const float GOL_FRENTE  = GOL_X - GOL_META; // linha do gol = abertura, voltada pro campo
const float GOL_FUNDO   = GOL_X + GOL_META; // fundo fechado da caixa do gol
const float GOL_TRAVE_Y = 3.0f;             // altura da trave (topo da abertura)

// Parede de seguranca no limite real do campo. O campo tem comprimento 35
// (de -17.5 a +17.5). As traves ficam centradas em x=14 e tem GOL_META=1.5
// de profundidade, indo ate x=15.5. A parede fica em x=17.5, na borda real
// do campo, totalmente atras das traves -- invisivel, so fisica.
const float PAREDE_FUNDO_X = 17.5f;
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

// ============================================================
// MODELOS 3D (CAMPO E TRAVE) -- AJUSTES VISUAIS
// ============================================================
// Nada aqui afeta fisica, colisao ou deteccao de gol -- e tudo so
// renderizacao. Os valores de offset comecam em uma estimativa razoavel
// (considerando "Origin to Geometry" no Blender) mas podem precisar de um
// pequeno ajuste fino depois de ver o resultado na tela.

const char* CAMINHO_MODELO_CAMPO = "assets/models/Campo_Oficial.glb";
const char* CAMINHO_MODELO_TRAVE = "assets/models/Trave_Oficial.glb";

// O Origin do campo foi movido (no Blender) pra parte externa, no topo do
// gramado -- ou seja, o (0,0,0) do modelo ja e exatamente a superficie do
// gramado. Como a fisica do jogo tambem considera o chao em y=0, nao
// precisamos mais de nenhum offset vertical pra alinhar os dois.
const float CAMPO_OFFSET_X = 0.0f;
const float CAMPO_OFFSET_Y = -2.0f;
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

    // Cor "de fallback" (do material), usada quando NAO ha cor por vertice
    float cor[3];

    // Cor por vertice (vertex paint do Blender), se o mesh tiver. Fica
    // vazio quando o mesh nao tem cores de vertice -- nesse caso usamos
    // so' "cor" (acima) pro grupo inteiro.
    std::vector<aiVector3D> coresPorVertice;
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
    }

private:
    std::vector<GrupoModelo3D> grupos;
    GLuint listaDeExibicao = 0;

    void processarMesh(const aiScene* cena, const aiMesh* mesh)
    {
        GrupoModelo3D grupo;

        // Cor padrao (cinza neutro), usada so se nenhuma fonte de cor abaixo for encontrada
        grupo.cor[0] = 0.8f;
        grupo.cor[1] = 0.8f;
        grupo.cor[2] = 0.8f;

        bool corDoMaterialEncontrada = false;

        if(mesh->mMaterialIndex < cena->mNumMaterials)
        {
            const aiMaterial* mat = cena->mMaterials[mesh->mMaterialIndex];

            // 1) Material PBR (glTF "Base Color") -- e o caminho mais comum
            //    pra materiais exportados do Blender em .glb
            aiColor4D corBase(0.8f, 0.8f, 0.8f, 1.0f);
            if(mat->Get(AI_MATKEY_BASE_COLOR, corBase) == AI_SUCCESS)
            {
                grupo.cor[0] = corBase.r;
                grupo.cor[1] = corBase.g;
                grupo.cor[2] = corBase.b;
                corDoMaterialEncontrada = true;
            }

            // 2) Material "legado" (diffuse) -- fallback pra formatos/materiais
            //    mais antigos que nao tem Base Color PBR
            if(!corDoMaterialEncontrada)
            {
                aiColor3D corDiffuse(0.8f, 0.8f, 0.8f);
                if(mat->Get(AI_MATKEY_COLOR_DIFFUSE, corDiffuse) == AI_SUCCESS)
                {
                    grupo.cor[0] = corDiffuse.r;
                    grupo.cor[1] = corDiffuse.g;
                    grupo.cor[2] = corDiffuse.b;
                    corDoMaterialEncontrada = true;
                }
            }
        }

        // 3) Cor por vertice (vertex paint) -- se existir, tem prioridade
        //    sobre a cor "achatada" do material, porque e mais especifica
        bool temCorPorVertice = mesh->HasVertexColors(0);

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
            bool temCorPorVertice = !grupo.coresPorVertice.empty();

            // Se nao tem cor por vertice, a cor e a mesma pro grupo inteiro,
            // entao so precisamos definir uma vez antes do glBegin
            if(!temCorPorVertice)
                glColor3f(grupo.cor[0], grupo.cor[1], grupo.cor[2]);

            glBegin(GL_TRIANGLES);
            for(size_t i = 0; i < grupo.vertices.size(); i++)
            {
                if(temCorPorVertice)
                {
                    const aiVector3D &c = grupo.coresPorVertice[i];
                    glColor3f(c.x, c.y, c.z);
                }

                glNormal3f(grupo.normais[i].x, grupo.normais[i].y, grupo.normais[i].z);
                glVertex3f(grupo.vertices[i].x, grupo.vertices[i].y, grupo.vertices[i].z);
            }
            glEnd();
        }

        glEndList();

        // Os dados ja estao na display list (na GPU), nao precisamos mais
        // guardar essas listas de vertices na CPU
        grupos.clear();
    }
};

// Instancias globais dos dois modelos. Sao carregadas uma unica vez, em
// init(), e reutilizadas em todo frame.
Modelo3D modeloCampo;
Modelo3D modeloTrave;

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

        // Apos a resolucao, garante que nenhum personagem atravessou o chao.
        // Chao logico agora em y=raioCabeca (pivo do campo no topo do gramado).
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

    // Leve atrito do ar: a bola perde um pouco de energia horizontal com o
    // tempo, em vez de quicar pra sempre com a mesma forca (deixa menos
    // artificial). Usamos powf pra esse "decaimento" continuar correto
    // independente do dt (em vez de so multiplicar por 0.999 a cada frame).
    velBolaX *= powf(0.999f, dt);

    // Chao
    if(bolaY < raioBola)
    {
        bolaY = raioBola;

        // Dentro da caixa do gol (atras da linha de frente), o teto da rede
        // do modelo do Blender e inclinado em direcao ao fundo. Com o
        // rebote normal (-0.8), a bola pode ficar quicando ali sem nunca
        // ganhar velocidade suficiente pra escapar de volta pro campo --
        // por isso, so' ali dentro, o rebote vertical e mais forte e a
        // bola recebe um empurraozinho horizontal de volta pra abertura.
        bool dentroGolDireito  = (bolaX >  GOL_FRENTE);
        bool dentroGolEsquerdo = (bolaX < -GOL_FRENTE);

        if(dentroGolDireito || dentroGolEsquerdo)
        {
            velBolaY *= -0.95f;
            velBolaX += (dentroGolDireito ? -1.0f : 1.0f) * 0.025f * dt;

            // Garante um salto minimo, pra ela sempre conseguir voltar a
            // quicar em direcao a abertura em vez de ficar "morta" no chao
            if(velBolaY < 0.12f)
                velBolaY = 0.12f;
        }
        else
        {
            velBolaY *= -0.8f;
        }

        if(bolaEmSaque)
        {
            // So ganha velocidade horizontal depois de cair reto e tocar o chao
            // Velocidade aumentada (antes 0.08, muito lenta)
            velBolaX = (rand()%2==0) ? 0.18f : -0.18f;
            bolaEmSaque = false;
        }
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

    // ---- CPU ----
    float movCpuAtual = 0.0f;

    if(bolaX > cpuX + 0.1f)
    {
        cpuX += velMovCpu * dt;
        movCpuAtual = velMovCpu;
    }
    else if(bolaX < cpuX - 0.1f)
    {
        cpuX -= velMovCpu * dt;
        movCpuAtual = -velMovCpu;
    }

    // IA agora pula pra cabecear quando a bola esta de fato a seu alcance
    // (distancia real, nao so uma faixa larga de X/Y), e somente quando a
    // bola esta acima dela. Isso evita que a CPU saia pulando toda vez que
    // a bola so passa perto, e -- combinado com o toque unico por contato --
    // evita que ela fique "cabeceando" a bola pra cima sem parar.
    float distCpuBola = sqrt((bolaX-cpuX)*(bolaX-cpuX) + (bolaY-cpuY)*(bolaY-cpuY));

    if(distCpuBola < 2.6f && bolaY > cpuY && cpuY <= raioCabeca + 0.05f)
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

    // Usa GOL_X (= 14) -- antes estava hardcoded como 13, desalinhando a trave esquerda
    desenharGol(-GOL_X, -1.0f);
    desenharGol( GOL_X,  1.0f);

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

    // GL_COLOR_MATERIAL faz com que glColor3f alimente o material difuso E
    // ambiente simultaneamente. Sem glColorMaterial explicito, o default e
    // GL_FRONT_AND_BACK / GL_AMBIENT_AND_DIFFUSE -- mas e mais seguro
    // declarar explicitamente pra garantir que funcione com todos os drivers.
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

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