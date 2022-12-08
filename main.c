// librerias c
#include <stdio.h>
#include <string.h>
// librerias esp32
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "esp_heap_caps.h"

// pin definitions
const int lims_pin[2] = {32, 25};
#define lim_pin_ro 32
#define lim_pin_alpha 25
const int ena_pin[2] = {-1, -1};
const int dir_pin[2] = {26, 12};
const int pul_pin[2] = {27, 13};
const int iman_pin = 33;
// motor id
#define ALPHA 1
#define RO 0
// direcciones
#define CW 1
#define CCW 0

// Conversion cm a steps
int cm_to_steps(int cm)
{
    return cm * 100;
}
// Conversion grados a steps
int degree_to_steps(int alpha)
{
    return (alpha * (800 / 360)) + 37;
}

// pin bit mask
#define gpio_input_mask (1ULL << lims_pin[ALPHA]) | (1ULL << lims_pin[RO])
#define gpio_output_mask (1ULL << dir_pin[ALPHA]) | (1ULL << pul_pin[ALPHA]) | (1ULL << dir_pin[RO]) | (1ULL << pul_pin[RO]) | (1ULL << iman_pin)

/* inicializacion del hardware
* modo output
    - no interrupciones
    - no pulldown
    - no pullup
* modo input
    - no interrupciones
    - pulldown
    - no pullup
*/
void init_hw(void)
{
    gpio_config_t io_config;

    io_config.pin_bit_mask = gpio_output_mask;
    io_config.mode = GPIO_MODE_OUTPUT;
    io_config.intr_type = GPIO_INTR_DISABLE;
    io_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_config.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_config);

    io_config.pin_bit_mask = gpio_input_mask;
    io_config.mode = GPIO_MODE_INPUT;
    io_config.intr_type = GPIO_INTR_DISABLE;
    io_config.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_config.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_config);

    for (int i = 0; i < 2; i++)
    {
        gpio_set_level(dir_pin[i], 0);
        gpio_set_level(pul_pin[i], 0);
    }
    gpio_set_level(iman_pin, 1);
}

const uart_port_t uart_num = UART_NUM_0;
#define uart_buffer_size (1024 * 2)
QueueHandle_t uart_queue;
void init_uart(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, -1, -1, -1, -1);
    uart_driver_install(uart_num, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0);
}

#define period_ro 1
#define period_alpha 3

int *i;
/* funcion de operacion del stepper
 * steps = cantidad de pulsos del reloj = cantidad de pasos del motor,
 * dir = direccion de giro
 * motor = id del motor
 */
void steps(int step, int dir, int motor)
{
    // asignar memoria
    i = heap_caps_calloc(1, 2, MALLOC_CAP_8BIT);

    // motor id
    if (motor == RO)
        step = cm_to_steps(step);
    else if (motor == ALPHA)
        step = degree_to_steps(step);
    else
        step = 0;

    // determinar direccion
    gpio_set_level(dir_pin[motor], dir);

    // tren de pulsos
    if (motor == ALPHA)
    {
        for (i[0] = 0; i[0] < step; i[0]++)
        {
            gpio_set_level(pul_pin[motor], 1);
            vTaskDelay(period_alpha / portTICK_PERIOD_MS);
            gpio_set_level(pul_pin[motor], 0);
            vTaskDelay(period_alpha / portTICK_PERIOD_MS);
        }
        // frenado
        gpio_set_level(dir_pin[motor], !dir);
        for (i[0] = 0; i[0] < 30; i[0]++)
        {
            gpio_set_level(pul_pin[motor], 1);
            vTaskDelay(period_alpha / portTICK_PERIOD_MS);
            gpio_set_level(pul_pin[motor], 0);
            vTaskDelay(period_alpha / portTICK_PERIOD_MS);
        }
    }
    else
    {
        for (i[0] = 0; i[0] < step; i[0]++)
        {
            gpio_set_level(pul_pin[motor], 1);
            vTaskDelay(period_ro / portTICK_PERIOD_MS);
            gpio_set_level(pul_pin[motor], 0);
            vTaskDelay(period_ro / portTICK_PERIOD_MS);
        }
    }
}

int *limit_level_1; // limit switch alpha
int *limit_level_2; // limit switch ro
int prev_r = 0;     // valor RO previo
int prev_a = 0;     // valor ALPHA previo
/* funcion de calibracion
 * alpha hasta tocar limit_switch
 * ro hasta tocar limit_switch
 */
void calibrate(void)
{
    // memory allocation
    limit_level_1 = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);
    limit_level_2 = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);
    i = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);

    // calibracion ro
    limit_level_2[0] = gpio_get_level(lim_pin_ro);
    gpio_set_level(dir_pin[RO], CCW);
    while (limit_level_2[0] == 0)
    {
        gpio_set_level(pul_pin[RO], 1);
        vTaskDelay(period_ro / portTICK_PERIOD_MS);
        gpio_set_level(pul_pin[RO], 0);
        vTaskDelay(period_ro / portTICK_PERIOD_MS);

        limit_level_2[0] = gpio_get_level(lim_pin_ro);
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);

    // calibracion alpha
    limit_level_1[0] = gpio_get_level(lim_pin_alpha);
    gpio_set_level(dir_pin[ALPHA], CCW);
    while (limit_level_1[0] == 1) // frecuencia de calibracion lenta
    {
        gpio_set_level(pul_pin[ALPHA], 1);
        vTaskDelay(period_alpha / portTICK_PERIOD_MS);
        gpio_set_level(pul_pin[ALPHA], 0);
        vTaskDelay(period_alpha / portTICK_PERIOD_MS);

        limit_level_1[0] = gpio_get_level(lim_pin_alpha);
    }
    // frenado
    for (i[0] = 0; i[0] < 10; i[0]++)
    {
        gpio_set_level(pul_pin[ALPHA], 1);
        vTaskDelay(3 / portTICK_PERIOD_MS);
        gpio_set_level(pul_pin[ALPHA], 0);
        vTaskDelay(3 / portTICK_PERIOD_MS);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    prev_a = 0;
    prev_r = 0;

    // liberar memoria
    heap_caps_free(limit_level_1);
    heap_caps_free(limit_level_2);
    heap_caps_free(i);
}

