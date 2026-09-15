#include <stdio.h>
#include <math.h>

#include "esp_log.h"
#include "esp_err.h"

#include "esp_matter.h"
#include "esp_matter_endpoint.h"

#include <app/server/OnboardingCodesUtil.h>

// ESP-IDF LED strip component
#include "led_strip.h"
#include "led_strip_rmt.h"

static const char *TAG = "matter_light";

// Specifically for ESP32-S3-DevKitC-1 v1.1

static constexpr gpio_num_t LIGHT_GPIO = GPIO_NUM_38;

// There is one onboard RGB LED.
static constexpr uint32_t LED_COUNT = 1;

static led_strip_handle_t led_strip = nullptr;

// Matter end-point
static uint16_t light_endpoint_id = 0;

// Matter Hue and Saturation are represented as 0-254.
//
// Hue:
//   0   = red
//   42  = yellow
//   85  = green
//   127 = cyan
//   169 = blue
//   212 = magenta
//
// Saturation:
//   0   = white
//   254 = fully saturated
//
// Brightness:
//   0   = off
//   254 = maximum
//
// We keep the values in Matter's native range.

static uint8_t current_hue = 0;
static uint8_t current_saturation = 0;
static uint8_t current_brightness = 254;

static bool current_on = false;


// ============================================================================
// HSV -> RGB
// ============================================================================
//
// Converts Matter's Hue/Saturation/Value representation into 8-bit RGB.
//
// Matter Hue/Saturation/Value use approximately 0-254.
// RGB uses 0-255.
//
// ============================================================================

static void hsv_to_rgb(
    uint8_t hue,
    uint8_t saturation,
    uint8_t value,
    uint8_t *red,
    uint8_t *green,
    uint8_t *blue)
{
    float h = ((float)hue / 254.0f) * 360.0f;
    float s = (float)saturation / 254.0f;
    float v = (float)value / 254.0f;

    float r;
    float g;
    float b;

    if (s <= 0.0f) {
        // No saturation = white/gray
        r = v;
        g = v;
        b = v;
    }
    else {
        float sector = h / 60.0f;
        int i = (int)floorf(sector);
        float f = sector - i;

        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i % 6) {
            case 0:
                r = v;
                g = t;
                b = p;
                break;

            case 1:
                r = q;
                g = v;
                b = p;
                break;

            case 2:
                r = p;
                g = v;
                b = t;
                break;

            case 3:
                r = p;
                g = q;
                b = v;
                break;

            case 4:
                r = t;
                g = p;
                b = v;
                break;

            default:
                r = v;
                g = p;
                b = q;
                break;
        }
    }

    *red   = (uint8_t)(r * 255.0f);
    *green = (uint8_t)(g * 255.0f);
    *blue  = (uint8_t)(b * 255.0f);
}


// ============================================================================
// Update physical RGB LED
// ============================================================================

static esp_err_t update_rgb_led()
{
    if (led_strip == nullptr) {
        ESP_LOGE(TAG, "LED strip is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    if (current_on && current_brightness > 0) {

        hsv_to_rgb(
            current_hue,
            current_saturation,
            current_brightness,
            &r,
            &g,
            &b
        );
    }

    ESP_LOGI(
        TAG,
        "LED state: %s  HSV=(%u,%u,%u) RGB=(%u,%u,%u)",
        current_on ? "ON" : "OFF",
        current_hue,
        current_saturation,
        current_brightness,
        r,
        g,
        b
    );

    // WS2812 LED
    //
    // led_strip_set_pixel() takes RGB.
    // The component handles the GRB ordering internally based
    // on the configured color_component_format.

    ESP_ERROR_CHECK(
        led_strip_set_pixel(
            led_strip,
            0,
            r,
            g,
            b
        )
    );

    ESP_ERROR_CHECK(
        led_strip_refresh(led_strip)
    );

    return ESP_OK;
}


// ============================================================================
// Initialize WS2812
// ============================================================================

static void init_rgb_led()
{
    ESP_LOGI(TAG, "Initializing onboard RGB LED");

    led_strip_config_t strip_config = {
        .strip_gpio_num = LIGHT_GPIO,
        .max_leds = LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        },
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 0,
        .flags = {
            .with_dma = false,
        },
    };

    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(
            &strip_config,
            &rmt_config,
            &led_strip
        )
    );

    // Start OFF.
    ESP_ERROR_CHECK(
        led_strip_clear(led_strip)
    );

    ESP_LOGI(TAG, "RGB LED initialized");
}


// ============================================================================
// Matter attribute callback
// ============================================================================

