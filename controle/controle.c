#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "cJSON.h"

#include "hardware/i2c.h"
#include "ssd1306.h"

#include "hardware/adc.h" // Biblioteca para controle do ADC (Conversor Analógico-Digital).

#define EX 26  // Define o pino 26 como o pino de entrada para o eixo X do joystick (GPI026).
#define EY 27  // Define o pino 27 como o pino de entrada para o eixo Y do joystick (GPI027).

// define a pinagem de saída dos botões
// #define BTN_A_PIN 5
#define BTN_B_PIN 6

#define WIFI_SSID "ssid" // Nome de identificação da sua rede WiFi
#define WIFI_PASSWORD "password" // Senha da sua rede WiFi

#define PICO_DEFAULT_LED_PIN 11

#define SERVER_IP "192.168.137.60" // IP real do seu robô
#define SERVER_PORT 80

// Definir o valor máximo de PWM
const int16_t MAX_PWM = 255;
float linear_x = 0, linear_y = 0, angular_z = 0;
int modo = 0;
// int robo = 0;

char conv_linear_x[20];
char conv_linear_y[20];
char conv_angular_z[20];
char conv_modo[20];
// char conv_robo[20];

int16_t pwm_front_left_wheel = 0;
int16_t pwm_back_left_wheel = 0;
int16_t pwm_front_right_wheel = 0;
int16_t pwm_back_right_wheel = 0;

float tensao = 0;
float corrente = 0;
float potencia = 0;

char conv_tensao[20];
char conv_corrente[20];
char conv_potencia[20];

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display
#define I2C_SDA 14          // Pino SDA
#define I2C_SCL 15          // Pino SCL

// Instância do Display
ssd1306_t display;

struct tcp_pcb *client_pcb;
char request_buffer[512];

// Função para calcular os valores de PWM das rodas de um robô omnidirecional
void calcular_pwm_omni(float linear_x, float linear_y, float angular_z,
                       int16_t *pwm_front_left_wheel, int16_t *pwm_front_right_wheel,
                       int16_t *pwm_back_left_wheel, int16_t *pwm_back_right_wheel)
{

    // Calcular os valores de PWM para cada roda
    *pwm_front_left_wheel = (linear_x + linear_y + angular_z) * MAX_PWM;
    *pwm_front_right_wheel = (linear_x - linear_y - angular_z) * MAX_PWM;
    *pwm_back_left_wheel = (linear_x - linear_y + angular_z) * MAX_PWM;
    *pwm_back_right_wheel = (linear_x + linear_y - angular_z) * MAX_PWM;

    // Garantir que os valores de PWM estejam dentro do intervalo permitido (-255 a 255)
    if (*pwm_front_left_wheel > MAX_PWM)
        *pwm_front_left_wheel = MAX_PWM;
    if (*pwm_front_left_wheel < -MAX_PWM)
        *pwm_front_left_wheel = -MAX_PWM;

    if (*pwm_front_right_wheel > MAX_PWM)
        *pwm_front_right_wheel = MAX_PWM;
    if (*pwm_front_right_wheel < -MAX_PWM)
        *pwm_front_right_wheel = -MAX_PWM;

    if (*pwm_back_left_wheel > MAX_PWM)
        *pwm_back_left_wheel = MAX_PWM;
    if (*pwm_back_left_wheel < -MAX_PWM)
        *pwm_back_left_wheel = -MAX_PWM;

    if (*pwm_back_right_wheel > MAX_PWM)
        *pwm_back_right_wheel = MAX_PWM;
    if (*pwm_back_right_wheel < -MAX_PWM)
        *pwm_back_right_wheel = -MAX_PWM;
}

// Função para processar o JSON recebido e controlar os valores dos sensores lidos
void process_json_request(const char *json_str)
{
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL)
    {
        printf("Erro ao analisar o JSON\n");
        return;
    }

    // Tentar obter valores de pwm
    cJSON *ten = cJSON_GetObjectItem(json, "Tensao");
    cJSON *cor = cJSON_GetObjectItem(json, "Corrente");
    cJSON *pot = cJSON_GetObjectItem(json, "Potencia");

    if (cJSON_IsNumber(ten) && cJSON_IsNumber(cor) && cJSON_IsNumber(pot))
    {
        // Controlar o pwm com os valores recebidos

        tensao = ten->valuedouble;
        corrente = cor->valuedouble;
        potencia = pot->valuedouble;
    }
    else
    {
        printf("Valores dos sensores não encontrados ou inválidos no JSON\n");
    }

    cJSON_Delete(json); // Liberar memória
}



