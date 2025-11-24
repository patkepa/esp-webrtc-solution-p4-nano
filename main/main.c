/* Door Bell Demo

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "sdkconfig.h"

// Define WEBRTC_SUPPORT_OPUS based on sdkconfig
#if CONFIG_AUDIO_ENCODER_OPUS_SUPPORT
#define WEBRTC_SUPPORT_OPUS
#endif

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_random.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_webrtc.h"
#include "media_lib_adapter.h"
#include "media_lib_os.h"
#include "esp_timer.h"
#include "webrtc_utils_time.h"
#include "esp_cpu.h"
#include "settings.h"
#include "common.h"
#include "esp_capture.h"

static const char *TAG = "Webrtc_Test";

static char room_url[128];

#define RUN_ASYNC(name, body)           \
    void run_async##name(void *arg)     \
    {                                   \
        body;                           \
        media_lib_thread_destroy(NULL); \
    }                                   \
    media_lib_thread_create_from_scheduler(NULL, #name, run_async##name, NULL);

char server_url[64] = "https://webrtc.espressif.com";

static void thread_scheduler(const char *thread_name, media_lib_thread_cfg_t *schedule_cfg)
{
    if (strcmp(thread_name, "venc_0") == 0) {
        // For H264 may need huge stack if use hardware encoder can set it to small value
        schedule_cfg->priority = 10;
#if CONFIG_IDF_TARGET_ESP32S3
        schedule_cfg->stack_size = 20 * 1024;
#endif
    }
#ifdef WEBRTC_SUPPORT_OPUS
    else if (strcmp(thread_name, "aenc_0") == 0) {
        // For OPUS encoder it need huge stack, when use G711 can set it to small value
        schedule_cfg->stack_size = 40 * 1024;
        schedule_cfg->priority = 10;
        schedule_cfg->core_id = 1;
    }
    else if (strcmp(thread_name, "Adec") == 0) {
        schedule_cfg->stack_size = 40 * 1024;
        schedule_cfg->priority = 10;
        schedule_cfg->core_id = 1;
    }
#endif
    else if (strcmp(thread_name, "AUD_SRC") == 0) {
        schedule_cfg->priority = 15;
    } else if (strcmp(thread_name, "pc_task") == 0) {
        schedule_cfg->stack_size = 25 * 1024;
        schedule_cfg->priority = 18;
        schedule_cfg->core_id = 1;
    }
    if (strcmp(thread_name, "start") == 0) {
        schedule_cfg->stack_size = 6 * 1024;
    }
}

static void capture_scheduler(const char *name, esp_capture_thread_schedule_cfg_t *schedule_cfg)
{
    media_lib_thread_cfg_t cfg = {
        .stack_size = schedule_cfg->stack_size,
        .priority = schedule_cfg->priority,
        .core_id = schedule_cfg->core_id,
    };
    schedule_cfg->stack_in_ext = true;
    thread_scheduler(name, &cfg);
    schedule_cfg->stack_size = cfg.stack_size;
    schedule_cfg->priority = cfg.priority;
    schedule_cfg->core_id = cfg.core_id;
}

static char* gen_room_id_use_mac(void)
{
    static char room_mac[24];
    uint8_t mac[6];
    uint32_t random_suffix;
    network_get_mac(mac);
    esp_fill_random(&random_suffix, sizeof(random_suffix));
    snprintf(room_mac, sizeof(room_mac)-1, "esp_%02x%02x%02x_%04x",
             mac[3], mac[4], mac[5], (unsigned int)(random_suffix & 0xFFFF));
    return room_mac;
}

static int network_event_handler(bool connected)
{
    if (connected) {
        // Enter into Room directly
        RUN_ASYNC(start, {
            char *room = gen_room_id_use_mac();
            snprintf(room_url, sizeof(room_url), "%s/join/%s", server_url, room);
            ESP_LOGI(TAG, "Start to join in room %s", room);
            if (start_webrtc(room_url) == 0) {
                ESP_LOGW(TAG, "Please use browser to join in %s", room);
            }
        });
    } else {
        stop_webrtc();
    }
    return 0;
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set("Webrtc_Test", ESP_LOG_INFO);
    esp_log_level_set("WEBRTC", ESP_LOG_INFO);
    esp_log_level_set("Board", ESP_LOG_INFO);
    esp_log_level_set("AGENT", ESP_LOG_INFO);  // Enable ICE agent logging
    esp_log_level_set("PEER", ESP_LOG_INFO);   // Enable peer logging
    media_lib_add_default_adapter();
    esp_capture_set_thread_scheduler(capture_scheduler);
    media_lib_thread_set_schedule_cb(thread_scheduler);
    init_board();
    media_sys_buildup();
    network_init(WIFI_SSID, WIFI_PASSWORD, network_event_handler);
    while (1) {
        media_lib_thread_sleep(2000);
        query_webrtc();
    }
}
