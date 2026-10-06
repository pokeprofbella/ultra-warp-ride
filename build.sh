#!/bin/bash
# Compila o plugin e instala no Azahar. Uso: bash build.sh
set -e
cd "$(dirname "$0")"

export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM

if [ ! -x "$DEVKITARM/bin/arm-none-eabi-gcc" ] || [ ! -f "$DEVKITPRO/libctru/lib/libctru.a" ]; then
  echo ""
  echo ">> Falta o devkitPro com o kit de 3DS."
  echo ">> Veja o passo 1 do LEIA-ME.md e rode este script de novo."
  exit 1
fi

rm -rf build
make -f plugin.mk
echo ""

# Instala nos Azahar que estiverem ao lado desta pasta (Azahar 1, Azahar 2...)
FOUND=0
for SDMC in ../Azahar*/user/sdmc; do
  [ -d "$SDMC" ] || continue
  for TID in 00040000001B5000 00040000001B5100; do
    mkdir -p "$SDMC/luma/plugins/$TID"
    cp build/UltraWarpRide.3gx "$SDMC/luma/plugins/$TID/UltraWarpRide.3gx"
  done
  echo ">> Instalado em: $(cd "$SDMC/../.." && pwd)"
  FOUND=1
done
if [ "$FOUND" = 1 ]; then
  echo ">> Pronto! Plugin instalado no Azahar (Ultra Sun e Ultra Moon)."
else
  echo ">> Plugin criado em build/UltraWarpRide.3gx, mas nao achei a pasta do Azahar."
fi