// Callback de recebimento de resposta http
err_t http_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
    if (p != NULL)
    {
        // Copiar os dados para um buffer legível
        char buffer[2048];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, (char *)p->payload, p->len);
        buffer[p->len] = '\0'; // Garantir terminação nula
        printf("Resposta do servidor:\n%.*s\n", buffer);

        // Procurar onde começa o corpo do JSON
        char *json_start = strstr(buffer, "\r\n\r\n");
        if (json_start != NULL)
        {
            json_start += 4; // Pular o cabeçalho HTTP

            printf("Corpo da requisição: %s\n", json_start);

            // Processa o JSON
            process_json_request(json_start);
        }

        pbuf_free(p); // Libera o buffer recebido
    }

    printf("Encerrando conexão TCP...\n");
    tcp_close(pcb); // Fecha a conexão corretamente
    return ERR_OK;
}

// Callback de conexão bem-sucedida
err_t http_connected_callback(void *arg, struct tcp_pcb *pcb, err_t err)
{
    if (err != ERR_OK)
    {
        printf("Erro ao conectar ao servidor: %d\n", err);
        return err;
    }

    // printf("Conectado ao servidor!\n");

    // Envia a requisição armazenada no buffer
    tcp_write(pcb, request_buffer, strlen(request_buffer), TCP_WRITE_FLAG_COPY);
    tcp_output(pcb);

    // Registrar callback de recebimento
    tcp_recv(pcb, http_recv_callback);

    return ERR_OK;
}

// Enviar requisição GET
void send_http_get()
{
    snprintf(request_buffer, sizeof(request_buffer),
             "GET /sensores HTTP/1.1\r\n"
             "Host: " SERVER_IP "\r\n"
             "Connection: close\r\n\r\n");

    ip_addr_t server_addr;
    ip4addr_aton(SERVER_IP, &server_addr);

    client_pcb = tcp_new();
    if (!client_pcb)
    {
        printf("Erro ao criar PCB\n");
        return;
    }

    tcp_connect(client_pcb, &server_addr, SERVER_PORT, http_connected_callback);
}

// Enviar requisição PUT
void send_http_put()
{
    cJSON *json = cJSON_CreateObject();
    calcular_pwm_omni(linear_x, linear_y, angular_z, &pwm_front_left_wheel, &pwm_front_right_wheel, &pwm_back_left_wheel, &pwm_back_right_wheel);
    cJSON_AddNumberToObject(json, "ml_1a", pwm_front_right_wheel);
    cJSON_AddNumberToObject(json, "ml_1b", pwm_front_left_wheel);
    cJSON_AddNumberToObject(json, "ml_2a", pwm_back_right_wheel);
    cJSON_AddNumberToObject(json, "ml_2b", pwm_back_left_wheel);
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);

    snprintf(request_buffer, sizeof(request_buffer),
             "PUT /atuadores HTTP/1.1\r\n"
             "Host: " SERVER_IP "\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n\r\n"
             "%s",
             (int)strlen(json_str), json_str);

    ip_addr_t server_addr;
    ip4addr_aton(SERVER_IP, &server_addr);

    client_pcb = tcp_new();
    if (!client_pcb)
    {
        printf("Erro ao criar PCB\n");
        return;
    }

    tcp_connect(client_pcb, &server_addr, SERVER_PORT, http_connected_callback);
}

float converterValor(float input)
{
    float min_input = 25;
    float max_input = 4080;
    float range = 200;

    // Verifica se o input está dentro dos limites
    if (input < min_input)
        input = min_input;
    if (input > max_input)
        input = max_input;

    if (input >= 1800 && input <= 2200)
        return 0.0;

    // Calcula o valor normalizado
    float output = -1.0 + 2.0 * (input - min_input) / (max_input - min_input);

    // Arredonda para os limites do range de coleta
    float steps = (max_input - min_input) / range;
    output = ((float)(output * steps + 0.5)) / (float)steps;

    return output;
}

