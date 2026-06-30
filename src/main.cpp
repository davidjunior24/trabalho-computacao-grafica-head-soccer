// ============================================================
// Head Soccer 3D -- main.cpp
// ============================================================
// Ponto de entrada do jogo: inicializa GLUT/OpenGL, carrega os modelos
// 3D e o audio, registra os callbacks, e entra no loop principal.
//
// DEPENDENCIAS EXTERNAS:
//
// 1) Assimp -- carrega os modelos 3D (.glb / .fbx) feitos no Blender.
//      Linux:   sudo apt install libassimp-dev
//      Compilar: g++ ... -lassimp
//
// 2) SDL2_mixer -- audio (gol, apito e musica de fundo tocam ao mesmo
//    tempo, em canais separados).
//      MSYS2/UCRT64: pacman -S mingw-w64-ucrt-x86_64-SDL2_mixer
//      Compilar: adicione -lSDL2main -lSDL2 -lSDL2_mixer
//
// 3) stb_image -- decodifica as texturas (PNG/JPG) embutidas nos .glb.
//    A implementacao (STB_IMAGE_IMPLEMENTATION) fica em Model3D.cpp.
//
// Os arquivos .glb/.fbx precisam estar em "assets/models/" a partir da
// pasta de onde o executavel e executado. Veja Config.h para ajustar os
// caminhos se necessario.

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <GL/glut.h>

#include "Config.h"
#include "GameState.h"
#include "Audio.h"
#include "HUD.h"
#include "Input.h"
#include "Renderer.h"
#include "Update.h"

void init()
{
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // GL_COLOR_MATERIAL faz com que glColor3f alimente o material difuso E
    // ambiente simultaneamente.
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    // GL_NORMALIZE: reescala normais apos glScalef, necessario para
    // iluminacao correta no modelo do jogador (escala nao-uniforme do FBX).
    glEnable(GL_NORMALIZE);

    // GL_LIGHT_MODEL_TWO_SIDE: ilumina ambas as faces dos triangulos.
    // Necessario porque o FBX tem normais invertidas em alguns meshes.
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    // Luz ambiente global: sem isso, faces voltadas pra longe da luz
    // ficam completamente pretas, escondendo a cor dos modelos do Blender.
    GLfloat luzAmbienteGlobal[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, luzAmbienteGlobal);

    // Componente difusa da GL_LIGHT0 (a luz direcional principal)
    GLfloat difusa[]    = { 0.9f, 0.9f, 0.9f, 1.0f };
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

    // Pre-carrega os WAVs na RAM para eliminar delay na primeira reproducao
    carregarSomGol();
    carregarSomApito();

    resetarBola();
    resetarPosicoes();

    // Toca o apito sinalizando o inicio da partida
    tocarSomApito();

    // Carrega os modelos 3D (campo, trave, etc). Se algum nao for
    // encontrado, um aviso e impresso no console e o jogo continua
    // rodando -- a fisica/colisao nao dependem do modelo visual.
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

    glutMouseFunc(
        mouseClique
    );

    glutTimerFunc(
        16,
        update,
        0
    );

    glutMainLoop();

    return 0;
}
