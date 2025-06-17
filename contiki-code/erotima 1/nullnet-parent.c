#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/linkaddr.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "utils.h"

#define MAX_NODES 10

typedef struct {
  uint8_t id;
  uint16_t count;
  int16_t temperature;
  int16_t humidity;
} sensor_data_t;

static linkaddr_t known_nodes[MAX_NODES];
static uint8_t assigned_ids[MAX_NODES];
static uint8_t next_id = 1;

PROCESS(broadcast_parent, "Broadcast & Receive");
AUTOSTART_PROCESSES(&broadcast_parent);

static void input_callback(const void *data, uint16_t len,
                           const linkaddr_t *src, const linkaddr_t *dest)
{
  if(len == sizeof(sensor_data_t)) {
    sensor_data_t received;
    memcpy(&received, data, len);

    char temp_str[8], hum_str[8];
    float2str(temperature_int2double(received.temperature), temp_str);
    float2str(humidity_int2double(received.humidity), hum_str);

    printf("ID: %u | Count: %u | Temp: %s | Hum: %s\n",
           received.id, received.count, temp_str, hum_str);
  }
  else {
    // Μήνυμα από νέο κόμβο για ανάθεση ID
    int found = 0;
    for(int i = 0; i < MAX_NODES; i++) {
      if(linkaddr_cmp(&known_nodes[i], src)) {
        found = 1;
        break;
      }
    }

    if(!found && next_id <= MAX_NODES) {
      printf("Assigning ID %u to node %02x:%02x\n", next_id, src->u8[6], src->u8[7]);
      known_nodes[next_id-1] = *src;
      assigned_ids[next_id-1] = next_id;
      nullnet_buf = (uint8_t *)&assigned_ids[next_id-1];
      nullnet_len = sizeof(uint8_t);
      NETSTACK_NETWORK.output(src);
      next_id++;
    }
  }
}

PROCESS_THREAD(broadcast_parent, ev, data_ev)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  nullnet_set_input_callback(input_callback);

  etimer_set(&timer, CLOCK_SECOND * 10);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    printf("I’m the parent node, my MAC is: ");
    for(int i = 0; i < LINKADDR_SIZE; i++) {
      printf("%02x", linkaddr_node_addr.u8[i]);
      if(i < LINKADDR_SIZE - 1) printf(":");
    }
    printf("\n");

    etimer_reset(&timer);
  }

  PROCESS_END();
}

