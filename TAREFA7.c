// Bibliotecas necessárias
#include "pico/stdlib.h"          // Biblioteca padrão do Pico
#include "hardware/adc.h"         // Biblioteca para ADC (Conversor Analógico-Digital)
#include "hardware/i2c.h"         // Biblioteca para comunicação I2C
#include "libs/ssd1306.h"         // Biblioteca para controle do display OLED SSD1306
#include "pico/cyw43_arch.h"      // Biblioteca para controle do hardware Wi-Fi CYW43
#include "lwip/pbuf.h"            // Biblioteca LWIP para buffers de rede
#include "lwip/tcp.h"             // Biblioteca LWIP para protocolo TCP
#include "lwip/dns.h"             // Biblioteca LWIP para resolução DNS

// Definições de Hardware
#define RED_LED_PIN 11            // Pino GPIO do LED vermelho
#define GREEN_LED_PIN 13           // Pino GPIO do LED verde
#define MICROFONE_PIN 28           // Pino do microfone (Canal ADC2)
#define VOICE_THRESHOLD 30         // Limite para detecção de voz
#define LOUD_THRESHOLD 3750        // Limite para detecção de som alto
#define SAMPLE_COUNT 100           // Número de amostras para leitura
#define I2C_DISPLAY i2c1           // Interface I2C para o display
#define SDA_PIN 14                 // Pino SDA do I2C
#define SCL_PIN 15                 // Pino SCL do I2C

// Configurações do Wi-Fi
#define WIFI_SSID "intelbras"      // Nome da rede Wi-Fi
#define WIFI_PASS "87125479"       // Senha da rede Wi-Fi
#define THINGSPEAK_HOST "api.thingspeak.com"  // Servidor ThingSpeak
#define THINGSPEAK_PORT 80         // Porta HTTP do ThingSpeak
#define API_KEY "UDNCLX7JPX0693CI"  // Chave da API ThingSpeak
#define SEND_INTERVAL_MS 15000      // Intervalo de envio de dados (15s)

// Variáveis Globais
ssd1306_t display;                 // Estrutura do display OLED
uint16_t baseline_noise = 0;       // Nível base de ruído ambiente
uint16_t max_baseline = 0;         // Valor máximo do ruído base
static absolute_time_t last_send_time; // Último tempo de envio

// Protótipos de Funções
void send_data_to_thingspeak(uint16_t peak);  // Envia dados para o ThingSpeak
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *arg); // Callback de DNS
static err_t tcp_connected(void *arg, struct tcp_pcb *tpcb, err_t err); // Callback de conexão TCP
static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len); // Callback de envio TCP

// Inicialização do Hardware
void init_hardware() {
    gpio_init(RED_LED_PIN);        // Inicializa pino do LED vermelho
    gpio_set_dir(RED_LED_PIN, GPIO_OUT); // Configura como saída
    gpio_init(GREEN_LED_PIN);      // Inicializa pino do LED verde
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT); // Configura como saída

    adc_init();                    // Inicializa o ADC
    adc_gpio_init(MICROFONE_PIN);  // Configura pino do microfone para ADC
    adc_select_input(2);           // Seleciona canal ADC2

    // Configuração do I2C para o display
    i2c_init(I2C_DISPLAY, 400000); // Inicializa I2C a 400kHz
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C); // Configura pino SDA
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C); // Configura pino SCL
    gpio_pull_up(SDA_PIN);         // Habilita pull-up no SDA
    gpio_pull_up(SCL_PIN);         // Habilita pull-up no SCL

    // Inicialização do display OLED
    ssd1306_init(&display, 128, 64, 0x3C, I2C_DISPLAY); 
    ssd1306_clear(&display);       // Limpa o display

    // Cálculo do ruído ambiente base
    uint32_t sum = 0;
    uint16_t max_val = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        uint16_t val = adc_read(); // Lê valor do ADC
        sum += val;                // Acumula para média
        if(val > max_val) max_val = val; // Atualiza máximo
        sleep_ms(10);              // Intervalo entre amostras
    }
    baseline_noise = sum / SAMPLE_COUNT; // Calcula média
    max_baseline = max_val + 50;   // Define limite máximo base

    // Mensagem inicial no display
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 43, 28, 1, "PRONTO!");
    ssd1306_show(&display);
    sleep_ms(1000);
    ssd1306_clear(&display);
}

