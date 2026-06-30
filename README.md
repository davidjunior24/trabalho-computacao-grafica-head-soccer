# Head Soccer 3D

## Estrutura do projeto sugerida

```
headsoccer/
├── CMakeLists.txt
├── README.md
├── .gitignore
│
├── include/              # cabecalhos (.h) -- interfaces dos modulos
│   ├── Config.h           # constantes do jogo (caminhos, geometria, HUD)
│   ├── GameState.h         # estado mutavel global (posicoes, placar, etc.)
│   ├── Model3D.h            # modelo 3D estatico (campo, trave, cenario)
│   ├── ModeloAnimado.h       # modelo 3D animado (jogador/CPU, FBX)
│   ├── Audio.h                 # SDL2_mixer (gol, apito, musica)
│   ├── Physics.h                 # colisoes, gol/trave/parede, previsao
│   ├── HUD.h                       # placar e menu de pausa
│   ├── Input.h                       # callbacks de teclado
│   ├── Renderer.h                       # desenho da cena (display/reshape)
│   └── Update.h                          # loop principal (fisica + IA)
│
├── src/                   # implementacoes (.cpp)
│   ├── main.cpp             # ponto de entrada: init() + main()
│   ├── GameState.cpp
│   ├── Model3D.cpp
│   ├── ModeloAnimado.cpp
│   ├── Audio.cpp
│   ├── Physics.cpp
│   ├── HUD.cpp
│   ├── Input.cpp
│   ├── Renderer.cpp
│   └── Update.cpp
│
├── third_party/            # bibliotecas header-only de terceiros
│   └── stb_image.h           # baixe em:
│                              # https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
│
└── assets/
    └── models/
        ├── campo.glb
        ├── trave.glb
        ├── arquibancadas.glb
        ├── refletor.glb
        ├── bola.glb
        ├── nuvem.glb
        ├── jogador.fbx
        ├── jogadorIA.fbx
        └── sounds/
            ├── gol.wav
            ├── apito.wav
            └── musicafundo.wav
```

### Por que essa organizacao

- **include/ vs src/**: separa a "interface" (o que cada modulo expõe)
  da implementacao. Isso deixa mais facil entender o jogo so' lendo os
  `.h`, sem precisar abrir a implementacao inteira.
- **Um modulo por responsabilidade**: cada arquivo cuida de uma parte
  clara do jogo (fisica, IA+loop, HUD, renderizacao, audio, modelos 3D).
  Isso facilita achar onde mexer quando for ajustar algo especifico
  (por exemplo, toda a IA da CPU agora esta isolada em `Update.cpp`,
  e toda a fisica de colisao em `Physics.cpp`).
- **third_party/**: bibliotecas externas de terceiros (header-only, como
  o `stb_image.h`) ficam separadas do seu proprio codigo, deixando claro
  o que voce escreveu e o que e dependencia externa.
- **assets/**: nenhuma mudanca aqui -- os caminhos em `Config.h` ja
  assumem que o executavel roda com `assets/` ao lado dele (o
  `CMakeLists.txt` incluido copia essa pasta automaticamente para o
  diretorio de build).
- **CMakeLists.txt**: builda o projeto buscando automaticamente todos os
  `.cpp` de `src/`, entao adicionar um novo arquivo no futuro nao exige
  editar nada no build -- so criar o `.cpp` e o `.h` correspondente.

### Sugestoes adicionais (conforme o projeto crescer)

- Se a IA da CPU continuar crescendo, considere separar a logica de IA
  (a parte de `Update.cpp` entre `// IA DA CPU` e `// DECISAO DE PULO`)
  em um `AI.h`/`AI.cpp` proprio, retornando apenas o alvo e a decisao de
  pulo para o `update()` aplicar.
- Um arquivo `.gitignore` ignorando `build/` evita versionar artefatos
  de compilacao.
- Se for adicionar testes, uma pasta `tests/` separada (testando por
  exemplo as funcoes puras de `Physics.cpp`, como `preverBola` e
  `tratarGol`) e o caminho natural.

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
./headsoccer        # (ou headsoccer.exe no Windows)
```

Dependencias necessarias: OpenGL, GLUT, Assimp, SDL2 e SDL2_mixer (ver
comentarios no topo de `src/main.cpp` para instrucoes de instalacao).
Lembre de colocar o `stb_image.h` em `third_party/` antes de compilar.

## Observacao sobre `desenharNuvens()`

A funcao `desenharNuvens()` (em `Renderer.cpp`) ja existe e o modelo de
nuvem (`modeloNuvem`) e carregado normalmente em `init()`, mas a funcao
nunca e chamada dentro de `display()` no codigo original -- ou seja, as
nuvens sao carregadas mas nunca aparecem na tela. Mantive a funcao (nao
e "morta" no sentido de inutil, parece um recurso que ficou pela metade),
mas vale decidir: ou chamar `desenharNuvens()` dentro de `display()`
(por exemplo, junto de `desenharArquibancada()`), ou remover o modelo
e a funcao caso as nuvens nao sejam mais desejadas.
