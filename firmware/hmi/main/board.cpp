#include "board.h"

#include <algorithm>
#include <iterator>

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace board {
namespace {

constexpr const char* kTag = "board";
constexpr int kWidth = 1024;
constexpr int kHeight = 600;
constexpr int kTimeoutMs = 50;

// Bus I2C
constexpr gpio_num_t kSda = GPIO_NUM_8;
constexpr gpio_num_t kScl = GPIO_NUM_9;
constexpr uint32_t kI2cHz = 400 * 1000;

// Expansor CH32V003 en 0x24: 0x02 es el modo de los 8 pines (1 = salida),
// 0x03 las salidas y 0x05 el PWM del backlight
constexpr uint8_t kExpanderAddress = 0x24;
constexpr uint8_t kExpanderMode = 0x02;
constexpr uint8_t kExpanderOutput = 0x03;
constexpr uint8_t kExpanderPwm = 0x05;
constexpr uint8_t kTouchReset = 1 << 1;  // IO1: en bajo resetea el GT911
constexpr uint8_t kBacklight = 1 << 2;   // IO2: prende el backlight
constexpr uint8_t kCanSelect = 1 << 5;   // IO5: GPIO19/20 al CAN (1) o al USB (0)
// Lo demás en alto, como el demo: IO3 suelta el reset de la pantalla e IO4
// deja sin elegir la micro SD. IO5 en 0: la consola va por el USB nativo.
uint8_t outputs = static_cast<uint8_t>(0xFF & ~kTouchReset & ~kBacklight & ~kCanSelect);

// Táctil GT911: con INT en bajo durante el reset queda en 0x5D
constexpr gpio_num_t kTouchInt = GPIO_NUM_4;
constexpr uint8_t kTouchAddress = 0x5D;
constexpr uint16_t kTouchStatus = 0x814E;  // bit 7: hay lectura nueva; bits 0-3: cuántos dedos
constexpr uint16_t kTouchFirst = 0x814F;   // primer dedo: id, x (2), y (2), tamaño (2), reservado

i2c_master_bus_handle_t bus = nullptr;
i2c_master_dev_handle_t expander = nullptr;
i2c_master_dev_handle_t touch = nullptr;
bool expander_found = false;
bool touch_found = false;

esp_lcd_panel_handle_t panel = nullptr;
TaskHandle_t lvgl_task = nullptr;

i2c_master_dev_handle_t add_device(uint8_t address) {
    i2c_device_config_t config = {};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = address;
    config.scl_speed_hz = kI2cHz;
    i2c_master_dev_handle_t device = nullptr;
    if (i2c_master_bus_add_device(bus, &config, &device) != ESP_OK) return nullptr;
    return device;
}

bool expander_write(uint8_t reg, uint8_t value) {
    if (expander == nullptr) return false;
    const uint8_t data[2] = {reg, value};
    return i2c_master_transmit(expander, data, sizeof data, kTimeoutMs) == ESP_OK;
}

void set_output(uint8_t bit, bool high) {
    outputs = high ? (outputs | bit) : (outputs & ~bit);
    expander_write(kExpanderOutput, outputs);
}

// Reset del GT911 con INT en bajo para que quede en 0x5D (secuencia del demo)
void reset_touch() {
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << kTouchInt;
    config.mode = GPIO_MODE_OUTPUT;
    gpio_config(&config);
    set_output(kTouchReset, false);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(kTouchInt, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    set_output(kTouchReset, true);
    vTaskDelay(pdMS_TO_TICKS(200));
    config.mode = GPIO_MODE_INPUT;  // ya arrancó: INT es del GT911
    gpio_config(&config);
}

bool touch_read_reg(uint16_t reg, uint8_t* data, size_t size) {
    const uint8_t address[2] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xFF)};
    return i2c_master_transmit_receive(touch, address, sizeof address, data, size, kTimeoutMs) == ESP_OK;
}

bool touch_clear() {
    const uint8_t data[3] = {static_cast<uint8_t>(kTouchStatus >> 8), static_cast<uint8_t>(kTouchStatus & 0xFF), 0};
    return i2c_master_transmit(touch, data, sizeof data, kTimeoutMs) == ESP_OK;
}

// LVGL pregunta cada 30 ms dónde está el dedo. El GT911 avisa cuando tiene
// lectura nueva; mientras no, se queda lo último que dijo.
void read_touch(lv_indev_t*, lv_indev_data_t* data) {
    static bool pressed = false;
    static int32_t x = 0;
    static int32_t y = 0;
    uint8_t status = 0;
    if (touch == nullptr || !touch_read_reg(kTouchStatus, &status, 1)) {
        pressed = false;  // si el I2C falla, el dedo no se queda pegado
    } else if ((status & 0x80) != 0) {
        const uint8_t fingers = status & 0x0F;
        uint8_t point[8] = {};
        pressed = fingers > 0 && fingers <= 5 && touch_read_reg(kTouchFirst, point, sizeof point);
        if (pressed) {
            x = std::min<int32_t>(point[1] | (point[2] << 8), kWidth - 1);
            y = std::min<int32_t>(point[3] | (point[4] << 8), kHeight - 1);
        }
        touch_clear();
    }
    data->point.x = x;
    data->point.y = y;
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

// Dos framebuffers en la PSRAM y LVGL en modo directo: dibuja en el que no
// se ve y al terminar la pantalla cambia al otro. Igual que el ejemplo
// rgb_panel de ESP-IDF.
bool IRAM_ATTR on_frame_done(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t*, void*) {
    BaseType_t wake = pdFALSE;
    if (lvgl_task != nullptr) vTaskNotifyGiveFromISR(lvgl_task, &wake);
    return wake == pdTRUE;
}

void flush(lv_display_t* display, const lv_area_t*, uint8_t* pixels) {
    if (!lv_display_flush_is_last(display)) {
        lv_display_flush_ready(display);
        return;
    }
    ulTaskNotifyTake(pdTRUE, 0);  // se tira un aviso viejo antes de esperar este cuadro
    esp_lcd_panel_draw_bitmap(panel, 0, 0, kWidth, kHeight, pixels);
}

void flush_wait(lv_display_t* display) {
    // No se vuelve a dibujar en un framebuffer que la pantalla sigue leyendo
    if (lv_display_flush_is_last(display)) ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    lv_display_flush_ready(display);
}

esp_err_t start_panel() {
    esp_lcd_rgb_panel_config_t config = {};
    config.clk_src = LCD_CLK_SRC_DEFAULT;
    config.timings.pclk_hz = CONFIG_HMI_LCD_PCLK_MHZ * 1000 * 1000;
    config.timings.h_res = kWidth;
    config.timings.v_res = kHeight;
    config.timings.hsync_pulse_width = 162;
    config.timings.hsync_back_porch = 152;
    config.timings.hsync_front_porch = 48;
    config.timings.vsync_pulse_width = 45;
    config.timings.vsync_back_porch = 13;
    config.timings.vsync_front_porch = 3;
    config.timings.flags.pclk_active_neg = 1;
    config.data_width = 16;
    config.num_fbs = 2;
    config.bounce_buffer_size_px = kWidth * 20;  // 20 renglones en la RAM interna
    config.dma_burst_size = 64;
    config.hsync_gpio_num = GPIO_NUM_46;
    config.vsync_gpio_num = GPIO_NUM_3;
    config.de_gpio_num = GPIO_NUM_5;
    config.pclk_gpio_num = GPIO_NUM_7;
    config.disp_gpio_num = GPIO_NUM_NC;
    const gpio_num_t data[16] = {GPIO_NUM_14, GPIO_NUM_38, GPIO_NUM_18, GPIO_NUM_17, GPIO_NUM_10, GPIO_NUM_39,
                                 GPIO_NUM_0,  GPIO_NUM_45, GPIO_NUM_48, GPIO_NUM_47, GPIO_NUM_21, GPIO_NUM_1,
                                 GPIO_NUM_2,  GPIO_NUM_42, GPIO_NUM_41, GPIO_NUM_40};
    std::copy(std::begin(data), std::end(data), config.data_gpio_nums);
    config.flags.fb_in_psram = 1;

    esp_err_t err = esp_lcd_new_rgb_panel(&config, &panel);
    if (err == ESP_OK) err = esp_lcd_panel_reset(panel);
    if (err == ESP_OK) err = esp_lcd_panel_init(panel);
    if (err != ESP_OK) return err;

    esp_lcd_rgb_panel_event_callbacks_t callbacks = {};
    callbacks.on_frame_buf_complete = on_frame_done;
    return esp_lcd_rgb_panel_register_event_callbacks(panel, &callbacks, nullptr);
}

}  // namespace

void start_i2c() {
    i2c_master_bus_config_t config = {};
    config.i2c_port = I2C_NUM_0;
    config.sda_io_num = kSda;
    config.scl_io_num = kScl;
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = 1;
    ESP_ERROR_CHECK(i2c_new_master_bus(&config, &bus));

    expander_found = probe(kExpanderAddress);
    if (expander_found) {
        expander = add_device(kExpanderAddress);
        expander_write(kExpanderMode, 0xFF);  // los 8 pines como salida
        expander_write(kExpanderOutput, outputs);
    } else {
        ESP_LOGE(kTag, "El expansor (0x%02x) no contesta: sin backlight ni táctil", kExpanderAddress);
    }
}

i2c_master_bus_handle_t i2c() { return bus; }

bool probe(uint8_t address) { return bus != nullptr && i2c_master_probe(bus, address, kTimeoutMs) == ESP_OK; }

lv_display_t* start_display() {
    lvgl_task = xTaskGetCurrentTaskHandle();
    ESP_ERROR_CHECK(start_panel());

    lv_display_t* display = lv_display_create(kWidth, kHeight);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    void* first = nullptr;
    void* second = nullptr;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel, 2, &first, &second));
    lv_display_set_buffers(display, first, second, kWidth * kHeight * 2, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(display, flush);
    lv_display_set_flush_wait_cb(display, flush_wait);
    return display;
}

void start_touch(lv_display_t* display) {
    if (expander_found) reset_touch();
    touch_found = probe(kTouchAddress);
    if (!touch_found) {
        ESP_LOGE(kTag, "El táctil GT911 (0x%02x) no contesta", kTouchAddress);
        return;
    }
    touch = add_device(kTouchAddress);
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, read_touch);
    lv_indev_set_display(indev, display);
}

bool touch_ok() { return touch_found; }

bool expander_ok() { return expander_found; }

void backlight(bool on) { set_output(kBacklight, on); }

void set_brightness(uint8_t percent) {
    // El PWM va al revés: 0 es todo el brillo. El demo no lo deja pasar de 97
    // para que la pantalla nunca quede negra del todo.
    const int off = std::clamp(100 - static_cast<int>(percent), 0, 97);
    expander_write(kExpanderPwm, static_cast<uint8_t>(off * 255 / 100));
}

}  // namespace board
