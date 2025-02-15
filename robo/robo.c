#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "cJSON.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"

#include "hardware/pwm.h"

#include "hardware/i2c.h"
#include "ssd1306.h"

#include "hardware/adc.h" // Biblioteca para controle do ADC (Conversor Analógico-Digital).

#define WIFI_SSID "ssid"         // Nome de identificação da sua rede WiFi
#define WIFI_PASSWORD "password" // Senha da sua rede WiFi
#define SERVER_PORT 80 // Porta HTTP padrão

char *ip_addr_str;

#define PICO_DEFAULT_LED_PIN 11

const uint16_t PERIOD = 255;
const float DIVIDER_PWM = 1.0;

// Definição das variáveis dos valores de PWM
int16_t motor_level_1_a = 0;
int16_t motor_level_1_b = 0;
int16_t motor_level_2_a = 0;
int16_t motor_level_2_b = 0;
char conv_ml_1a_1b[20];
char conv_ml_2a_2b[20];

// Definição dos pinos da ponte H
const uint EN_1_A = 16;
#define IN_1_1 0
#define IN_1_2 1
const uint EN_1_B = 17;
#define IN_1_3 2
#define IN_1_4 3

const uint EN_2_A = 18;
#define IN_2_1 20
#define IN_2_2 4
const uint EN_2_B = 19;
#define IN_2_3 9
#define IN_2_4 8

#define ACS758 28
float tensao_fix = 12.0f;
float sensibilidade = 0.060f; // Sensibilidade do sensor ACS758 (60mV/A)
float tensao = 0;
float corrente = 0;
float watt = 0;
char conv_tensao[20];
char conv_corrente[20];
char conv_watt[20];

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display
#define I2C_SDA 14          // Pino SDA
#define I2C_SCL 15          // Pino SCL

// Instância do Display
ssd1306_t display;

// Função para enviar uma resposta HTTP com JSON
void http_send_response(struct tcp_pcb *tpcb, const char *status, const char *content_type, const char *body)
{
    char response[1024];
    snprintf(response, sizeof(response),
             "HTTP/1.1 %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status, content_type, (int)strlen(body), body);
    tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
}

// Função para processar o JSON recebido e controlar PWMs
void process_json_request(const char *json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL)
    {
        printf("Erro ao analisar o JSON\n");
        return;
    }

    // Tentar obter valores de pwm
    cJSON *ml_1a = cJSON_GetObjectItem(json, "ml_1a");
    cJSON *ml_1b = cJSON_GetObjectItem(json, "ml_1b");
    cJSON *ml_2a = cJSON_GetObjectItem(json, "ml_2a");
    cJSON *ml_2b = cJSON_GetObjectItem(json, "ml_2b");

    if (cJSON_IsNumber(ml_1a) && cJSON_IsNumber(ml_1b) && cJSON_IsNumber(ml_2a) && cJSON_IsNumber(ml_2b))
    {
        // Controlar o pwm com os valores recebidos

        motor_level_1_a = ml_1a->valueint;
        motor_level_1_b = ml_1b->valueint;
        motor_level_2_a = ml_2a->valueint;
        motor_level_2_b = ml_2b->valueint;

        if (motor_level_1_a < 0)
        {
            gpio_put(IN_1_1, 1);
            gpio_put(IN_1_2, 0);
        }
        else if (motor_level_1_a > 0)
        {
            gpio_put(IN_1_1, 0);
            gpio_put(IN_1_2, 1);
        }
        else
        {
            gpio_put(IN_1_1, 1);
            gpio_put(IN_1_2, 1);
        }

        if (motor_level_1_b < 0)
        {
            gpio_put(IN_1_3, 1);
            gpio_put(IN_1_4, 0);
        }
        else if (motor_level_1_b > 0)
        {
            gpio_put(IN_1_3, 0);
            gpio_put(IN_1_4, 1);
        }
        else
        {
            gpio_put(IN_1_3, 1);
            gpio_put(IN_1_4, 1);
        }

        if (motor_level_2_a < 0)
        {
            gpio_put(IN_2_1, 1);
            gpio_put(IN_2_2, 0);
        }
        else if (motor_level_2_a > 0)
        {
            gpio_put(IN_2_1, 0);
            gpio_put(IN_2_2, 1);
        }
        else
        {
            gpio_put(IN_2_1, 1);
            gpio_put(IN_2_2, 1);
        }

        if (motor_level_2_b < 0)
        {
            gpio_put(IN_2_3, 1);
            gpio_put(IN_2_4, 0);
        }
        else if (motor_level_2_b > 0)
        {
            gpio_put(IN_2_3, 0);
            gpio_put(IN_2_4, 1);
        }
        else
        {
            gpio_put(IN_2_3, 1);
            gpio_put(IN_2_4, 1);
        }

        printf("uint_ml_1a: %u\n", (uint16_t)abs(motor_level_1_a));
        printf("uint_ml_1b: %u\n", (uint16_t)abs(motor_level_1_b));
        printf("uint_ml_2a: %u\n", (uint16_t)abs(motor_level_2_a));
        printf("uint_ml_2b: %u\n", (uint16_t)abs(motor_level_2_b));
        pwm_set_gpio_level(EN_1_A, (uint16_t)abs(motor_level_1_a));
        pwm_set_gpio_level(EN_1_B, (uint16_t)abs(motor_level_1_b));
        pwm_set_gpio_level(EN_2_A, (uint16_t)abs(motor_level_2_a));
        pwm_set_gpio_level(EN_2_B, (uint16_t)abs(motor_level_2_b));
    }
    else
    {
        printf("Valores de RGB não encontrados ou inválidos no JSON\n");
    }

    cJSON_Delete(json); // Liberar memória
}

