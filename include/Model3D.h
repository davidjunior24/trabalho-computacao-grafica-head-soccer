#ifndef MODEL3D_H
#define MODEL3D_H

#include <GL/glut.h>
#include <vector>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

//=====================================
// MODELO 3D ESTATICO (ASSIMP)
//=====================================
// Usado para cenario estatico (campo, trave, arquibancada, refletor,
// bola, nuvem) carregado de arquivos .glb com Assimp e desenhado via
// display list do OpenGL.

// Um grupo de triangulos que compartilham a mesma cor (vinda do material
// do Blender). Os vertices/normais ja saem da importacao em coordenadas
// "de mundo do modelo" (com a hierarquia de nodes e as transformacoes do
// Blender ja aplicadas, gracas a aiProcess_PreTransformVertices).
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
    // motivo no console e o modelo simplesmente nao desenha nada.
    bool carregar(const std::string &caminho);

    void desenhar() const;

    ~Modelo3D();

private:
    std::vector<GrupoModelo3D> grupos;
    std::vector<GLuint> texturasCarregadas;
    GLuint listaDeExibicao = 0;

    GLuint carregarTexturaDoMaterial(const aiScene* cena, const aiMaterial* mat);
    void processarMesh(const aiScene* cena, const aiMesh* mesh);
    void construirListaDeExibicao();
};

#endif // MODEL3D_H
