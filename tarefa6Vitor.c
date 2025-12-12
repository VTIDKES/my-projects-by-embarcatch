#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"

// Pinos
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;
const uint BUTTON_PIN_A = 5;
const uint BUTTON_PIN_B = 6;
const uint LED_VERDE = 11;
const uint LED_VERMELHO = 13;

// Área de renderização
struct render_area frame_area = {
    start_column : 0,
    end_column : ssd1306_width - 1,
    start_page : 0,
    end_page : ssd1306_n_pages - 1
};

uint8_t ssd[ssd1306_buffer_length];

// Declaração das funções
void LimpaDisplay(void);
void mensagemDisplay(const char *text[], int lines);
void SinalAberto(void);
void Avisobotao(void);
void SinalFechado(void);
int WaitWithRead(int seg);


void LimpaDisplay(void) {
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
};

void mensagemDisplay(const char *text[], int lines) {
    int y = 0;
    for (int i = 0; i < lines; i++) {
        ssd1306_draw_string(ssd, 5, y, (char *)text[i]);
        y += 8; // Próxima linha
    }
    render_on_display(ssd, &frame_area);
};

void SinalAberto(void) {
    gpio_put(LED_VERDE, 1);
    gpio_put(LED_VERMELHO, 0);
    LimpaDisplay();
    const char *text[] = {
        "SINAL ABERTO",
        "     ",
        "ATREVESSAR",
        "      ",
        "COM CUIDADO"
    };
    mensagemDisplay(text, 5);
};

void Avisobotao(void) {
    gpio_put(LED_VERDE, 1);
    gpio_put(LED_VERMELHO, 1);
    LimpaDisplay();
    const char *text[] = {
        "BOTAO",
        "     ",
        "PRESSIONADO",
        "     ",
        "ABRINDO O SINAL",
        "     ",
        "EM 5 SEG"
    };
    mensagemDisplay(text, 7);
};

void SinalFechado(void) {
    gpio_put(LED_VERDE, 0);
    gpio_put(LED_VERMELHO, 1);
    LimpaDisplay();
    const char *text[] = {
        "SINAL FECHADO",
        "     ",
        "AGUARDE"
    };
    mensagemDisplay(text, 3);
};

int WaitWithRead(int seg) {
    for (int i = 0; i < seg * 10; i++) {
        if (gpio_get(BUTTON_PIN_A) == 0 || gpio_get(BUTTON_PIN_B) == 0) {
            return 1;
        }
        sleep_ms(100);
    }
    return 0;
};

int main() {
    stdio_init_all();

    // Configuração dos botões
    gpio_init(BUTTON_PIN_A);
    gpio_set_dir(BUTTON_PIN_A, GPIO_IN);
    gpio_pull_up(BUTTON_PIN_A);

    gpio_init(BUTTON_PIN_B);
    gpio_set_dir(BUTTON_PIN_B, GPIO_IN);
    gpio_pull_up(BUTTON_PIN_B);

    // Configuração dos LEDs
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);

    // Configuração do I2C
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização do display
    ssd1306_init();
    calculate_render_area_buffer_length(&frame_area);

    // Loop principal
    while (true) {
        SinalFechado();
        if (WaitWithRead(10)) {
            Avisobotao();
            sleep_ms(5000);
            SinalAberto();
            sleep_ms(15000);
        } else {
            SinalAberto();
            sleep_ms(10000);
        }
    };

    return 0;
}
