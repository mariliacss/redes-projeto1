cmake_minimum_required(VERSION 3.10)  # Define a versão mínima do CMake
project(Streaming C)  # Nome do projeto e linguagem

# set(SOURCE_FILES src/main.c src/funcoes.c)  # Lista de arquivos fonte para fazer apenas um executável
# add_executable(meu_programa ${SOURCE_FILES})  # Cria o executável

# faz o executável do servidor
add_executable(server src/server.c)

# faz o executável do cliente
add_executable(client src/client.c)