uint8_t *uart_data; // bus para recibir datos del uart
uint8_t *a_index;   // indice de letra a
uint8_t *f_index;   // indice caracter ';'
char *rc_val;       // string con el valor en RO
char *ac_val;       // string con el valor en ALPHA
int *r;             // int con valor en RO
int *a;             // int con valor en ALPHA
void get_uart_values(int length)
{
    // asignar memoria
    i = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);
    a_index = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);
    f_index = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);
    rc_val = heap_caps_calloc(3, 1, MALLOC_CAP_8BIT);
    ac_val = heap_caps_calloc(3, 1, MALLOC_CAP_8BIT);

    // obtener indices de instruccion: r###a###
    for (i[0] = 0; i[0] < length; i[0]++)
    {
        if ((char)uart_data[i[0]] == 'a')
            a_index[0] = i[0];
        if ((char)uart_data[i[0]] == ';')
            f_index[0] = i[0];
    }

    // convertir a int el valor de desplazamiento en RO
    for (i[0] = 0; i[0] < a_index[0] - 1; i[0]++)
    {
        rc_val[i[0]] = (char)uart_data[i[0] + 1];
    }
    sscanf(rc_val, "%d", &r[0]);

    // convertir a int el valor de desplazamiento en ALPHA
    for (i[0] = a_index[0]; i[0] < f_index[0]; i[0]++)
    {
        ac_val[i[0] - a_index[0]] = (char)uart_data[i[0] + 1];
    }
    sscanf(ac_val, "%d", &a[0]);

    // liberar memoria
    heap_caps_free(i);
    heap_caps_free(a_index);
    heap_caps_free(f_index);
    heap_caps_free(rc_val);
    heap_caps_free(ac_val);
}

int cnt = 0; // contador de fase de rutina
/* Funcion de rutina => buscar, recoger, regresar
 * hace un string-to-int para los valores obtenidos del uart
 * para la distancia en RO y el angulo en ALPHA
 * luego ejecuta la rutina
 */
void rutine(int length)
{
    if (cnt == 0)
    {
        // buscar
        steps(r[0], CW, RO);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        steps(a[0], CW, ALPHA);
        // agarrar
        gpio_set_level(iman_pin, 0);

        cnt = 1;
        prev_a = a[0];
        prev_r = r[0];
    }
    else if (cnt == 1)
    {
        uint8_t *n_dir_ro = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);
        uint8_t *n_dir_alpha = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);
        uint8_t *n_alpha = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);
        uint8_t *n_ro = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);

        if (prev_a < a[0]) // si el nuevo alpha es mayor (movimiento izquierda)
        {
            n_dir_alpha[0] = CW;
            n_alpha[0] = a[0] - prev_a;
        }
        else
        {
            n_dir_alpha[0] = CCW;
            n_alpha[0] = prev_a - a[0];
        }
        if (prev_r < r[0]) // si nuevo ro es mayor (movimiento adelante)
        {
            n_dir_ro[0] = CW;
            n_ro[0] = r[0] - prev_r;
        }
        else
        {
            n_dir_ro[0] = CCW;
            n_ro[0] = prev_r - r[0];
        }

        // ir a lugar destino
        steps(n_ro[0], n_dir_ro[0], RO);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        steps(n_alpha[0], n_dir_alpha[0], ALPHA);
        // soltar
        gpio_set_level(iman_pin, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        // regresar a posicion (0,0)
        calibrate();

        // liberar memoria
        heap_caps_free(n_dir_ro);
        heap_caps_free(n_dir_alpha);
        heap_caps_free(n_ro);
        heap_caps_free(n_alpha);

        cnt = 0;
    }
}

void app_main()
{
    // void setup()
    init_hw();
    init_uart();
    // printf("Config Finalizada. \n");
    calibrate();

    int *length;

    while (1)
    {
        // asignar memoria
        uart_data = heap_caps_calloc(10, 1, MALLOC_CAP_8BIT);
        length = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);
        r = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);
        a = heap_caps_calloc(1, 1, MALLOC_CAP_8BIT);

        // leer uart
        length[0] = uart_read_bytes(uart_num, uart_data, 12, 10);
        if (length[0] > 0)
        {
            // printf((const char *)uart_data);
            uart_flush(uart_num);

            if ((char)uart_data[0] == 'r')
            {
                get_uart_values(length[0]);
                rutine(length[0]); // funcion de rutina
            }
            else if ((char)uart_data[0] == 'f')
            {
                steps(1, CW, RO);
            }
            else if ((char)uart_data[0] == 'b')
            {
                steps(1, CCW, RO);
            }
            else if ((char)uart_data[0] == 'd')
            {
                steps(15, CCW, ALPHA);
            }
            else if ((char)uart_data[0] == 'l')
            {
                steps(15, CW, ALPHA);
            }
            else if ((char)uart_data[0] == 'c')
            {
                calibrate();
            }
        }

        // liberar memoria
        heap_caps_free(uart_data);
        heap_caps_free(length);
        heap_caps_free(r);
        heap_caps_free(a);

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// IMPORTANTE - modificar sdkconfig.h #define CONFIG_FREERTOS_HZ 100 a 1000