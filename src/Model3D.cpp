#include "Model3D.h"

#include <cstdio>
#include <cstdlib>
#include <GL/glu.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/texture.h>

// stb_image: decodifica PNG/JPG embutidos nos .glb para textura OpenGL.
// STB_IMAGE_IMPLEMENTATION so pode aparecer em UM arquivo .cpp do projeto.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool Modelo3D::carregar(const std::string &caminho)
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

void Modelo3D::desenhar() const
{
    if(listaDeExibicao != 0)
        glCallList(listaDeExibicao);
}

Modelo3D::~Modelo3D()
{
    if(listaDeExibicao != 0)
        glDeleteLists(listaDeExibicao, 1);

    for(GLuint tex : texturasCarregadas)
        glDeleteTextures(1, &tex);
}

// Carrega a textura Base Color (PBR) ou Diffuse (legado) de um
// material, se houver. Retorna 0 se o material nao tiver textura (caso
// em que o grupo cai pra cor solida / vertex color).
GLuint Modelo3D::carregarTexturaDoMaterial(const aiScene* cena, const aiMaterial* mat)
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

void Modelo3D::processarMesh(const aiScene* cena, const aiMesh* mesh)
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

void Modelo3D::construirListaDeExibicao()
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