// Atualização do Display
void update_display(bool loud, bool voice) {
    ssd1306_clear(&display);       // Limpa o buffer do display
    
    if(loud) {                     // Se detectar som alto
        ssd1306_draw_string(&display, 43, 20, 1, "PERIGO!");
        ssd1306_draw_string(&display, 13, 30, 1, "RUIDO ALTO!CUIDADO!");
    } else if(voice) {            // Se detectar voz normal
        ssd1306_draw_string(&display, 34, 30, 1, "SOM NORMAL");
    } else {                       // Ambiente silencioso
        ssd1306_draw_string(&display, 31, 30, 1, "Ambiente OK");
    }
    ssd1306_show(&display);        // Atualiza o display físico
}

// Callback de DNS
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *arg) {
    struct tcp_pcb *tpcb = (struct tcp_pcb *)arg;
    if (ipaddr) {
        printf("Conectando a %s\n", ipaddr_ntoa(ipaddr));
        tcp_connect(tpcb, ipaddr, THINGSPEAK_PORT, tcp_connected); // Inicia conexão TCP
    } else {
        printf("DNS falhou\n");
        tcp_close(tpcb);           // Fecha conexão em caso de falha
    }
}

// Callback de Conexão TCP Estabelecida
static err_t tcp_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK) {
        printf("Conexão falhou: %d\n", err);
        tcp_close(tpcb);           // Fecha conexão se houver erro
        return ERR_OK;
    }

    uint16_t peak = (uintptr_t)arg; // Recupera o valor do pico
    // Cria requisição HTTP GET
    char request[256];
    snprintf(request, sizeof(request),
        "GET /update?api_key=%s&field1=%d HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n",
        API_KEY, peak, THINGSPEAK_HOST
    );

    // Configura callback e envia requisição
    tcp_sent(tpcb, tcp_sent_callback);
    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);              // Força transmissão imediata
    return ERR_OK;
}

// Callback de Dados Enviados
static err_t tcp_sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    printf("Dados enviados!\n");
    tcp_close(tpcb);               // Fecha conexão após envio
    return ERR_OK;
}

// Função de Envio de Dados para o ThingSpeak
void send_data_to_thingspeak(uint16_t peak) {
    static absolute_time_t last_send = 0;
    // Verifica intervalo mínimo entre envios
    if (absolute_time_diff_us(last_send, get_absolute_time()) < SEND_INTERVAL_MS * 1000) {
        return;
    }
    last_send = get_absolute_time();

    // Cria nova estrutura TCP
    struct tcp_pcb *tpcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!tpcb) {
        printf("Falha ao criar PCB\n");
        return;
    }

    // Configura argumentos e inicia resolução DNS
    tpcb->callback_arg = (void *)(uintptr_t)peak;
    ip_addr_t ip_addr;
    err_t err = dns_gethostbyname(THINGSPEAK_HOST, &ip_addr, dns_callback, tpcb);
    if (err == ERR_INPROGRESS) {
        printf("Resolvendo DNS...\n");
    } else if (err != ERR_OK) {
        printf("Erro DNS: %d\n", err);
        tcp_close(tpcb);
    }
}

// Função Principal
int main() {
    stdio_init_all();              // Inicializa todas as entradas/saídas padrão
    init_hardware();               // Configura hardware

    // Inicialização do Wi-Fi
    if (cyw43_arch_init()) {       // Inicializa driver Wi-Fi
        printf("Falha ao inicializar Wi-Fi\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();  // Modo estação (cliente)
    printf("Conectando ao Wi-Fi...\n");
    // Tenta conexão por 30 segundos
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Conexão falhou\n");
        return 1;
    }
    printf("Conectado!\n");
    gpio_put(GREEN_LED_PIN, 1);    // Acende LED verde

    while(1) {                     // Loop principal
        uint16_t peak = 0;         // Pico de som detectado
        uint32_t sum_dev = 0;      // Soma das variações

        // Coleta 100 amostras do microfone
        for(int i = 0; i < SAMPLE_COUNT; i++) {
            uint16_t val = adc_read(); 
            if(val > peak) peak = val;             // Atualiza pico
            sum_dev += abs(val - baseline_noise);  // Calcula desvio
            sleep_us(100);            // Intervalo entre amostras
        }

        // Determina estados de detecção
        bool voice_detected = (sum_dev/SAMPLE_COUNT) > VOICE_THRESHOLD;
        bool loud_detected = peak > LOUD_THRESHOLD && peak > max_baseline;

        // Controle dos LEDs
        gpio_put(RED_LED_PIN, !loud_detected);    // Vermelho para alerta
        gpio_put(GREEN_LED_PIN, loud_detected);   // Verde para normal

        update_display(loud_detected, voice_detected); // Atualiza display
        send_data_to_thingspeak(peak);             // Envia dados

        cyw43_arch_poll();         // Mantém conexão Wi-Fi ativa
        sleep_ms(100);             // Intervalo entre ciclos
    }
}