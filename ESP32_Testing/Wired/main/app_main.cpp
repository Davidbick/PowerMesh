#include <esp_err.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#include <common_macros.h>
#include <log_heap_numbers.h>

#include <app_priv.h>
#include <app_reset.h>
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include <platform/ESP32/OpenthreadLauncher.h>
#endif

#include <app/server/CommissioningWindowManager.h>
#include <app/server/Server.h>



#include <platform/CHIPDeviceLayer.h>

#include <inet/InetInterface.h>
#include <inet/IPAddress.h>
#include <string.h>

#include "uart_frame.h"
#include "uart_ipv6_netif.h"
#include "driver/uart.h"
#include "uart_ipv6_udp_test.h"



#ifdef CONFIG_ENABLE_SET_CERT_DECLARATION_API
#include <esp_matter_providers.h>
#include <lib/support/Span.h>
#ifdef CONFIG_SEC_CERT_DAC_PROVIDER
#include <platform/ESP32/ESP32SecureCertDACProvider.h>
#elif defined(CONFIG_FACTORY_PARTITION_DAC_PROVIDER)
#include <platform/ESP32/ESP32FactoryDataProvider.h>
#endif
using namespace chip::DeviceLayer;
#endif

static const char *TAG = "app_main";
uint16_t light_endpoint_id = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

constexpr auto k_timeout_seconds = 300;

#ifdef CONFIG_ENABLE_SET_CERT_DECLARATION_API
extern const uint8_t cd_start[] asm("_binary_certification_declaration_der_start");
extern const uint8_t cd_end[] asm("_binary_certification_declaration_der_end");

const chip::ByteSpan cdSpan(cd_start, static_cast<size_t>(cd_end - cd_start));
#endif // CONFIG_ENABLE_SET_CERT_DECLARATION_API

#if CONFIG_ENABLE_ENCRYPTED_OTA
extern const char decryption_key_start[] asm("_binary_esp_image_encryption_key_pem_start");
extern const char decryption_key_end[] asm("_binary_esp_image_encryption_key_pem_end");

static const char *s_decryption_key = decryption_key_start;
static const uint16_t s_decryption_key_len = decryption_key_end - decryption_key_start;
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address changed");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        MEMORY_PROFILER_DUMP_HEAP_STAT("commissioning complete");
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        MEMORY_PROFILER_DUMP_HEAP_STAT("commissioning window opened");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricRemoved: {
        ESP_LOGI(TAG, "Fabric removed successfully");
        if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0) {
            chip::CommissioningWindowManager  &commissionMgr = chip::Server::GetInstance().GetCommissioningWindowManager();
            constexpr auto kTimeoutSeconds = chip::System::Clock::Seconds16(k_timeout_seconds);
            if (!commissionMgr.IsCommissioningWindowOpen()) {
                /* After removing last fabric, this example does not remove the Wi-Fi credentials
                 * and still has IP connectivity so, only advertising on DNS-SD.
                 */
                CHIP_ERROR err = commissionMgr.OpenBasicCommissioningWindow(kTimeoutSeconds,
                                                                            chip::CommissioningWindowAdvertisement::kDnssdOnly);
                if (err != CHIP_NO_ERROR) {
                    ESP_LOGE(TAG, "Failed to open commissioning window, err:%" CHIP_ERROR_FORMAT, err.Format());
                }
            }
        }
        break;
    }

    case chip::DeviceLayer::DeviceEventType::kFabricWillBeRemoved:
        ESP_LOGI(TAG, "Fabric will be removed");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricUpdated:
        ESP_LOGI(TAG, "Fabric is updated");
        break;

    case chip::DeviceLayer::DeviceEventType::kFabricCommitted:
        ESP_LOGI(TAG, "Fabric is committed");
        break;

    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
        ESP_LOGI(TAG, "BLE deinitialized and memory reclaimed");
        MEMORY_PROFILER_DUMP_HEAP_STAT("BLE deinitialized");
        break;

    default:
        break;
    }
}

// This callback is invoked when clients interact with the Identify Cluster.
// In the callback implementation, an endpoint can identify itself. (e.g., by flashing an LED or light).
static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