// Função de callback para processar as requisições HTTP
err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p != NULL)
    {
        // Copiar os dados para um buffer legível
        char buffer[2048];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, (char *)p->payload, p->len);
        buffer[p->len] = '\0'; // Garantir terminação nula

        // Verificar se é uma requisição PUT
        if (strncmp(buffer, "PUT /atuadores", 14) == 0)
        {
            // Procurar onde começa o corpo do JSON
            char *json_start = strstr(buffer, "\r\n\r\n");
            if (json_start != NULL)
            {
                json_start += 4; // Pular o cabeçalho HTTP

                printf("Corpo da requisição: %s\n", json_start);

                // Processar o JSON
                process_json_request(json_start);

                // Preparar a resposta
                char body[256];
                snprintf(body, sizeof(body), "{\"status\":\"OK\", \"message\":\"Dados de pwm atualizados\"}\n");
                http_send_response(tpcb, "200 OK", "application/json", body);
            }
        }
        else if (strncmp(buffer, "GET /sensores", 13) == 0)
        {
            char body[100]; // Ajuste o tamanho conforme necessário
            snprintf(body, sizeof(body),
                     "{\"Tensao\":%.3f,\"Corrente\":%.3f,\"Potencia\":%.3f}\n",
                     tensao, corrente, watt);
            http_send_response(tpcb, "200 OK", "application/json", body);
        }

        else
        {
            // Se não for uma requisição esperada, enviar uma mensagem de erro
            char *body = "{\"status\":\"ERROR\", \"message\":\"Requisição não suportada\"}\n";
            http_send_response(tpcb, "405 Method Not Allowed", "application/json", body);
        }

        // Liberar buffer
        pbuf_free(p);
    }
    else
    {
        // Se p for NULL, a conexão foi fechada
        tcp_close(tpcb);
    }
    return ERR_OK;
}

// Função de callback para gerenciar nova conexão
err_t http_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    // Definir a função de recebimento para esta nova conexão
    tcp_recv(newpcb, http_recv_callback);
    return ERR_OK;
}

// Função para iniciar o servidor HTTP
void http_server(void)
{
    struct tcp_pcb *pcb;
    err_t err;

    printf("Iniciando servidor HTTP...\n");

    // Criar um novo PCB (control block) para o servidor HTTP
    pcb = tcp_new();
    if (pcb == NULL)
    {
        printf("Erro ao criar o PCB HTTP.\n");
        return;
    }

    // Vincular o servidor ao endereço e porta desejada
    ip_addr_t ipaddr;
    IP4_ADDR(&ipaddr, 0, 0, 0, 0); // Ou use IP_ADDR_ANY para todas as interfaces
    err = tcp_bind(pcb, &ipaddr, SERVER_PORT);
    if (err != ERR_OK)
    {
        printf("Erro ao vincular ao endereço e porta.\n");
        return;
    }

    // Colocar o servidor para ouvir conexões
    pcb = tcp_listen(pcb);
    if (pcb == NULL)
    {
        printf("Erro ao colocar o servidor em escuta.\n");
        return;
    }

    // Configurar a função de aceitação das conexões
    tcp_accept(pcb, http_accept_callback);
    printf("Servidor HTTP iniciado na porta %d.\n", SERVER_PORT);
}

void setup_pwm()
{
    uint slice_1_a, slice_1_b, slice_2_a, slice_2_b;

    gpio_set_function(EN_1_A, GPIO_FUNC_PWM);
    slice_1_a = pwm_gpio_to_slice_num(EN_1_A);
    pwm_set_clkdiv(slice_1_a, DIVIDER_PWM);
    pwm_set_wrap(slice_1_a, PERIOD);
    pwm_set_gpio_level(EN_1_A, (uint16_t)abs(motor_level_1_a));
    pwm_set_enabled(slice_1_a, true);

    gpio_set_function(EN_1_B, GPIO_FUNC_PWM);
    slice_1_b = pwm_gpio_to_slice_num(EN_1_B);
    pwm_set_clkdiv(slice_1_b, DIVIDER_PWM);
    pwm_set_wrap(slice_1_b, PERIOD);
    pwm_set_gpio_level(EN_1_B, (uint16_t)abs(motor_level_1_b));
    pwm_set_enabled(slice_1_b, true);

    gpio_set_function(EN_2_A, GPIO_FUNC_PWM);
    slice_2_a = pwm_gpio_to_slice_num(EN_2_A);
    pwm_set_clkdiv(slice_2_a, DIVIDER_PWM);
    pwm_set_wrap(slice_2_a, PERIOD);
    pwm_set_gpio_level(EN_2_A, (uint16_t)abs(motor_level_2_a));
    pwm_set_enabled(slice_2_a, true);

    gpio_set_function(EN_2_B, GPIO_FUNC_PWM);
    slice_2_b = pwm_gpio_to_slice_num(EN_2_B);
    pwm_set_clkdiv(slice_2_b, DIVIDER_PWM);
    pwm_set_wrap(slice_2_b, PERIOD);
    pwm_set_gpio_level(EN_2_B, (uint16_t)abs(motor_level_2_b));
    pwm_set_enabled(slice_2_b, true);
}