int main()
{
    stdio_init_all();
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // Inicializa I2C no canal 1
    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    adc_init(); // Inicializa o ADC para realizar leituras analógicas.

    adc_gpio_init(EX); // Inicializa o pino EX (GPI026) para ser usado como entrada analógica para o eixo X do joystick.
    adc_gpio_init(EY); // Inicializa o pino EY (GPI027) para ser usado como entrada analógica para o eixo Y do joystick.

    gpio_init(11);              // Inicializa o pino 11 como um pino de controle (para indicar direções no joystick).
    gpio_set_dir(11, GPIO_OUT); // Configura o pino 11 como saída.

    gpio_init(13);              // Inicializa o pino 13 como um pino de controle (para indicar direções no joystick).
    gpio_set_dir(13, GPIO_OUT); // Configura o pino 13 como saída.

    // Inicializando o botão A
    // gpio_init(BTN_A_PIN);
    // gpio_set_dir(BTN_A_PIN, GPIO_IN);
    // gpio_pull_up(BTN_A_PIN); // default 1

    // inicializando o botão B
    gpio_init(BTN_B_PIN);
    gpio_set_dir(BTN_B_PIN, GPIO_IN);
    gpio_pull_up(BTN_B_PIN); //

    // Inicializa o display
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1))
    {
        printf("Falha ao inicializar o display SSD1306\n");
        return 1; // Sai do programa
    }

    printf("Display SSD1306 inicializado com sucesso!\n");

    // Inicializa o chip WiFi
    if (cyw43_arch_init())
    {
        printf("Falha ao iniciar WiFi\n");
        return -1;
    }

    // Habilita o WiFi
    cyw43_arch_enable_sta_mode();

    printf("Conectanto ao WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao WiFi\n");
        return 1;
    }
    else
    {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        printf("Conectado ao WiFi\n");
        sleep_ms(1000);

        while (true)
        {
            send_http_get();
            // // Retornando uma resposta em caso de pressionar o botão A
            // if (gpio_get(BTN_A_PIN) == 0)
            // {
            //     robo = (robo == 0 ? 1 : 0);
            // }

            // Retornando uma resposta em caso de pressionar o botão B
            if (gpio_get(BTN_B_PIN) == 0)
            {
                modo = (modo == 0 ? 1 : 0);
            }

            // Eixo x
            adc_select_input(0);         // Seleciona o canal de entrada do ADC 0 (pino GPI026) para ler o valor analógico do eixo X do joystick.
            uint adc_x_raw = adc_read(); // Lê o valor analógico do eixo X (GPI026) e armazena na variável adc_y_raw.
            linear_x = converterValor(adc_x_raw);

            // Eixo y
            adc_select_input(1);         // Seleciona o canal de entrada do ADC 1 (pino GPI027) para ler o valor analógico do eixo Y do joystick.
            uint adc_y_raw = adc_read(); // Lê o valor analógico do eixo Y (GPI027) e armazena na variável adc_x_raw.

            if (modo == 0){
                angular_z = converterValor(adc_y_raw);
                linear_y = 0.0;
            }
            else {
                linear_y = converterValor(adc_y_raw);
                angular_z = 0.0;
            }

            cyw43_arch_poll();

            sprintf(conv_tensao, "Tensao: %.3f V", tensao);
            sprintf(conv_corrente, "Amperagem: %.3f A", corrente);
            sprintf(conv_potencia, "Consumo: %.3f W", potencia);

            // sprintf(conv_robo, "Robo: %s", (robo == 0 ? "Roomba" : "Omni"));
            sprintf(conv_modo, "Modo: %s", (modo == 0 ? "Normal" : "Holonomico"));
            sprintf(conv_linear_x, "Linear X: %.1f", linear_x);
            sprintf(conv_linear_y, "Linear Y: %.1f", linear_y);
            sprintf(conv_angular_z, "Angular Z: %.1f", angular_z);

            ssd1306_clear(&display);
            ssd1306_draw_string(&display, 0, 0, 1, conv_tensao);
            ssd1306_draw_string(&display, 0, 8, 1, conv_corrente);
            ssd1306_draw_string(&display, 0, 16, 1, conv_potencia);

            // ssd1306_draw_string(&display, 0, 24, 1, conv_robo);
            ssd1306_draw_string(&display, 0, 32, 1, conv_modo);
            ssd1306_draw_string(&display, 0, 40, 1, conv_linear_x);
            ssd1306_draw_string(&display, 0, 48, 1, (modo == 0 ? conv_angular_z : conv_linear_y));
            ssd1306_show(&display);
            send_http_put();
            sleep_ms(750);
        }
    }
}