// This callback is called for every attribute update. The callback implementation shall
// handle the desired attributes and return an appropriate error code. If the attribute
// is not of your interest, please do not return an error code and strictly return ESP_OK.
static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    esp_err_t err = ESP_OK;

    if (type == PRE_UPDATE) {
        /* Driver update */
        app_driver_handle_t driver_handle = (app_driver_handle_t)priv_data;
        err = app_driver_attribute_update(driver_handle, endpoint_id, cluster_id, attribute_id, val);
    }

    return err;
}

// UART / IPv6 testing =======================
static void uart_frame_received(
    uint8_t frame_type,
    uint16_t sequence,
    const uint8_t *payload,
    size_t length,
    void *context)
{
    (void)context;

    if (frame_type == UART_FRAME_TYPE_ACK) {
        ESP_LOGI(TAG, "Received ACK: seq=%u", (unsigned)sequence);
        return;
    }

    if (frame_type == UART_FRAME_TYPE_DATA) {
        ESP_LOGI(
            TAG,
            "Received DATA: seq=%u len=%u",
            (unsigned)sequence,
            (unsigned)length
        );

        esp_err_t input_err = uart_ipv6_netif_input(payload, length);
        if (input_err == ESP_OK) {
            ESP_LOGI(TAG, "Queued DATA seq=%u into lwIP", (unsigned)sequence);
        } else {
            ESP_LOGW(TAG,
                     "Could not pass DATA seq=%u to lwIP: %s",
                     (unsigned)sequence,
                     esp_err_to_name(input_err));
        }
    }
}
// ==========================================

/*
 * Diagnostic only: enumerate interfaces using CHIP's Inet API, rather than
 * inspecting lwIP directly. Run after uart_ipv6_netif_start() has added uaN.
 */