static esp_err_t app_attribute_update_cb(
    esp_matter::attribute::callback_type_t type,
    uint16_t endpoint_id,
    uint32_t cluster_id,
    uint32_t attribute_id,
    esp_matter_attr_val_t *val,
    void *priv_data)
{
    if (type != esp_matter::attribute::callback_type_t::PRE_UPDATE) {
        return ESP_OK;
    }

    if (endpoint_id != light_endpoint_id) {
        return ESP_OK;
    }


    // ------------------------------------------------------------------------
    // ON/OFF
    // ------------------------------------------------------------------------

    if (cluster_id == chip::app::Clusters::OnOff::Id &&
        attribute_id ==
            chip::app::Clusters::OnOff::Attributes::OnOff::Id) {

        current_on = val->val.b;

        ESP_LOGI(
            TAG,
            "Matter OnOff: %s",
            current_on ? "ON" : "OFF"
        );

        return update_rgb_led();
    }


    // ------------------------------------------------------------------------
    // BRIGHTNESS
    // ------------------------------------------------------------------------

    if (cluster_id == chip::app::Clusters::LevelControl::Id &&
        attribute_id ==
            chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Id) {

        current_brightness = val->val.u8;

        ESP_LOGI(
            TAG,
            "Matter Brightness: %u",
            current_brightness
        );

        return update_rgb_led();
    }


    // ------------------------------------------------------------------------
    // HUE
    // ------------------------------------------------------------------------

    if (cluster_id == chip::app::Clusters::ColorControl::Id &&
        attribute_id ==
            chip::app::Clusters::ColorControl::Attributes::CurrentHue::Id) {

        current_hue = val->val.u8;

        ESP_LOGI(
            TAG,
            "Matter Hue: %u",
            current_hue
        );

        return update_rgb_led();
    }


    // ------------------------------------------------------------------------
    // SATURATION
    // ------------------------------------------------------------------------

    if (cluster_id == chip::app::Clusters::ColorControl::Id &&
        attribute_id ==
            chip::app::Clusters::ColorControl::Attributes::CurrentSaturation::Id) {

        current_saturation = val->val.u8;

        ESP_LOGI(
            TAG,
            "Matter Saturation: %u",
            current_saturation
        );

        return update_rgb_led();
    }

    return ESP_OK;
}


// ============================================================================
// Matter Identify callback
// ============================================================================

static esp_err_t app_identification_cb(
    esp_matter::identification::callback_type_t type,
    uint16_t endpoint_id,
    uint8_t effect_id,
    uint8_t effect_variant,
    void *priv_data)
{
    ESP_LOGI(
        TAG,
        "Identify requested for endpoint %u",
        endpoint_id
    );

    return ESP_OK;
}


// ============================================================================
// Matter event callback
// ============================================================================

static void app_event_cb(
    const ChipDeviceEvent *event,
    intptr_t arg)
{
}


// ============================================================================
// Main
// ============================================================================

extern "C" void app_main()
{
    ESP_LOGI(TAG, "Starting Matter RGB Light");

    init_rgb_led();

    // ------------------------------------------------------------------------
    // Create Matter node
    // ------------------------------------------------------------------------

    esp_matter::node::config_t node_config;

    esp_matter::node_t *node =
        esp_matter::node::create(
            &node_config,
            app_attribute_update_cb,
            app_identification_cb
        );

    if (node == nullptr) {
        ESP_LOGE(TAG, "Failed to create Matter node");
        return;
    }


    // ------------------------------------------------------------------------
    // Create Extended Color Light endpoint
    // ------------------------------------------------------------------------
    //
    // This endpoint provides:
    //
    //   On/Off
    //   Brightness
    //   Hue
    //   Saturation
    //   Color Control
    //
    // ------------------------------------------------------------------------

    esp_matter::endpoint::extended_color_light::config_t light_config;

    // Initial On/Off state
    light_config.on_off.on_off = false;

    // Initial brightness
    light_config.level_control.current_level = 254;

    // Initial Hue
    light_config.color_control.current_hue = 0;

    // Initial Saturation
    light_config.color_control.current_saturation = 0;

    // Color mode = HSV
    light_config.color_control.color_mode =
        (uint8_t)chip::app::Clusters::ColorControl::ColorMode::kCurrentHueAndCurrentSaturation;

    light_config.color_control.enhanced_color_mode =
        (uint8_t)chip::app::Clusters::ColorControl::ColorMode::kCurrentHueAndCurrentSaturation;


    esp_matter::endpoint_t *endpoint =
        esp_matter::endpoint::extended_color_light::create(
            node,
            &light_config,
            esp_matter::ENDPOINT_FLAG_NONE,
            nullptr
        );

    if (endpoint == nullptr) {
        ESP_LOGE(
            TAG,
            "Failed to create Matter extended color light endpoint"
        );

        return;
    }


    // Save endpoint ID
    light_endpoint_id =
        esp_matter::endpoint::get_id(endpoint);

    ESP_LOGI(
        TAG,
        "Matter RGB light endpoint: %u",
        light_endpoint_id
    );


    // ------------------------------------------------------------------------
    // Start Matter
    // ------------------------------------------------------------------------

    esp_err_t err =
        esp_matter::start(app_event_cb);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Failed to start Matter: %s",
            esp_err_to_name(err)
        );

        return;
    }

    ESP_LOGI(TAG, "Matter started");


    // ------------------------------------------------------------------------
    // Print commissioning codes
    // ------------------------------------------------------------------------

    ESP_LOGI(
        TAG,
        "Matter commissioning information:"
    );

    PrintOnboardingCodes(
        chip::RendezvousInformationFlags(
            chip::RendezvousInformationFlag::kBLE
        )
    );
}
