# OmniBot: Robô Omnidirecional e Solução IoT para Controle e Monitoramento Remoto

# Robô Omnidirecional com Controle e Monitoramento Remoto via WiFi

Este repositório contém o código-fonte para um robô omnidirecional controlado remotamente via WiFi, utilizando requisições HTTP. O projeto é dividido em duas partes principais:

- **`robo/`**: Código-fonte do robô omnidirecional, incluindo controle de motores e comunicação com o sistema remoto.
- **`controle/`**: Código-fonte do sistema de controle e monitoramento remoto via WiFi.

## Funcionalidades

### Robô Omnidirecional (`robo/`)
- Controle de movimento utilizando rodas omnidirecionais Mecanum Wheels.
- Interface com motores via PWM.
- Comunicação via WiFi com o sistema remoto.
- Processamento de comandos recebidos via HTTP.
- Sensoriamento por intermédio do sensor de corrente ACS758.

### Sistema de Controle e Monitoramento Remoto (`controle/`)
- Comunicação via WiFi.
- Uso do botão B da placa de desenvolvimento BitDogLab para alternância entre os modos de controle normal (linear X e angular Z) e holonômico (linear X e linear Y).
- Uso do joystick para controle de movimentação.
- Envio de comandos HTTP PUT para movimentação.
- Envio de comandos HTTP GET para monitoramento dos parâmetros obtidos pelo sensor ACS758 do robô.


## 📂 Estrutura do Repositório
```
/
├── robo/                # Código-fonte do robô omnidirecional
│   ├── libs/            # Bibliotecas e sdk
│   ├── robo.c           # Arquivo C principal
│   ├── CMakeList.txt    # Arquivo de compilação
│
├── controle/     # Código-fonte do sistema de controle e monitoramento
│   ├── libs/            # Bibliotecas e sdk
│   ├── controle.c       # Arquivo C principal
│   ├── CMakeList.txt    # Arquivo de compilação
│
└── README.md            # Este arquivo
```

## Configuração e Uso

### Compatibilidade
- Placa de desenvolvimento BitDogLab.
- Raspberry Pi Pico W.

### Compilação e Upload
1. Clone este repositório:
   ```
   git clone https://github.com/seu-usuario/seu-repositorio.git
   ```
2. Execute a extensão do Visual Studio Code e abra o projeto na subpasta robo/
3. Atualize as linhas 19 e 20 do código com as suas credenciais WiFi:
   ```
   #define WIFI_SSID "your_ssid" 
   ```

   ```
   #define WIFI_PASSWORD "your_password" 
   ```
4. Compile o código do sistema do robô através da extensão
5. Faça o upload do firmware para o microcontrolador
6. Ligue o robô para verificar o seu IP na rede WiFi, e aguarde o LED acender, confirmando a conexão bem sucedida na rede WiFi. (Caso não conecte, reset o sistema)
7. Execute a extensão do Visual Studio Code e abra o projeto na subpasta controle/
8. Atualize as linhas 22, 23 e 27 do código com as suas devidas credenciais:
   ```
   #define WIFI_SSID "your_ssid" 
   ```

   ```
   #define WIFI_PASSWORD "your_password" 
   ```

   ```
   #define SERVER_IP "ip_do_robo" // IP que o seu robô exibiu no display OLED após se coneectar ao WiFi configurado. 
   ```
9. Compile o código do sistema de controle e monitoramento remoto através da extensão
10. Faça o upload do firmware para o microcontrolador
11. Ligue o sistema de controle e monitoramento remoto, e aguarde o LED acender, confirmando a conexão bem sucedida na rede WiFi. (Caso não conecte, reset o sistema)


## Possíveis dificuldades
Caso o seu robô ou o seu sistema de controle e monitoramento não acendam o led RGB, verifique se configurou corretamente as informações da rede WiFi e o IP do robô no sistema de controle.


## Direitos de Uso
Este projeto é disponibilizado para leitura e citação em outros trabalhos. Caso utilize este projeto como referência, por favor, forneça os devidos créditos ao autor.

## Autor
Desenvolvido por Paulo Roberto Araújo Leal.