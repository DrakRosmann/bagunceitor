/*
Compilar: make (Windows: mingw32-make)

./bagunceitor <imagem> [chave]
./bagunceitor -e <imagem> [chave] [saida.png]
./bagunceitor -d <baguncada.png> [chave] [saida.png]
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb_image.h>
#include <stb_image_write.h>

typedef struct {
  unsigned char r, g, b;
} Pixel;

typedef struct {
  int width, height;
  int channels;
  Pixel* pixels;
} Img;

#define CHAVE_PADRAO "progbn2026"

#define SAL_CONFUSAO 0xC0FFEE01u
#define SAL_CANAIS 0xD1F0500Au
#define SAL_PIXELS 0xD1F0500Bu

#define MULT 157u

void load(char* name, Img* pic);
Img nova_imagem(int width, int height);
void mostra_bytes(const char* titulo, const Img* pic);

uint32_t semente(const char* chave, uint32_t sal);
uint32_t xorshift32(uint32_t* estado);
unsigned char rotl8(unsigned char v, int r);
unsigned char rotr8(unsigned char v, int r);
unsigned char inverso_mod256(unsigned char a);

void confusao(Img* pic, const char* chave);
void confusao_inv(Img* pic, const char* chave);
size_t* gera_permutacao(size_t n, const char* chave);
void roda_canais(Img* pic, const char* chave, int sentido);
void difusao(const Img* src, Img* dst, const char* chave);
void difusao_inv(const Img* src, Img* dst, const char* chave);

void bagunca(const Img* orig, Img* baguncada, const char* chave);
void recupera(const Img* baguncada, Img* recuperada, const char* chave);
void grava(const char* nome, const Img* pic);
size_t compara(const Img* a, const Img* b);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Uso:\n");
    printf("  bagunceitor <imagem> [chave]                     (demonstracao completa)\n");
    printf("  bagunceitor -e <imagem> [chave] [saida.png]      (so bagunca)\n");
    printf("  bagunceitor -d <baguncada.png> [chave] [saida.png] (so recupera)\n");
    exit(1);
  }

  if (strcmp(argv[1], "-e") == 0 || strcmp(argv[1], "-d") == 0) {
    if (argc < 3) {
      printf("Faltou o nome da imagem\n");
      exit(1);
    }
    int embaralhar = argv[1][1] == 'e';
    const char* chave = argc > 3 ? argv[3] : CHAVE_PADRAO;
    const char* saida =
        argc > 4 ? argv[4] : (embaralhar ? "baguncada.png" : "recuperada.png");

    Img in, out;
    load(argv[2], &in);
    out = nova_imagem(in.width, in.height);
    if (embaralhar)
      bagunca(&in, &out, chave);
    else
      recupera(&in, &out, chave);
    grava(saida, &out);
    free(in.pixels);
    free(out.pixels);
    return 0;
  }

  const char* chave = argc > 2 ? argv[2] : CHAVE_PADRAO;
  Img in;
  load(argv[1], &in);
  printf("Origem   : %s %d x %d\n", argv[1], in.width, in.height);
  printf("Chave    : \"%s\"\n", chave);

  Img baguncada = nova_imagem(in.width, in.height);
  Img recuperada = nova_imagem(in.width, in.height);

  Img so_c = nova_imagem(in.width, in.height);
  memcpy(so_c.pixels, in.pixels, (size_t)in.width * in.height * sizeof(Pixel));
  confusao(&so_c, chave);
  grava("etapa_confusao.png", &so_c);

  Img so_d = nova_imagem(in.width, in.height);
  difusao(&in, &so_d, chave);
  grava("etapa_difusao.png", &so_d);

  printf("Processando...\n");
  bagunca(&in, &baguncada, chave);
  grava("baguncada.png", &baguncada);

  recupera(&baguncada, &recuperada, chave);
  grava("recuperada.png", &recuperada);

  mostra_bytes("Original", &in);
  mostra_bytes("Baguncada", &baguncada);
  mostra_bytes("Recuperada", &recuperada);

  size_t total = (size_t)in.width * in.height * 3;
  size_t dif_bag = compara(&in, &baguncada);
  size_t dif_rec = compara(&in, &recuperada);
  printf("\nBytes diferentes (original x baguncada) : %zu de %zu (%.2f%%)\n",
         dif_bag, total, 100.0 * dif_bag / total);
  printf("Bytes diferentes (original x recuperada): %zu de %zu\n", dif_rec,
         total);
  printf("memcmp(original, recuperada) = %d -> %s\n",
         memcmp(in.pixels, recuperada.pixels, total),
         dif_rec == 0 ? "RECUPERACAO EXATA" : "FALHOU");

  Img errada = nova_imagem(in.width, in.height);
  recupera(&baguncada, &errada, "chave-errada");
  grava("recuperada_chave_errada.png", &errada);
  printf("Com chave errada: %zu bytes diferentes\n", compara(&in, &errada));

  free(in.pixels);
  free(baguncada.pixels);
  free(recuperada.pixels);
  free(so_c.pixels);
  free(so_d.pixels);
  free(errada.pixels);
  return dif_rec == 0 ? 0 : 2;
}

void bagunca(const Img* orig, Img* baguncada, const char* chave) {
  size_t bytes = (size_t)orig->width * orig->height * sizeof(Pixel);
  Img tmp = nova_imagem(orig->width, orig->height);
  memcpy(tmp.pixels, orig->pixels, bytes);
  confusao(&tmp, chave);
  difusao(&tmp, baguncada, chave);
  free(tmp.pixels);
}

void recupera(const Img* baguncada, Img* recuperada, const char* chave) {
  difusao_inv(baguncada, recuperada, chave);
  confusao_inv(recuperada, chave);
}

uint32_t semente(const char* chave, uint32_t sal) {
  uint32_t h = 2166136261u ^ sal;
  for (const unsigned char* p = (const unsigned char*)chave; *p; p++) {
    h ^= *p;
    h *= 16777619u;
  }
  return h ? h : 1;  // xorshift trava no 0
}

uint32_t xorshift32(uint32_t* estado) {
  uint32_t x = *estado;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return *estado = x;
}

unsigned char rotl8(unsigned char v, int r) {
  return (unsigned char)((v << r) | (v >> ((8 - r) & 7)));
}

unsigned char rotr8(unsigned char v, int r) {
  return (unsigned char)((v >> r) | (v << ((8 - r) & 7)));
}

unsigned char inverso_mod256(unsigned char a) {
  unsigned char inv = a;
  for (int i = 0; i < 3; i++) inv = (unsigned char)(inv * (2 - a * inv));
  return inv;
}

void confusao(Img* pic, const char* chave) {
  uint32_t estado = semente(chave, SAL_CONFUSAO);
  unsigned char* p = (unsigned char*)pic->pixels;
  unsigned char* fim = p + (size_t)pic->width * pic->height * 3;
  unsigned char anterior = (unsigned char)estado;

  while (p < fim) {
    uint32_t sorteio = xorshift32(&estado);
    unsigned char k = sorteio & 0xFF;
    int r = (sorteio >> 8) & 7;

    unsigned char y = *p ^ k;
    y = rotl8(y, r);
    y = (unsigned char)(y * MULT);
    y = (unsigned char)(y + anterior);

    *p++ = y;
    anterior = y;
  }
}

void confusao_inv(Img* pic, const char* chave) {
  uint32_t estado = semente(chave, SAL_CONFUSAO);
  unsigned char* p = (unsigned char*)pic->pixels;
  unsigned char* fim = p + (size_t)pic->width * pic->height * 3;
  unsigned char anterior = (unsigned char)estado;
  unsigned char mult_inv = inverso_mod256(MULT);

  while (p < fim) {
    uint32_t sorteio = xorshift32(&estado);
    unsigned char k = sorteio & 0xFF;
    int r = (sorteio >> 8) & 7;

    unsigned char c = *p;
    unsigned char y = (unsigned char)(c - anterior);
    y = (unsigned char)(y * mult_inv);
    y = rotr8(y, r);
    *p++ = y ^ k;
    anterior = c;
  }
}

size_t* gera_permutacao(size_t n, const char* chave) {
  size_t* perm = malloc(n * sizeof(size_t));
  if (!perm) {
    printf("Sem memoria\n");
    exit(1);
  }
  for (size_t i = 0; i < n; i++) *(perm + i) = i;

  uint32_t estado = semente(chave, SAL_PIXELS);
  for (size_t i = n - 1; i > 0; i--) {
    size_t j = xorshift32(&estado) % (i + 1);
    size_t t = *(perm + i);
    *(perm + i) = *(perm + j);
    *(perm + j) = t;
  }
  return perm;
}

void roda_canais(Img* pic, const char* chave, int sentido) {
  uint32_t estado = semente(chave, SAL_CANAIS);
  Pixel* px = pic->pixels;
  Pixel* fim = px + (size_t)pic->width * pic->height;

  for (; px < fim; px++) {
    int passo = xorshift32(&estado) % 3;
    if (sentido < 0) passo = (3 - passo) % 3;

    unsigned char* b = (unsigned char*)px;
    unsigned char t[3] = {b[0], b[1], b[2]};
    for (int c = 0; c < 3; c++) *(b + c) = t[(c + passo) % 3];
  }
}

void difusao(const Img* src, Img* dst, const char* chave) {
  size_t n = (size_t)src->width * src->height;
  size_t* perm = gera_permutacao(n, chave);

  Pixel* d = dst->pixels;
  for (size_t* p = perm; p < perm + n; p++) *d++ = *(src->pixels + *p);
  free(perm);

  roda_canais(dst, chave, +1);
}

void difusao_inv(const Img* src, Img* dst, const char* chave) {
  size_t n = (size_t)src->width * src->height;
  size_t* perm = gera_permutacao(n, chave);

  Img tmp = nova_imagem(src->width, src->height);
  memcpy(tmp.pixels, src->pixels, n * sizeof(Pixel));
  roda_canais(&tmp, chave, -1);

  Pixel* s = tmp.pixels;
  for (size_t* p = perm; p < perm + n; p++) *(dst->pixels + *p) = *s++;

  free(tmp.pixels);
  free(perm);
}

Img nova_imagem(int width, int height) {
  Img pic;
  pic.width = width;
  pic.height = height;
  pic.channels = 3;
  size_t bytes = (size_t)width * height * sizeof(Pixel);
  pic.pixels = malloc(bytes);
  if (!pic.pixels) {
    printf("Sem memoria\n");
    exit(1);
  }
  memset(pic.pixels, 0, bytes);
  return pic;
}

size_t compara(const Img* a, const Img* b) {
  const unsigned char* pa = (const unsigned char*)a->pixels;
  const unsigned char* pb = (const unsigned char*)b->pixels;
  const unsigned char* fim = pa + (size_t)a->width * a->height * 3;
  size_t diferentes = 0;
  while (pa < fim) diferentes += *pa++ != *pb++;
  return diferentes;
}

void grava(const char* nome, const Img* pic) {
  if (!stbi_write_png(nome, pic->width, pic->height, 3, pic->pixels, 0)) {
    printf("Erro ao gravar %s\n", nome);
    exit(1);
  }
  printf("Gravado  : %s\n", nome);
}

void mostra_bytes(const char* titulo, const Img* pic) {
  printf("\n%s (%p):\n", titulo, (void*)pic->pixels);
  Pixel* px = pic->pixels;
  for (int i = 0; i < 8; i++, px++)
    printf("[%02X %02X %02X] ", px->r, px->g, px->b);
  printf("\n");
}

void load(char* name, Img* pic) {
  pic->pixels = (Pixel*)stbi_load(name, &pic->width, &pic->height,
                                  &pic->channels, 3);
  if (!pic->pixels) {
    printf("Erro de leitura: %s\n", stbi_failure_reason());
    exit(1);
  }
  printf("Load: %d x %d x %d\n", pic->width, pic->height, pic->channels);
  pic->channels = 3;
}
