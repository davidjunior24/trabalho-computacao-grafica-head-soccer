#include "ModeloAnimado.h"
#include "Config.h"

#include <cstdio>
#include <GL/glut.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

bool ModeloAnimado::carregar(const std::string& caminho) {
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

void ModeloAnimado::desenharRepouso() const {
    if (!cenaGlobal) return;
    aplicarOffsetEscala();
    desenharNo(cenaGlobal->mRootNode, -1.0, nullptr, nullptr);
    glPopMatrix();
}

void ModeloAnimado::desenharChute(float tempoTicks) const {
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

float ModeloAnimado::getDuracaoChute() const {
    if (!cenaGlobal || ANIM_CHUTE_CABECA >= cenaGlobal->mNumAnimations) return 0.5f;
    const aiAnimation* a = cenaGlobal->mAnimations[ANIM_CHUTE_CABECA];
    float tps = (a->mTicksPerSecond > 0) ? (float)a->mTicksPerSecond : 24.0f;
    return (float)a->mDuration / tps;
}

float ModeloAnimado::getTicksPerSecondChute() const {
    if (!cenaGlobal || ANIM_CHUTE_CABECA >= cenaGlobal->mNumAnimations) return 24.0f;
    const aiAnimation* a = cenaGlobal->mAnimations[ANIM_CHUTE_CABECA];
    return (a->mTicksPerSecond > 0) ? (float)a->mTicksPerSecond : 24.0f;
}

// ---- Pre-carrega cores dos materiais ----
void ModeloAnimado::carregarCoresMateriais() {
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
void ModeloAnimado::aplicarOffsetEscala() const {
    glPushMatrix();
    // Primeiro translada para centralizar o modelo em X/Z
    // e colocar a base em y=0
    glTranslatef(offsetX, offsetY, offsetZ);
    glScalef(escala, escala, escala);
}

// ---- Busca canal de animacao por nome de no ----
aiNodeAnim* ModeloAnimado::buscarCanal(const aiAnimation* anim, const std::string& nome) const {
    if (!anim) return nullptr;
    for (unsigned i = 0; i < anim->mNumChannels; i++)
        if (anim->mChannels[i]->mNodeName.C_Str() == nome)
            return anim->mChannels[i];
    return nullptr;
}

aiVector3D ModeloAnimado::interpolarPos(const aiNodeAnim* ch, double t) const {
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

aiQuaternion ModeloAnimado::interpolarRot(const aiNodeAnim* ch, double t) const {
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

aiVector3D ModeloAnimado::interpolarEscala(const aiNodeAnim* ch, double t) const {
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

aiMatrix4x4 ModeloAnimado::transformacaoDoNo(const aiNode* node, double t, const aiAnimation* anim) const {
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
void ModeloAnimado::desenharNo(const aiNode* node, double t,
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

void ModeloAnimado::desenharMesh(const aiMesh* mesh) const {
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
void ModeloAnimado::calcularEscala() {
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

void ModeloAnimado::calcularBBNo(const aiNode* node, aiMatrix4x4 parentGlobal,
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
