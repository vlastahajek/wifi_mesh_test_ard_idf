#include <Arduino.h>
#include <esp_idf_version.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0) && defined(ESP_MESH_LITE_ENABLE)
# include <esp_bridge.h>
# include <esp_mesh_lite.h>
#endif
#include "wifi_mesh.h"
#include "tools.h"

const char *TAG = "local_control";

static uint8_t primary                 = 0;
static wifi_second_chan_t second       =  WIFI_SECOND_CHAN_NONE;
static uint8_t sta_mac[6];
static wifi_ap_record_t ap_info;
static wifi_sta_list_t wifi_sta_list;

static long getTaskFreeStackSize(TaskHandle_t task) {
    return uxTaskGetStackHighWaterMark(task) * 4;
}
    
static void print_system_info_timercb(TimerHandle_t timer)
{
    char *tn = pcTaskGetName(NULL);
    ESP_LOGI(TAG, "Task name %s ", tn);
    ESP_LOGI(TAG, "Free stack size: %" PRIu32 "", getTaskFreeStackSize(NULL));
    memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
    memset(&ap_info, 0, sizeof(wifi_ap_record_t)); 
    memset(sta_mac, 0, sizeof(sta_mac));
    ESP_LOGI(TAG, "Free stack size: %" PRIu32 "", getTaskFreeStackSize(NULL));
    esp_wifi_sta_get_ap_info(&ap_info);
    esp_wifi_get_mac(WIFI_IF_STA, sta_mac);
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    esp_wifi_get_channel(&primary, &second);
    ESP_LOGI(TAG, "Free stack size: %" PRIu32 "", getTaskFreeStackSize(NULL));
    ESP_LOGI(TAG, "System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR ", parent rssi: %d, free heap: %" PRIu32 "", primary,
             esp_mesh_lite_get_level(), MAC2STR(sta_mac), MAC2STR(ap_info.bssid),
             (ap_info.rssi != 0 ? ap_info.rssi : -120), esp_get_free_heap_size());
    ESP_LOGI(TAG, "Free stack size: %" PRIu32 "", getTaskFreeStackSize(NULL));
#if CONFIG_MESH_LITE_NODE_INFO_REPORT
    ESP_LOGI(TAG, "All node number: %"PRIu32"", esp_mesh_lite_get_mesh_node_number());
#endif /* MESH_LITE_NODE_INFO_REPORT */
    for (int i = 0; i < wifi_sta_list.num; i++) {
        ESP_LOGI(TAG, "Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
    }
    ESP_LOGI(TAG, "Free stack size: %" PRIu32 "", getTaskFreeStackSize(NULL));
}



static size_t _wifi_strncpy(char *dst, const char *src, size_t dst_len) {
  if (!dst || !src || !dst_len) {
    return 0;
  }
  size_t src_len = strlen(src);
  if (src_len >= dst_len) {
    src_len = dst_len;
  } else {
    src_len += 1;
  }
  memcpy(dst, src, src_len);
  return src_len;
}

bool wifi_mesh_is_connected(void) {
    wifi_ap_record_t ap_info;
    memset(&ap_info, 0, sizeof(wifi_ap_record_t)); 
    esp_wifi_sta_get_ap_info(&ap_info);
    if (ap_info.rssi != 0) {
        return true;
    } else {
        return false;
    }
} 

static void wifi_init(void)
{
    // Station
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));
    _wifi_strncpy((char *)wifi_config.sta.ssid, CONFIG_ROUTER_SSID, 32);
    _wifi_strncpy((char *)wifi_config.sta.password, CONFIG_ROUTER_PASSWORD, 64);
    esp_bridge_wifi_set_config(WIFI_IF_STA, &wifi_config);

    memset(&wifi_config, 0, sizeof(wifi_config_t));
    // Softap
    _wifi_strncpy((char *)wifi_config.ap.ssid, CONFIG_BRIDGE_SOFTAP_SSID, 32);
    _wifi_strncpy((char *)wifi_config.ap.password, CONFIG_BRIDGE_SOFTAP_PASSWORD, 64);
    esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_config);
}


bool wifi_mesh_init(void) {
    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);

    // Those two lines are normally called by Arduino network framework
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_bridge_create_all_netif();

    wifi_init();

    esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
    esp_mesh_lite_init(&mesh_lite_config);

    //app_wifi_set_softap_info();

    esp_mesh_lite_start();
    PRINTLN("Mesh-Lite started.");
    /**
     * @breif Create handler
     */
    

    TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_PERIOD_MS,
                                       true, NULL, print_system_info_timercb);
    xTimerStart(timer, 0);
    return true;
}
