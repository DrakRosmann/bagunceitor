Trabalho Pratico 1 - Confusao e Difusao em Imagens
Programacao de Baixo Nivel - 2026/2

Integrantes: Guilherme de Lima Rosmann e Matheus Lopes Azambuja

Video da apresentacao (YouTube, nao listado): https://youtu.be/uAjDBBHjrDc
Repositorio GitHub: https://github.com/DrakRosmann/bagunceitor
Relatorio: relatorio.pdf (nesta pasta)

Compilar:   make            (Windows: mingw32-make)
Executar:   ./bagunceitor predio32.jpg [chave]
              -> gera etapa_confusao.png, etapa_difusao.png, baguncada.png,
                 recuperada.png e recuperada_chave_errada.png, e confere
                 byte a byte se a recuperada e identica a original.
            ./bagunceitor -e imagem.jpg [chave] [saida.png]     (so bagunca)
            ./bagunceitor -d baguncada.png [chave] [saida.png]  (so recupera)
Limpar:     make clean

Chave padrao: progbn2026
