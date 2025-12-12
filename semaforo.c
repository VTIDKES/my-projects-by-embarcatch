#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// Definindo os pinos dos LEDs, buzzer e botão
#define RED_LED 2
#define YELLOW_LED 3
#define GREEN_LED 4
#define PEDESTRIAN_LED 5
#define BUZZER_PIN 6
#define BUTTON_PIN 7
#define BUZZER_FREQUENCY_LOW 100
#define BUZZER_FREQUENCY_HIGH 320

// Declaração das funções
void start();
void start_pin(uint pin, uint config);
void run();
void light_up_red();
void light_up_yellow();
void light_up_green();
uint default_traffic(uint count);
void alternative_traffic();
void pwm_init_buzzer(uint pin);
void beep(uint pin, uint duration_ms);

// Função principal que chama a inicialização e o loop principal
int main() {
  start();
  run();
}

// Função para iniciar e configurar os pinos
void start(){
  start_pin(RED_LED, GPIO_OUT);
  start_pin(YELLOW_LED, GPIO_OUT);
  start_pin(GREEN_LED, GPIO_OUT);
  start_pin(PEDESTRIAN_LED, GPIO_OUT);

  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_PIN);

  gpio_init(BUZZER_PIN);
  gpio_set_dir(BUZZER_PIN, GPIO_OUT);

  pwm_init_buzzer(BUZZER_PIN);
}

// Inicializa e configura um pino específico
void start_pin(uint pin, uint config){
  gpio_init(pin);
  gpio_set_dir(pin, config);
  gpio_put(pin, 0);
}

// Função principal do programa que controla o loop
void run(){
  uint count = 0;
  bool button_pressed = false;

  while(true){
    // Verifica se o botão foi acionado
    if(gpio_get(BUTTON_PIN) == 0){
      button_pressed = true;
    }

    // Tráfego alternativo se o botão foi pressionado
    if(button_pressed){
      alternative_traffic();
      count = 0;
      button_pressed = false;
    }

    // Mantém o tráfego padrão se o botão não foi acionado
    if(!button_pressed){
      count = default_traffic(count);
    }
  }
}

// Tráfego alternativo para pedestres
void alternative_traffic(){
  light_up_yellow();
  sleep_ms(5000);
  light_up_red();
  gpio_put(PEDESTRIAN_LED, 1); 
  beep(BUZZER_PIN, 15000);
  gpio_put(PEDESTRIAN_LED, 0);
}

// Tráfego padrão dos sinais dos carros
uint default_traffic(uint count){
  const uint DEFAULT_TIME_GREEN_LED = 8000;
  const uint DEFAULT_TIME_YELLOW_LED = 2000;
  const uint DEFAULT_TIME_RED_LED = 10000;

  if(count < DEFAULT_TIME_GREEN_LED){
    light_up_green();
    count++;
    sleep_ms(1);
  } else if(count < DEFAULT_TIME_GREEN_LED + DEFAULT_TIME_YELLOW_LED){
    light_up_yellow();
    count++;
    sleep_ms(1);
  } else if(count < DEFAULT_TIME_GREEN_LED + DEFAULT_TIME_YELLOW_LED + DEFAULT_TIME_RED_LED){
    light_up_red();
    count++;
    sleep_ms(1);
  } else {
    count = 0;
  }
  
  return count;
}

// Acende o LED vermelho e apaga os outros
void light_up_red(){
  gpio_put(YELLOW_LED, 0);
  gpio_put(RED_LED, 1);
  gpio_put(GREEN_LED, 0);
}

// Acende o LED amarelo e apaga os outros
void light_up_yellow(){
  gpio_put(YELLOW_LED, 1);
  gpio_put(RED_LED, 0);
  gpio_put(GREEN_LED, 0);
}

// Acende o LED verde e apaga os outros
void light_up_green(){
  gpio_put(YELLOW_LED, 0);
  gpio_put(RED_LED, 0);
  gpio_put(GREEN_LED, 1);
}

// Configuração do buzzer
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0);
}

// Emite um beep com duração especificada e frequência intermitente
void beep(uint pin, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t freq = BUZZER_FREQUENCY_LOW;
    uint step_duration = 100; // Duração de cada passo em ms

    for (uint elapsed = 0; elapsed < duration_ms; elapsed += step_duration) {
        // Alterna a frequência entre 100 Hz e 320 Hz
        freq = (freq == BUZZER_FREQUENCY_LOW) ? BUZZER_FREQUENCY_HIGH : BUZZER_FREQUENCY_LOW;

        // Configura o divisor de clock para a nova frequência
        pwm_set_clkdiv(slice_num, clock_get_hz(clk_sys) / (freq * 4096));

        // Emite o som com a nova frequência
        pwm_set_gpio_level(pin, 2048);
        sleep_ms(step_duration);
    }

    // Desativa o sinal PWM
    pwm_set_gpio_level(pin, 0);
}


