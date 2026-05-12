#!/bin/bash

# Pára imediatamente se der algum erro fatal
set -e

start=$(date +'%s')
CORES=$(nproc)

echo "A compilar o kernel na pasta linux-6.19.9-moker com $CORES threads..."

cd linux-6.19.9-moker

# Compilação (sem sudo). Se falhar (||), captura o erro e mostra na hora!
make -j"$CORES" 2> ../errors-6.19.9-moker || {
    echo "--- A COMPILAÇÃO FALHOU! Erros registados: ---"
    cat ../errors-6.19.9-moker
    echo "----------------------------------------------"
    exit 1
}

echo "Compilação concluída com sucesso. A instalar..."

# Instalação (com sudo)
sudo make modules_install
sudo make install

cd ..

sudo update-grub2

echo "--- Avisos registados durante a compilação ---"
cat errors-6.19.9-moker
echo "----------------------------------------------"

echo "Linux kernel compilation and installation took $(($(date +'%s') - $start)) seconds"
