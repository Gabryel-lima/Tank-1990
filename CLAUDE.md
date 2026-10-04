# Tank 1990

Clone de Battle City em C++17 com SDL2. Campanha de 1 a 4 jogadores e modos extras
(Duelo, Sobrevivência). Detalhes de uso no `README.md`.

## Antes de mexer em qualquer modo extra

Leia e siga **[MODOS_EXTRAS.md](MODOS_EXTRAS.md)**: as regras comuns a todos os modos extras
(jogadores, menu, mapas, cores, poderes, sinais visuais, fogo amigo, código) e o checklist de
um modo novo. Um modo novo segue todas as regras; uma exceção precisa de motivo e entra na
tabela de diferenças intencionais do fim do arquivo. Quando aparecer um padrão novo, repetido
em mais de um modo, ele vira regra lá e sobe para um ponto comum no código.

## Convenções

- Comentários e mensagens de commit em português; textos na tela do jogo em inglês.
- Números ajustáveis no `AppConfig` (`duel_*`, `survival_*`, `power_*`), comentados.
- A campanha não muda: os modos extras usam ganchos virtuais do `Game` cujo padrão é o
  comportamento da campanha.
- README em português (`README.md`) e em inglês (`README.en.md`), sempre os dois.

## Verificar

```bash
make build        # compila (sem warnings)
make check-maps   # valida os mapas de todos os modos extras
make duel-sim     # simulação do duelo sem janela (equilíbrio e partidas travadas)
```

Mudança no `Game`, no `Player`, no `Tank`, no `Object` ou no `Renderer`: confirme que as
telas da campanha continuam idênticas pixel a pixel.