static void log_chip_inet_interfaces(void)
{
    ESP_LOGI(TAG, "========== CHIP Inet interface diagnostic ==========");

    unsigned int interface_count = 0;
    bool found_uart_interface = false;
    bool uart_interface_up = false;
    bool uart_interface_multicast = false;

    chip::Inet::InterfaceIterator interfaces;
    for (; interfaces.HasCurrent(); interfaces.Next()) {
        char interface_name[chip::Inet::InterfaceId::kMaxIfNameLength] = {};
        CHIP_ERROR name_error = interfaces.GetInterfaceName(
            interface_name, sizeof(interface_name));

        if (name_error != CHIP_NO_ERROR) {
            ESP_LOGW(TAG, "CHIP Inet: failed to read interface name");
            continue;
        }

        const bool is_up = interfaces.IsUp();
        const bool supports_multicast = interfaces.SupportsMulticast();

        ESP_LOGI(TAG,
                 "CHIP interface[%u]: name=%s up=%s multicast=%s",
                 interface_count,
                 interface_name,
                 is_up ? "yes" : "no",
                 supports_multicast ? "yes" : "no");
        interface_count++;

        // lwIP formats this interface as uaN, e.g. ua0.
        if (interface_name[0] == 'u' && interface_name[1] == 'a') {
            found_uart_interface = true;
            uart_interface_up = is_up;
            uart_interface_multicast = supports_multicast;
        }
    }

    if (interface_count == 0) {
        ESP_LOGE(TAG, "CHIP Inet iterator found no network interfaces");
    }

    if (!found_uart_interface) {
        ESP_LOGE(TAG, "FAIL: CHIP Inet iterator does not see the UART netif (uaN)");
    } else {
        ESP_LOGI(TAG,
                 "UART netif visibility: found=yes up=%s multicast=%s",
                 uart_interface_up ? "yes" : "no",
                 uart_interface_multicast ? "yes" : "no");

        if (!uart_interface_up) {
            ESP_LOGE(TAG, "FAIL: CHIP sees the UART netif, but it is not up");
        }
        if (!uart_interface_multicast) {
            ESP_LOGW(TAG,
                     "WARNING: CHIP reports no multicast support on the UART netif; "
                     "Matter discovery may not use it");
        }
    }

    unsigned int address_count = 0;
    bool found_uart_link_local = false;
    bool found_uart_ula = false;

    chip::Inet::InterfaceAddressIterator addresses;
    for (; addresses.HasCurrent(); addresses.Next()) {
        char interface_name[chip::Inet::InterfaceId::kMaxIfNameLength] = {};
        CHIP_ERROR name_error = addresses.GetInterfaceName(
            interface_name, sizeof(interface_name));
        if (name_error != CHIP_NO_ERROR) {
            ESP_LOGW(TAG, "CHIP Inet: failed to read interface name for address");
            continue;
        }

        chip::Inet::IPAddress address;
        CHIP_ERROR address_error = addresses.GetAddress(address);
        if (address_error != CHIP_NO_ERROR) {
            ESP_LOGW(TAG, "CHIP Inet: failed to read address on %s", interface_name);
            continue;
        }

        char address_text[chip::Inet::IPAddress::kMaxStringLength] = {};
        if (address.ToString(address_text, sizeof(address_text)) == nullptr) {
            ESP_LOGW(TAG, "CHIP Inet: failed to format address on %s", interface_name);
            continue;
        }

        const bool is_up = addresses.IsUp();
        const bool supports_multicast = addresses.SupportsMulticast();

        ESP_LOGI(TAG,
                 "CHIP address[%u]: %s on %s up=%s multicast=%s",
                 address_count,
                 address_text,
                 interface_name,
                 is_up ? "yes" : "no",
                 supports_multicast ? "yes" : "no");
        address_count++;

        if (interface_name[0] == 'u' && interface_name[1] == 'a') {
            if (strcmp(address_text, "fe80::2") == 0) {
                found_uart_link_local = true;
            }
            if (strcmp(address_text, "fd00:706d::2") == 0) {
                found_uart_ula = true;
            }
        }
    }

    ESP_LOGI(TAG,
             "CHIP Inet diagnostic result: uaN=%s fe80::2=%s fd00:706d::2=%s",
             found_uart_interface ? "FOUND" : "MISSING",
             found_uart_link_local ? "FOUND" : "MISSING",
             found_uart_ula ? "FOUND" : "MISSING");
    ESP_LOGI(TAG, "========== end CHIP Inet diagnostic ==========");
}

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    /* Initialize the ESP NVS layer */
    nvs_flash_init();

    MEMORY_PROFILER_DUMP_HEAP_STAT("Bootup");

    /* Initialize driver */
    app_driver_handle_t light_handle = app_driver_light_init();
    app_driver_handle_t button_handle = app_driver_button_init();
    app_reset_button_register(button_handle);

    /* Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config;

    // node handle can be used to add/modify other endpoints.
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

    MEMORY_PROFILER_DUMP_HEAP_STAT("node created");

    extended_color_light::config_t light_config;
    light_config.on_off.on_off = DEFAULT_POWER;
    light_config.on_off_lighting.start_up_on_off = nullptr;
    light_config.level_control.current_level = DEFAULT_BRIGHTNESS;
    light_config.level_control.on_level = DEFAULT_BRIGHTNESS;
    light_config.level_control_lighting.start_up_current_level = DEFAULT_BRIGHTNESS;
    light_config.color_control.color_mode = (uint8_t)ColorControl::ColorMode::kColorTemperature;
    light_config.color_control.enhanced_color_mode = (uint8_t)ColorControl::ColorMode::kColorTemperature;
    light_config.color_control_color_temperature.start_up_color_temperature_mireds = nullptr;

    // endpoint handles can be used to add/modify clusters.
    endpoint_t *endpoint = extended_color_light::create(node, &light_config, ENDPOINT_FLAG_NONE, light_handle);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create extended color light endpoint"));

    light_endpoint_id = endpoint::get_id(endpoint);
    ESP_LOGI(TAG, "Light created with endpoint_id %d", light_endpoint_id);

    /* Mark deferred persistence for some attributes that might be changed rapidly */
    attribute_t *current_level_attribute = attribute::get(light_endpoint_id, LevelControl::Id, LevelControl::Attributes::CurrentLevel::Id);
    attribute::set_deferred_persistence(current_level_attribute);

    attribute_t *current_x_attribute = attribute::get(light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentX::Id);
    attribute::set_deferred_persistence(current_x_attribute);
    attribute_t *current_y_attribute = attribute::get(light_endpoint_id, ColorControl::Id, ColorControl::Attributes::CurrentY::Id);
    attribute::set_deferred_persistence(current_y_attribute);
    attribute_t *color_temp_attribute = attribute::get(light_endpoint_id, ColorControl::Id, ColorControl::Attributes::ColorTemperatureMireds::Id);
    attribute::set_deferred_persistence(color_temp_attribute);

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD && CHIP_DEVICE_CONFIG_ENABLE_WIFI_STATION
    // Enable secondary network interface
    secondary_network_interface::config_t secondary_network_interface_config;
    endpoint = endpoint::secondary_network_interface::create(node, &secondary_network_interface_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(endpoint != nullptr, ESP_LOGE(TAG, "Failed to create secondary network interface endpoint"));
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    /* Set OpenThread platform config */
    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&config);
