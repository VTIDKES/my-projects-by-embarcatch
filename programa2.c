// programa2.c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "programa2.h"

#define BUZZER_PIN 21
#define SW 22  // Pino do botão do joystick (usado para interromper)

// Notas musicais para o tema de Star Wars
const uint star_wars_notes[] = {
    330, 330, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 262, 392, 523, 330, 262,
    392, 523, 330, 659, 659, 659, 698, 523,
    415, 349, 330, 523, 494, 440, 392, 330,
    659, 784, 659, 523, 494, 440, 392, 330,
    659, 659, 330, 784, 880, 698, 784, 659,
    523, 494, 440, 392, 659, 784, 659, 523,
    494, 440, 392, 330, 659, 523, 659, 262,
    330, 294, 247, 262, 220, 262, 330, 262,
    330, 294, 247, 262, 330, 392, 523, 440,
    349, 330, 659, 784, 659, 523, 494, 440,
    392, 659, 784, 659, 523, 494, 440, 392
};

// Duração das notas (ms)
const uint note_duration[] = {
    500, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 650, 500, 150, 300, 500, 350,
    150, 300, 500, 150, 300, 500, 350, 150,
    300, 650, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 500, 500, 500, 350, 150,
    300, 500, 500, 350, 150, 300, 500, 350,
    150, 300, 500, 350, 150, 300, 500, 500,
    350, 150, 300, 500, 500, 350, 150, 300,
};

// Inicializa o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f); // Ajusta o divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Inicialmente sem som
}

// Toca uma nota verificando periodicamente o botão.
// Retorna 1 se a execução for interrompida; 0 caso contrário.
int play_tone(uint pin, uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;
    
    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2); // 50% do duty cycle

    // Divide a duração em passos de 10 ms para checar o botão
    uint elapsed = 0;
    const uint step = 10;
    while (elapsed < duration_ms) {
        if (gpio_get(SW) == 0) {  // Se o botão for pressionado
            pwm_set_gpio_level(pin, 0);
            sleep_ms(50); // debounce
            return 1;     // Interrompe a nota
        }
        sleep_ms(step);
        elapsed += step;
    }
    
    pwm_set_gpio_level(pin, 0); // Desliga a nota
    sleep_ms(50);               // Pausa entre notas
    return 0;
}

// Toca o tema de Star Wars; retorna 1 se for interrompido, 0 se executado normalmente.
int play_star_wars(uint pin) {
    int total_notes = sizeof(star_wars_notes) / sizeof(star_wars_notes[0]);
    for (int i = 0; i < total_notes; i++) {
        // Verifica antes de cada nota se o botão foi pressionado
        if (gpio_get(SW) == 0) {
            return 1;
        }
        if (star_wars_notes[i] == 0) {
            sleep_ms(note_duration[i]);
        } else {
            int interrompido = play_tone(pin, star_wars_notes[i], note_duration[i]);
            if (interrompido) {
                return 1;
            }
        }
    }
    return 0;
}

// Função que executa o programa do Buzzer (toca a música e retorna ao menu se o botão for pressionado)
void buzzerProgram(void) {
    // Inicializa o PWM do buzzer
    pwm_init_buzzer(BUZZER_PIN);
    printf("Buzzer Program: Tocando o tema de Star Wars...\n");
    
    // Toca a música; se o botão for pressionado durante a execução, a função retorna 1.
    int interrompido = play_star_wars(BUZZER_PIN);
    if (interrompido) {
        printf("Música interrompida pelo botão.\n");
    }
    sleep_ms(300); // Pequeno delay antes de retornar ao menu
}
