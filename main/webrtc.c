/* WebRTC Video Feed application code

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

#include "esp_webrtc.h"
#include "media_lib_os.h"
#include "common.h"
#include "esp_log.h"
#include "esp_webrtc_defaults.h"
#include "esp_peer_default.h"

#define TAG "WEBRTC"

static esp_webrtc_handle_t webrtc;

extern const uint8_t join_music_start[] asm("_binary_join_aac_start");
extern const uint8_t join_music_end[] asm("_binary_join_aac_end");

static void play_join_sound(void)
{

}

static int webrtc_event_handler(esp_webrtc_event_t *event, void *ctx)
{
    if (event->type == ESP_WEBRTC_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "WebRTC peer connected");
        play_join_sound();
    } else if (event->type == ESP_WEBRTC_EVENT_CONNECT_FAILED) {
        ESP_LOGI(TAG, "WebRTC connection failed");
    } else if (event->type == ESP_WEBRTC_EVENT_DISCONNECTED) {
        ESP_LOGI(TAG, "WebRTC peer disconnected");
        play_join_sound();
    }
    return 0;
}

int start_webrtc(char *url)
{
    if (network_is_connected() == false) {
        ESP_LOGE(TAG, "Wifi not connected yet");
        return -1;
    }
    if (url[0] == 0) {
        ESP_LOGE(TAG, "Room Url not set yet");
        return -1;
    }
    if (webrtc) {
        esp_webrtc_close(webrtc);
        webrtc = NULL;
    }

    esp_peer_default_cfg_t peer_cfg = {
        .agent_recv_timeout = 2000,  // Increased timeout for slower networks
    };
    esp_webrtc_cfg_t cfg = {
        .peer_cfg = {
            .audio_info = {
#ifdef WEBRTC_SUPPORT_OPUS
                .codec = ESP_PEER_AUDIO_CODEC_OPUS,
                .sample_rate = 16000,
                .channel = 2,
#else
                .codec = ESP_PEER_AUDIO_CODEC_G711A,
#endif
            },
            .video_info = {
                .codec = ESP_PEER_VIDEO_CODEC_H264,
                .width = VIDEO_WIDTH,
                .height = VIDEO_HEIGHT,
                .fps = VIDEO_FPS,
            },
            .audio_dir = ESP_PEER_MEDIA_DIR_SEND_RECV,
            .video_dir = ESP_PEER_MEDIA_DIR_SEND_ONLY,
            .enable_data_channel = DATA_CHANNEL_ENABLED,
            .no_auto_reconnect = true,
            .extra_cfg = &peer_cfg,
            .extra_size = sizeof(peer_cfg),
        },
        .signaling_cfg = {
            .signal_url = url,
        },
        .peer_impl = esp_peer_get_default_impl(),
        .signaling_impl = esp_signaling_get_apprtc_impl(),
    };
    int ret = esp_webrtc_open(&cfg, &webrtc);
    if (ret != 0) {
        ESP_LOGE(TAG, "Fail to open webrtc");
        return ret;
    }
    // Set media provider
    esp_webrtc_media_provider_t media_provider = {};
    media_sys_get_provider(&media_provider);
    esp_webrtc_set_media_provider(webrtc, &media_provider);

    // Set event handler
    esp_webrtc_set_event_handler(webrtc, webrtc_event_handler, NULL);

    // Start webrtc first to fetch ICE credentials
    ret = esp_webrtc_start(webrtc);
    if (ret != 0) {
        ESP_LOGE(TAG, "Fail to start webrtc");
        return ret;
    }
    ESP_LOGI(TAG, "WebRTC signaling started successfully");

    // Give signaling a brief moment to connect and fetch ICE credentials
    // The ICE fetch is asynchronous, and enabling peer connection before
    // ICE info is available will cause an error (though it recovers automatically)
    media_lib_thread_sleep(100);

    // Enable peer connection after signaling starts
    // This allows ICE info to be fetched before attempting connection
    esp_webrtc_enable_peer_connection(webrtc, true);

    play_join_sound();
    return ret;
}

void query_webrtc(void)
{
    if (webrtc) {
        esp_webrtc_query(webrtc);
    }
}

int stop_webrtc(void)
{
    if (webrtc) {
        esp_webrtc_handle_t handle = webrtc;
        webrtc = NULL;
        ESP_LOGI(TAG, "Start to close webrtc %p", handle);
        esp_webrtc_close(handle);
    }
    return 0;
}
