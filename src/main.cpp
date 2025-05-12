

#include <Arduino.h>


#include "tools.h"
#include "wifi_mesh.h"
#include <esp_wifi.h>
#include <esp_netif.h>
#include "lwip/ip_addr.h"
#include <sys/socket.h>
# include <esp_mesh_lite.h>

static int g_sockfd    = -1;

static int socket_tcp_client_create(const char *ip, uint16_t port)
{
    ESP_LOGD(TAG, "Create a tcp client, ip: %s, port: %d", ip, port);

    esp_err_t ret = ESP_OK;
    int sockfd    = -1;
    struct ifreq iface;
    memset(&iface, 0x0, sizeof(iface));
    struct sockaddr_in server_addr = {
        .sin_len = sizeof(struct sockaddr_in),
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = {
          .s_addr = inet_addr(ip),
        },
        .sin_zero = {0},
    };

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ESP_LOGE(TAG, "socket create, sockfd: %d", sockfd);
        goto ERR_EXIT;
    }

    esp_netif_get_netif_impl_name(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), iface.ifr_name);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, &iface, sizeof(struct ifreq)) != 0) {
        ESP_LOGE(TAG, "Bind [sock=%d] to interface %s fail", sockfd, iface.ifr_name);
    }

    ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        ESP_LOGD(TAG, "socket connect, ret: %d, ip: %s, port: %d", ret, ip, port);
        goto ERR_EXIT;
    }
    return sockfd;

ERR_EXIT:

    if (sockfd != -1) {
        close(sockfd);
    }

    return -1;
}

void tcp_client_write_task(void *arg)
{
    size_t size        = 0;
    int count          = 0;
    char *data         = NULL;
    esp_err_t ret      = ESP_OK;
    uint8_t sta_mac[6] = {0};

    esp_wifi_get_mac(WIFI_IF_STA, sta_mac);

    ESP_LOGI(TAG, "TCP client write task is running");

    while (1) {
        if (g_sockfd == -1) {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            g_sockfd = socket_tcp_client_create(CONFIG_SERVER_IP, CONFIG_SERVER_PORT);
            if (g_sockfd == -1) {
                ESP_LOGE(TAG, "Create TCP client failed");
                continue;
            }
            continue;
        }

        vTaskDelay(3000 / portTICK_PERIOD_MS);

        size = asprintf(&data, "{\"src_addr\": \"" MACSTR "\",\"data\": \"Hello TCP Server!\",\"level\": %d,\"count\": %d}\r\n",
                        MAC2STR(sta_mac), esp_mesh_lite_get_level(), count++);

        ESP_LOGD(TAG, "TCP write, size: %d, data: %s", size, data);
        ret = write(g_sockfd, data, size);
        free(data);

        if (ret <= 0) {
            ESP_LOGE(TAG, "<%s> TCP write", strerror(errno));
            close(g_sockfd);
            g_sockfd = -1;
            continue;
        }
    }

    ESP_LOGI(TAG, "TCP client write task is exit");

    close(g_sockfd);
    g_sockfd = -1;
    if (data) {
        free(data);
    }
    vTaskDelete(NULL);
}

static void ip_event_sta_got_ip_handler(void *arg, esp_event_base_t event_base,
  int32_t event_id, void *event_data)
{
  static bool tcp_task = false;
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));


    if (!tcp_task) {
        xTaskCreate(tcp_client_write_task, "tcp_client_write_task", 4 * 1024, NULL, 5, NULL);
        tcp_task = true;
    }
}

// setup function
void setup() {
#ifdef DEBUG_ENABLE
  delay(1000); //wait for serial monitor to start
#endif
  Serial.begin(115200);
  PRINT( "CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH: "); PRINTLN( CONFIG_FREERTOS_TIMER_TASK_STACK_DEPTH);

  PRINT_RAM("== Setup");
  PRINT("CPU reset:  "); PRINTLN(esp_reset_reason()); //4-PANIC (stack overflow, invalid memory access); 6-TASK_WDT (code stuck)

  wifi_mesh_init(); //init mesh network
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_sta_got_ip_handler, NULL, NULL));
  PRINT_RAM("Setup done");
}

// loop function
void loop() {
 
  delay(999); //delay slightly less than 1s
}


#if CONFIG_LWIP_HOOK_IP6_INPUT_CUSTOM
extern "C" int lwip_hook_ip6_input(struct pbuf *p, struct netif *inp) __attribute__((weak));
extern "C" int lwip_hook_ip6_input(struct pbuf *p, struct netif *inp) {
  if (ip6_addr_isany_val(inp->ip6_addr[0].u_addr.ip6)) {
    // We don't have an LL address -> eat this packet here, so it won't get accepted on input netif
    pbuf_free(p);
    return 1;
  }
  return 0;
}
#endif