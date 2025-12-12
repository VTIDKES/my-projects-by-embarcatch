#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "programa3.h"

#define LED 12      // Pino do LED (pode ser o canal de um LED RGB)
#define SW 22       // Pino do botão do joystick (usado para interromper o programa)

static const uint16_t PERIOD = 2000;     // Período do PWM (valor máximo do contador)
static const float DIVIDER_PWM = 16.0;     // Divisor do clock para o PWM
const uint16_t LED_STEP = 100;             // Passo de incremento/decremento para o duty cycle do LED
static uint16_t led_level = 100;           // Nível inicial do PWM (duty cycle)

// Função que verifica periodicamente se o botão foi pressionado durante um atraso.
// Retorna true se o botão for pressionado.
static bool checkButtonDuringDelay(uint delay_ms) {
    const uint step = 50;  // Intervalo de verificação (50 ms)
    uint elapsed = 0;
    while (elapsed < delay_ms) {
        if (gpio_get(SW) == 0) {
            sleep_ms(50); // debounce
            if (gpio_get(SW) == 0) {
                return true; // Botão pressionado
            }
        }
        sleep_ms(step);
        elapsed += step;
    }
    return false;
}

// Configura o PWM para o LED
static void setup_pwm() {
    uint slice = pwm_gpio_to_slice_num(LED);
    gpio_set_function(LED, GPIO_FUNC_PWM);
    pwm_set_clkdiv(slice, DIVIDER_PWM);
    pwm_set_wrap(slice, PERIOD);
    pwm_set_gpio_level(LED, led_level);
    pwm_set_enabled(slice, true);
}

// Função que executa o programa do LED RGB.
// Se o botão (SW) for pressionado, o programa é interrompido e retorna ao menu.
void ledRgbProgram(void) {
    printf("LED RGB Program started.\n");
    setup_pwm();
    uint up_down = 1;  // Controle para aumentar ou diminuir o duty cycle

    while (true) {
        // Atualiza o nível de PWM para o LED
        pwm_set_gpio_level(LED, led_level);

        // Verifica imediatamente se o botão foi pressionado
        if (gpio_get(SW) == 0) {
            sleep_ms(50); // debounce
            if (gpio_get(SW) == 0) {
                printf("LED RGB Program interrupted.\n");
                break;
            }
        }

        // Em vez de um atraso único de 1000 ms, dividimos em intervalos curtos
        if (checkButtonDuringDelay(1000)) {
            printf("LED RGB Program interrupted during delay.\n");
            break;
        }

        // Atualiza o nível do LED conforme a direção
        if (up_down) {
            led_level += LED_STEP;
            if (led_level >= PERIOD)
                up_down = 0;
        } else {
            led_level -= LED_STEP;
            if (led_level <= LED_STEP)
                up_down = 1;
        }
    }
}
