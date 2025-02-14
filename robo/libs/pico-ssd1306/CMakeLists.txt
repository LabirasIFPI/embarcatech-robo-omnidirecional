cmake_minimum_required(VERSION 3.13)

# Nome do projeto
project(pico-ssd1306)

# Adicionar os arquivos da biblioteca
add_library(pico-ssd1306
    ssd1306.c  # Arquivo fonte principal
)

# Incluir os diretórios de cabeçalhos
target_include_directories(pico-ssd1306 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR} 
    )

# Dependências da biblioteca
target_link_libraries(pico-ssd1306
    pico_stdlib    # Biblioteca padrão do Pico SDK
    hardware_i2c   # Biblioteca para suporte ao I2C
)
