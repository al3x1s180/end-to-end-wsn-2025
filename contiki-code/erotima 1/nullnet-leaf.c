#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/linkaddr.h"
#include "sys/log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
void float2str(double value, char *buf);  // Προώθηση δήλωσης


#define LOG_MODULE "Leaf"
#define LOG_LEVEL LOG_LEVEL_INFO

#define SEND_INTERVAL (20 * CLOCK_SECOND)

PROCESS(nullnet_unicast_process, "NullNet Unicast (Leaf)");
AUTOSTART_PROCESSES(&nullnet_unicast_process);

static struct etimer periodic_timer;
static uint8_t assigned_id = 0;
static uint8_t measurement_count = 1;

typedef struct {
  uint8_t id;
  uint8_t count;
  int temperature_raw;
  int humidity_raw;
} measurement_packet_t;

void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest) {
  if(len == sizeof(uint8_t)) {
    assigned_id = *(uint8_t *)data;
    LOG_INFO("Received ID from parent: %u\n", assigned_id);
  }
}
PROCESS_THREAD(nullnet_unicast_process, ev, data_ev) {
  PROCESS_BEGIN();

  // Hardcoded διεύθυνση MAC του πατέρα – αντικατέστησέ την με την πραγματική
  static linkaddr_t parent_addr = {{0x00, 0x12, 0x4b, 0x00, 0x01, 0x23, 0x45, 0x67}};

  // Εγγραφή callback
  nullnet_set_input_callback(input_callback);

  // Στείλε μήνυμα στον πατέρα για να πάρεις ID
  static char msg[] = "I'm a leaf node!";
  nullnet_buf = (uint8_t *)msg;
  nullnet_len = strlen(msg) + 1;
  NETSTACK_NETWORK.output(&parent_addr);

  LOG_INFO("Waiting for ID assignment...\n");

  while(!assigned_id) {
    PROCESS_PAUSE();
  }

  LOG_INFO("Starting measurements transmission:\n");
  etimer_set(&periodic_timer, SEND_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    // Δημιουργία μετρήσεων (mock αισθητήρες)
    int temp_raw = 2345 + rand() % 100;   // integer value
    int hum_raw = 5890 + rand() % 100;    // integer value

    double temp = temperature_int2double(temp_raw);
    double hum = humidity_int2double(hum_raw);

    measurement_packet_t packet = {
      .id = assigned_id,
      .count = measurement_count++,
      .temperature_raw = temp_raw,
      .humidity_raw = hum_raw
    };
    // Εκτύπωση για το terminal (debug)
    char temp_str[10], hum_str[10];
    float2str(temp, temp_str);
    float2str(hum, hum_str);
    LOG_INFO("ID: %u Count: %u Temperature: %s C Humidity: %s%%\n",
             packet.id, packet.count, temp_str, hum_str);

    // Αποστολή στον πατέρα
    nullnet_buf = (uint8_t *)&packet;
    nullnet_len = sizeof(packet);
    NETSTACK_NETWORK.output(&parent_addr);

    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}

