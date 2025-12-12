#include <stdio.h>
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "programa1.h"

// Definição dos pinos usados para o joystick e LEDs
const int VRX = 26;          // Eixo X do joystick
const int VRY = 27;          // Eixo Y do joystick
const int ADC_CHANNEL_0 = 0; // Canal ADC para o eixo X
const int ADC_CHANNEL_1 = 1; // Canal ADC para o eixo Y
const int SW = 22;           // Pino do botão do joystick

const int LED_B = 13;        // Pino para o LED azul via PWM
const int LED_R = 11;        // Pino para o LED vermelho via PWM
static const float DIVIDER_PWM = 16.0;
static const uint16_t PERIOD = 4096;
uint16_t led_b_level = 100, led_r_level = 100;
uint slice_led_b, slice_led_r;

// Função para configurar o ADC e o pino do botão do joystick
void setup_joystick(void)
{
    adc_init();          // Inicializa o ADC
    adc_gpio_init(VRX);  // Configura o pino do eixo X para ADC
    adc_gpio_init(VRY);  // Configura o pino do eixo Y para ADC

    gpio_init(SW);                   // Inicializa o pino do botão
    gpio_set_dir(SW, GPIO_IN);       // Define o pino como entrada
    gpio_pull_up(SW);                // Ativa pull-up para estabilidade
}

// Função para configurar o PWM para um LED (genérica)
void setup_pwm_led(uint led, uint *slice, uint16_t level)
{
    gpio_set_function(led, GPIO_FUNC_PWM);  // Configura o pino para PWM
    *slice = pwm_gpio_to_slice_num(led);      // Obtém o slice do PWM
    pwm_set_clkdiv(*slice, DIVIDER_PWM);       // Configura o divisor do clock
    pwm_set_wrap(*slice, PERIOD);              // Define o período do PWM
    pwm_set_gpio_level(led, level);            // Define o nível inicial do PWM
    pwm_set_enabled(*slice, true);             // Habilita o PWM
}

// Função de configuração específica do programa do joystick
void joystick_setup(void)
{
    stdio_init_all();           // Inicializa a porta serial, se necessário
    setup_joystick();           // Configura o ADC e o botão
    setup_pwm_led(LED_B, &slice_led_b, led_b_level); // Configura o LED azul
    setup_pwm_led(LED_R, &slice_led_r, led_r_level); // Configura o LED vermelho
}

// Função para ler os valores dos eixos do joystick
void joystick_read_axis(uint16_t *vrx_value, uint16_t *vry_value)
{
    // Leitura do eixo X
    adc_select_input(ADC_CHANNEL_0);
    sleep_us(2);
    *vrx_value = adc_read();

    // Leitura do eixo Y
    adc_select_input(ADC_CHANNEL_1);
    sleep_us(2);
    *vry_value = adc_read();
}

// Função que executa o programa do joystick
void joystickProgram(void)
{
    uint16_t vrx_value, vry_value;
    // Configura os periféricos do joystick e LEDs
    joystick_setup();
    printf("Joystick-PWM\n");

    // Loop principal do programa do joystick
    while (1)
    {
        joystick_read_axis(&vrx_value, &vry_value);
        // Ajusta o PWM dos LEDs de acordo com os valores do joystick
        pwm_set_gpio_level(LED_B, vrx_value);
        pwm_set_gpio_level(LED_R, vry_value);
        sleep_ms(100);

        // Exemplo: se o botão for pressionado, saia do programa do joystick
        // (ou use outra condição para retornar ao menu)
        if (gpio_get(SW) == 0)
        {
            // Aguarda o "debounce" e sai do loop
            sleep_ms(300);
            break;
        }
    }
}