#endif

#ifdef CONFIG_ENABLE_SET_CERT_DECLARATION_API
    auto * dac_provider = get_dac_provider();
#ifdef CONFIG_SEC_CERT_DAC_PROVIDER
    static_cast<ESP32SecureCertDACProvider *>(dac_provider)->SetCertificationDeclaration(cdSpan);
#elif defined(CONFIG_FACTORY_PARTITION_DAC_PROVIDER)
    static_cast<ESP32FactoryDataProvider *>(dac_provider)->SetCertificationDeclaration(cdSpan);
#endif
#endif // CONFIG_ENABLE_SET_CERT_DECLARATION_API

    /* Matter start */
    err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter, err:%d", err));

    MEMORY_PROFILER_DUMP_HEAP_STAT("matter started");

    /* Starting driver with default values */
    app_driver_light_set_defaults(light_endpoint_id);


    // UART TESTING ==============================
    constexpr int UART_PORT = UART_NUM_1;
    constexpr int UART_TX_GPIO = 17;
    constexpr int UART_RX_GPIO = 18;

    err = uart_frame_uart_start(
        UART_PORT,
        UART_TX_GPIO,
        UART_RX_GPIO
    );

    ABORT_APP_ON_FAILURE(
        err == ESP_OK,
        ESP_LOGE(TAG, "Failed to initialize UART")
    );

    err = uart_ipv6_netif_start();
    ABORT_APP_ON_FAILURE(
        err == ESP_OK,
        ESP_LOGE(TAG, "Failed to start UART IPv6 lwIP netif: %s", esp_err_to_name(err))
    );

    // Request Matter DNS-SD to refresh after adding the UART IPv6 interface.
    chip::DeviceLayer::ChipDeviceEvent dnssd_restart_event{};
    dnssd_restart_event.Type =
        chip::DeviceLayer::DeviceEventType::kDnssdRestartNeeded;
    CHIP_ERROR post_err =
        chip::DeviceLayer::PlatformMgr().PostEvent(&dnssd_restart_event);
    if (post_err != CHIP_NO_ERROR) {
        ESP_LOGE(TAG, "Failed to request Matter DNS-SD restart: %" CHIP_ERROR_FORMAT, post_err.Format());
    } else {
        ESP_LOGI(TAG, "Requested Matter DNS-SD restart after UART netif init");
    }

    // Diagnostic: check whether CHIP's Inet layer sees the custom netif.
    log_chip_inet_interfaces();

    err = uart_frame_parser_start(
        uart_frame_received,
        nullptr
    );

    ABORT_APP_ON_FAILURE(
        err == ESP_OK,
        ESP_LOGE(TAG, "Failed to start UART parser")
    );

    err = uart_ipv6_udp_test_start();

    ABORT_APP_ON_FAILURE(
        err == ESP_OK,
        ESP_LOGE(TAG, "Failed to start IPv6 UDP test receiver")
    );

    ESP_LOGI(TAG, "UART frame + raw IPv6 lwIP input initialized");
    // ===========================================


#if CONFIG_ENABLE_ENCRYPTED_OTA
    err = esp_matter_ota_requestor_encrypted_init(s_decryption_key, s_decryption_key_len);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to initialized the encrypted OTA, err: %d", err));
#endif // CONFIG_ENABLE_ENCRYPTED_OTA

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::wifi_register_commands();
    esp_matter::console::factoryreset_register_commands();
    esp_matter::console::attribute_register_commands();
#if CONFIG_OPENTHREAD_CLI
    esp_matter::console::otcli_register_commands();
#endif
    esp_matter::console::init();
#endif

    while (true) {
        MEMORY_PROFILER_DUMP_HEAP_STAT("Idle");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
