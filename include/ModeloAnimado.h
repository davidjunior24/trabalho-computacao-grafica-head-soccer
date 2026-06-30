#ifndef MODELO_ANIMADO_H
#define MODELO_ANIMADO_H

#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

//=====================================
// MODELO ANIMADO (jogador FBX com animacao de chute)
//=====================================
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
// POSICIONAMENTO:
//   Desenhamos com glTranslatef(jogadorX, jogadorY - raioCabeca, 0)
//   e a classe aplica internamente:
//     glTranslatef(offsetX, offsetY, offsetZ)  <- centraliza
//     glScalef(escala, escala, escala)
//   Assim o modelo fica centrado em X/Z com a base no chao.

class ModeloAnimado {
public:
    // Offsets de centralizacao (calculados em calcularEscala, usados no desenho)
    float escala   = 1.0f;
    float offsetX  = 0.0f; // translacao para centralizar em X
    float offsetY  = 0.0f; // translacao para base do modelo em y=0
    float offsetZ  = 0.0f; // translacao para centralizar em Z

    bool carregar(const std::string& caminho);

    // Desenha na pose de repouso
    void desenharRepouso() const;

    // Desenha o chute: anima Object_3 com anim[ANIM_CHUTE_CABECA] e
    // Object_3.001 com anim[ANIM_CHUTE_CORPO] no mesmo tempo em ticks.
    void desenharChute(float tempoTicks) const;

    float getDuracaoChute() const;
    float getTicksPerSecondChute() const;

private:
    Assimp::Importer importer;
    const aiScene*   cenaGlobal = nullptr;

    // Cores pre-carregadas por indice de material
    struct CorMaterial { float r, g, b; };
    std::vector<CorMaterial> coresMateriais;

    void carregarCoresMateriais();
    void aplicarOffsetEscala() const;

    aiNodeAnim* buscarCanal(const aiAnimation* anim, const std::string& nome) const;
    aiVector3D interpolarPos(const aiNodeAnim* ch, double t) const;
    aiQuaternion interpolarRot(const aiNodeAnim* ch, double t) const;
    aiVector3D interpolarEscala(const aiNodeAnim* ch, double t) const;
    aiMatrix4x4 transformacaoDoNo(const aiNode* node, double t, const aiAnimation* anim) const;

    void desenharNo(const aiNode* node, double t,
                    const aiAnimation* animCabeca,
                    const aiAnimation* animCorpo) const;
    void desenharMesh(const aiMesh* mesh) const;

    void calcularEscala();
    void calcularBBNo(const aiNode* node, aiMatrix4x4 parentGlobal,
                      float& xmin, float& xmax,
                      float& ymin, float& ymax,
                      float& zmin, float& zmax, bool& tem) const;
};

#endif // MODELO_ANIMADO_H
