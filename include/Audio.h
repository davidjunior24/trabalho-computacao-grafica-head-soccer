#ifndef AUDIO_H
#define AUDIO_H

//=====================================
// AUDIO (SDL2_mixer)
//=====================================
// Mixagem real -- toca gol e apito simultaneamente, e musica de fundo em
// loop. Requer SDL2_mixer instalado (ver instrucoes no README/CMake).

// Inicializa o SDL2_mixer, carrega o WAV de gol e a musica de fundo
// (que comeca a tocar em loop imediatamente). Chamado uma unica vez em init().
void carregarSomGol();

// Carrega o WAV do apito. Chamado uma unica vez em init().
void carregarSomApito();

void tocarSomGol();
void tocarSomApito();

#endif // AUDIO_H
