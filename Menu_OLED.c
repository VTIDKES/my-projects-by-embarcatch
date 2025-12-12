#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "hardware/adc.h"

// Inclusão dos módulos dos programas
#include "programa1.h"   // Módulo do Joystick (Prog 1)
#include "programa2.h"   // Módulo do Buzzer (Prog 2)
#include "programa3.h" 
#define I2C_PORT i2c1
#define I2C_SDA 15
#define I2C_SCL 14

#define SW 22    // Botão do Joystick (usado para seleção e para interromper os programas)
#define VRY 26   // ADC do eixo vertical do Joystick

// Instância do display OLED
ssd1306_t disp;

// Inicializa o display OLED via I2C
void init_display() {
    stdio_init_all();
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);
    ssd1306_clear(&disp);
}

// Função auxiliar para desenhar uma string no display
void display_text(int x, int y, int scale, const char *text) {
    ssd1306_draw_string(&disp, x, y, scale, (char *)text);
    ssd1306_show(&disp);
}

// Função auxiliar para desenhar um retângulo vazio
void display_rect(int x1, int y1, int x2, int y2) {
    ssd1306_draw_empty_square(&disp, x1, y1, x2, y2);
    ssd1306_show(&disp);
}

// Exibe o menu no OLED com a opção selecionada destacada
void print_menu(uint8_t sel) {
    ssd1306_clear(&disp);
    
    // Cabeçalho do menu
    display_text(52, 2, 1, "MENU");
    
    // Opções do menu
    display_text(6, 18, 1.5, "1. Joystick LED");
    display_text(6, 30, 1.5, "2. Buzzer");
    display_text(6, 42, 1.5, "3. LED RGB");
    
    // Define a posição vertical para o retângulo da opção selecionada
    int rect_y;
    if (sel == 1)
        rect_y = 16;
    else if (sel == 2)
        rect_y = 28;
    else if(sel == 3)
        rect_y = 40;
    
    // Desenha um retângulo menor para destacar a opção selecionada
    // Neste exemplo, o retângulo tem altura de 8 pixels (de rect_y a rect_y+8)
    display_rect(2,rect_y+2,120,12);
}

int main() {
    // Inicializações do display, ADC e botão
    init_display();
    
    // Configuração do pino do botão (SW)
    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);
    
    // Inicializa o ADC para o joystick (pino VRY)
    adc_init();
    adc_gpio_init(VRY);
    
    // Variável para o item selecionado do menu (1 a 3)
    uint8_t menu_sel = 1;
    print_menu(menu_sel);
    
    while (true) {
        // Leitura do ADC (canal 0) para navegação vertical
        adc_select_input(0);
        uint16_t adc_val = adc_read();
        
        // Invertendo a lógica:
        // Se o valor lido for menor (joystick movido para baixo), incrementa a seleção.
        if (adc_val < 1500) {
            if (menu_sel < 3) {
                menu_sel++;
                print_menu(menu_sel);
                sleep_ms(200); // Delay para debounce
            }
        }
        // Se o valor lido for maior (joystick movido para cima), decrementa a seleção.
        else if (adc_val > 2500) {
            if (menu_sel > 1) {
                menu_sel--;
                print_menu(menu_sel);
                sleep_ms(200);
            }
        }
        
        // Se o botão for pressionado, seleciona a opção atual
        if (gpio_get(SW) == 0) {
            sleep_ms(200); // Debounce
            switch (menu_sel) {
                case 1:
                    // Executa o programa do Joystick (Prog 1)
                    joystickProgram();
                    break;
                case 2:
                    // Executa o programa do Buzzer (Prog 2)
                    buzzerProgram();
                    break;
                case 3:
                    ledRgbProgram();
                    break;
                default:
                    break;
            }
            // Após a execução, reexibe o menu
            print_menu(menu_sel);
        }
        sleep_ms(50);
    }
    
    return 0;
}
