# Ultra Warp Ride · Missão Living Dex

Plugin (.3gx) para **Pokémon Ultra Sun e Ultra Moon**, feito pela [Poké Prof. Bella](https://pokeprofbella.github.io/livingdex/) para a Missão Living Dex.

Durante o Ultra Warp Ride, ele mostra os **anos-luz exatos de cada buraco à vista**. É o mesmo número que aparece quando você entra nele. Junto vêm a **cor**, os **anéis** e as **chances de lendário, shiny e Ultra Beast** daquele buraco.

- **Tela de cima:** o buraco mais próximo (cor, anos-luz e anéis: 1\* 2\* 3\* = flor).
- **Tela de baixo:** todos os buracos à vista, do mais perto para o mais longe.

Com ele dá pra saber exatamente quando uma flor vai aparecer, ou se vale entrar num buraco de dois anéis quando você já está lenta e a gravidade puxando.

## Download

Baixe o arquivo **UltraWarpRide-v1.0.zip** na página de [Releases](../../releases/latest). O passo a passo de instalação está no `COMO-INSTALAR.txt` dentro do zip.

## Instalação rápida

### Azahar (testado)

1. Preferências → System → marque **Enable 3GX Plugin Loader**. Em cada cópia do Azahar que você usar.
2. Arquivo → Abrir pasta de usuário do Azahar → `sdmc`.
3. Copie `UltraWarpRide.3gx` para:
   - `luma/plugins/00040000001B5000/` (Ultra Sun)
   - `luma/plugins/00040000001B5100/` (Ultra Moon)
4. Feche e abra o jogo.

### 3DS com Luma3DS (ainda não testado)

Mesmas pastas no cartão SD. Ative o Plugin Loader no menu Rosalina (L + ↓ + Select). Se testar, abra uma *issue* contando se funcionou.

## Como funciona

O jogo não tem um contador de distância durante o voo. Cada buraco já nasce com a própria distância gravada. Quando você entra, o jogo divide esse número por 100 e arredonda pra cima. O plugin lê a lista de buracos que o jogo mantém na memória e desenha os dados na tela.

As chances por faixa de distância e por anel são as do Serebii e do Bulbapedia, as mesmas da aba Ultra Space do [guia da Living Dex Gen 7](https://pokeprofbella.github.io/livingdex/).

## Compilar (para quem quiser mexer no código)

1. Requer o [devkitPro](https://devkitpro.org/wiki/Getting_Started) com o pacote `3ds-dev`.
2. Rode `bash build.sh`. Ele cria `build/UltraWarpRide.3gx` e instala nos Azahar que estiverem na pasta ao lado.
3. O empacotamento .3gx é feito por `tools/pack3gx.py` (Python 3), sem precisar do 3gxtool.

## Créditos e licença

- Gancho de desenho na tela baseado no [PokeReader](https://github.com/zaksabeast/PokeReader), de zaksabeast.
- Fonte 8×8 das amostras do NTR (44670).
- Licença: **GPL-3.0** (veja `LICENSE`).
- Use com o seu próprio jogo. O plugin não contém nada do jogo.