// Função para converter o valor lido do ADC para corrente em Amperes
float adc_to_tensao(uint16_t adc_value)
{
    // Constantes fornecidas no datasheet do RP2040
    const float conversion_factor = 3.3f / (1 << 12); // Conversão de 12 bits (0-4095) para 0-3.3V
    float voltagem = adc_value * conversion_factor;   // Converte o valor ADC para tensão
    return voltagem;
}

int main()
{
    stdio_init_all();
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    gpio_init(IN_1_1);
    gpio_set_dir(IN_1_1, GPIO_OUT);
    gpio_init(IN_1_2);
    gpio_set_dir(IN_1_2, GPIO_OUT);
    gpio_init(IN_1_3);
    gpio_set_dir(IN_1_3, GPIO_OUT);
    gpio_init(IN_1_4);
    gpio_set_dir(IN_1_4, GPIO_OUT);

    gpio_init(IN_2_1);
    gpio_set_dir(IN_2_1, GPIO_OUT);
    gpio_init(IN_2_2);
    gpio_set_dir(IN_2_2, GPIO_OUT);
    gpio_init(IN_2_3);
    gpio_set_dir(IN_2_3, GPIO_OUT);
    gpio_init(IN_2_4);
    gpio_set_dir(IN_2_4, GPIO_OUT);

    setup_pwm();

    gpio_put(IN_1_1, 1);
    gpio_put(IN_1_2, 1);
    gpio_put(IN_1_3, 1);
    gpio_put(IN_1_4, 1);

    gpio_put(IN_2_1, 1);
    gpio_put(IN_2_2, 1);
    gpio_put(IN_2_3, 1);
    gpio_put(IN_2_4, 1);

    // Inicializa o módulo ADC (Conversor Analógico-Digital) do Pico
    // Isso prepara o ADC para começar a ler os valores dos canais analógicos.
    adc_init();

    // Configura o pino GPIO 28 para uso como entrada ADC (Canal 2)
    // O pino 28 está associado ao canal 2 do ADC.
    adc_gpio_init(ACS758);

    // Seleciona o canal 2 do ADC para leitura
    // Essa função configura o ADC para ler o valor do pino 28.
    adc_select_input(2);

    // Inicializa I2C no canal 1
    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1))
    {
        printf("Falha ao inicializar o display SSD1306\n");
        return 1; // Sai do programa
    }

    printf("Display SSD1306 inicializado com sucesso!\n");

    // Inicializa o chip WiFi
    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Falha ao conectar ao WiFi.\n");
    }
    else
    {
        printf("Conectado ao WiFi.\n");
        gpio_put(PICO_DEFAULT_LED_PIN, 1);

        // Lê o endereço IP
        uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
        ip_addr_str = ip4addr_ntoa(&cyw43_state.netif[0].ip_addr);

        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 0, 0, 1, "Conectado!");
        ssd1306_draw_string(&display, 0, 8, 1, ip_addr_str);
        ssd1306_show(&display);
    }

    // Inicia o servidor HTTP
    http_server();

    while (true)
    {
        // Calcula tensão, corrente e potência
        tensao = adc_to_tensao(adc_read()); // Calcula a tensão em Volts
        corrente = tensao / sensibilidade;  // Calcula a corrente em Amperes
        watt = tensao_fix * corrente;       // Calcula a potência em Watts

        cyw43_arch_poll();

        sprintf(conv_tensao, "Tensao: %.3f V", tensao);
        sprintf(conv_corrente, "Amperagem: %.3f A", corrente);
        sprintf(conv_watt, "Consumo: %.3f W", watt);

        sprintf(conv_ml_1a_1b, "1a: %i 1b: %i", motor_level_1_a, motor_level_1_b);
        sprintf(conv_ml_2a_2b, "2a: %i 2b: %i", motor_level_2_a, motor_level_2_b);

        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 0, 0, 1, "Conectado!");
        ssd1306_draw_string(&display, 0, 8, 1, ip_addr_str);
        ssd1306_draw_string(&display, 0, 16, 1, conv_tensao);
        ssd1306_draw_string(&display, 0, 24, 1, conv_corrente);
        ssd1306_draw_string(&display, 0, 32, 1, conv_watt);
        ssd1306_draw_string(&display, 0, 40, 1, conv_ml_1a_1b);
        ssd1306_draw_string(&display, 0, 48, 1, conv_ml_2a_2b);
        ssd1306_show(&display);
        sleep_ms(500);
    }
}
