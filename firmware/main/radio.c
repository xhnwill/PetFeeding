#include "app.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "store/config/ble_store_config.h"
#include <stdio.h>
#include <string.h>
static const ble_uuid128_t svc =
    BLE_UUID128_INIT(0x10, 0xa0, 0x44, 0x21, 0xd1, 0x73, 0xf2, 0x91, 0x40, 0x46,
                     0x3a, 0x63, 0x00, 0x00, 0x50, 0x4d);
static const ble_uuid128_t state_uuid =
    BLE_UUID128_INIT(0x10, 0xa0, 0x44, 0x21, 0xd1, 0x73, 0xf2, 0x91, 0x40, 0x46,
                     0x3a, 0x63, 0x01, 0x00, 0x50, 0x4d);
static const ble_uuid128_t command_uuid =
    BLE_UUID128_INIT(0x10, 0xa0, 0x44, 0x21, 0xd1, 0x73, 0xf2, 0x91, 0x40, 0x46,
                     0x3a, 0x63, 0x02, 0x00, 0x50, 0x4d);
static const ble_uuid128_t visit_uuid =
    BLE_UUID128_INIT(0x10, 0xa0, 0x44, 0x21, 0xd1, 0x73, 0xf2, 0x91, 0x40, 0x46,
                     0x3a, 0x63, 0x03, 0x00, 0x50, 0x4d);
static uint8_t addr_type;
static bool ready, window, friends, central, visit_sent;
static uint16_t connection = BLE_HS_CONN_HANDLE_NONE, remote_value;
static uint16_t remote_visit;
static char visit_message[128], peer_message[128];
static size_t sent, peer_used;
static void summary(char *out, size_t n) {
  snprintf(
      out, n,
      "{\"action\":\"visit\",\"friend\":%lu,\"species\":%lu,\"color\":%lu}",
      (unsigned long)pet.genes[0], (unsigned long)(pet.genes[2] % 3),
      (unsigned long)(pet.genes[3] % 4));
}
static int64_t deadline;
static ble_addr_t peer;
static char input[512];
static size_t used;
unsigned pairing_number;
bool pairing_pending, friend_found;
static int gap(struct ble_gap_event *e, void *arg);
static void advertise(void) {
  if (!ready || !window || connection != BLE_HS_CONN_HANDLE_NONE)
    return;
  struct ble_hs_adv_fields f = {0};
  f.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
  f.uuids128 = (ble_uuid128_t *)&svc;
  f.num_uuids128 = 1;
  f.uuids128_is_complete = 1;
  uint8_t mode[3] = {0xff, 0xff, friends ? 1 : 0};
  f.mfg_data = mode;
  f.mfg_data_len = 3;
  ble_gap_adv_set_fields(&f);
  struct ble_hs_adv_fields rsp = {0};
  rsp.name = (uint8_t *)"MyPat";
  rsp.name_len = 5;
  rsp.name_is_complete = 1;
  ble_gap_adv_rsp_set_fields(&rsp);
  struct ble_gap_adv_params p = {.conn_mode = BLE_GAP_CONN_MODE_UND,
                                 .disc_mode = BLE_GAP_DISC_MODE_GEN};
  ble_gap_adv_start(addr_type, NULL, BLE_HS_FOREVER, &p, gap, NULL);
}
static int access_cb(uint16_t conn, uint16_t attr,
                     struct ble_gatt_access_ctxt *ctx, void *arg) {
  if (ctx->op == BLE_GATT_ACCESS_OP_READ_CHR) {
    if (arg) {
      char data[128];
      summary(data, sizeof(data));
      return os_mbuf_append(ctx->om, data, strlen(data));
    }
    char state[1024];
    app_state(state, sizeof(state), !friends);
    return os_mbuf_append(ctx->om, state, strlen(state))
               ? BLE_ATT_ERR_INSUFFICIENT_RES
               : 0;
  }
  uint16_t n = OS_MBUF_PKTLEN(ctx->om);
  if (!n || n > sizeof(input) - 1 - used) {
    used = 0;
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  }
  uint8_t chunk[512];
  os_mbuf_copydata(ctx->om, 0, n, chunk);
  for (int i = 0; i < n; i++) {
    if (chunk[i] == '\n') {
      input[used] = 0;
      if (!app_queue(input, !friends)) {
        used = 0;
        return BLE_ATT_ERR_INSUFFICIENT_RES;
      }
      used = 0;
    } else if (chunk[i] >= 32)
      input[used++] = (char)chunk[i];
    else {
      used = 0;
      return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
  }
  return 0;
}
static const struct ble_gatt_svc_def services[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &svc.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {.uuid = &state_uuid.u,
              .access_cb = access_cb,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC |
                       BLE_GATT_CHR_F_READ_AUTHEN},
             {.uuid = &command_uuid.u,
              .access_cb = access_cb,
              .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC |
                       BLE_GATT_CHR_F_WRITE_AUTHEN},
             {.uuid = &visit_uuid.u,
              .access_cb = access_cb,
              .arg = (void *)1,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC |
                       BLE_GATT_CHR_F_READ_AUTHEN},
             {0}}},
    {0}};
static int peer_read(uint16_t c, const struct ble_gatt_error *error,
                     struct ble_gatt_attr *attr, void *arg) {
  if (!error->status && attr) {
    size_t n = OS_MBUF_PKTLEN(attr->om);
    if (peer_used + n >= sizeof(peer_message))
      return BLE_HS_EINVAL;
    os_mbuf_copydata(attr->om, 0, n, peer_message + peer_used);
    peer_used += n;
  } else if (error->status == BLE_HS_EDONE) {
    peer_message[peer_used] = 0;
    app_queue(peer_message, false);
  }
  return 0;
}
static int visit_done(uint16_t c, const struct ble_gatt_error *error,
                      struct ble_gatt_attr *attr, void *arg) {
  if (error->status == 0) {
    size_t total = strlen(visit_message);
    if (sent < total) {
      size_t n = total - sent;
      if (n > 18)
        n = 18;
      size_t offset = sent;
      sent += n;
      ble_gattc_write_flat(c, remote_value, visit_message + offset, n,
                           visit_done, NULL);
    } else if (!visit_sent) {
      visit_sent = true;
      peer_used = 0;
      ble_gattc_read_long(c, remote_visit, 0, peer_read, NULL);
    }
  }
  return 0;
}
static int characteristic(uint16_t c, const struct ble_gatt_error *err,
                          const struct ble_gatt_chr *chr, void *arg) {
  if (!err->status && chr) {
    if (!ble_uuid_cmp(&chr->uuid.u, &command_uuid.u))
      remote_value = chr->val_handle;
    if (!ble_uuid_cmp(&chr->uuid.u, &visit_uuid.u))
      remote_visit = chr->val_handle;
    return 0;
  }
  if (err->status == BLE_HS_EDONE && remote_value && remote_visit) {
    summary(visit_message, sizeof(visit_message) - 2);
    strcat(visit_message, "\n");
    sent = 0;
    struct ble_gatt_error success = {0};
    visit_done(c, &success, NULL, NULL);
  }
  return 0;
}
static int gap(struct ble_gap_event *e, void *arg) {
  switch (e->type) {
  case BLE_GAP_EVENT_DISC: {
    if (!friends || friend_found)
      return 0;
    struct ble_hs_adv_fields f;
    if (!ble_hs_adv_parse_fields(&f, e->disc.data, e->disc.length_data) &&
        f.mfg_data_len == 3 && f.mfg_data[0] == 255 && f.mfg_data[1] == 255 &&
        f.mfg_data[2] == 1)
      for (int i = 0; i < f.num_uuids128; i++)
        if (!ble_uuid_cmp(&f.uuids128[i].u, &svc.u)) {
          peer = e->disc.addr;
          friend_found = true;
        }
    break;
  }
  case BLE_GAP_EVENT_CONNECT:
    if (e->connect.status) {
      central = false;
      advertise();
      break;
    }
    connection = e->connect.conn_handle;
    used = 0;
    visit_sent = false;
    ble_gap_disc_cancel();
    ble_gap_security_initiate(connection);
    break;
  case BLE_GAP_EVENT_DISCONNECT:
    connection = BLE_HS_CONN_HANDLE_NONE;
    pairing_pending = false;
    used = 0;
    central = false;
    advertise();
    break;
  case BLE_GAP_EVENT_PASSKEY_ACTION:
    pairing_number = e->passkey.params.action == BLE_SM_IOACT_NUMCMP
                         ? e->passkey.params.numcmp
                         : 0;
    if (e->passkey.params.action == BLE_SM_IOACT_DISP) {
      pairing_number = 100000 + esp_random() % 900000;
      struct ble_sm_io io = {.action = BLE_SM_IOACT_DISP,
                             .passkey = pairing_number};
      ble_sm_inject_io(connection, &io);
    } else if (e->passkey.params.action == BLE_SM_IOACT_NUMCMP)
      pairing_pending = true;
    break;
  case BLE_GAP_EVENT_ENC_CHANGE:
    if (!e->enc_change.status && central) {
      remote_value = remote_visit = 0;
      ble_gattc_disc_all_chrs(connection, 1, 0xffff, characteristic, NULL);
    }
    break;
  case BLE_GAP_EVENT_ADV_COMPLETE:
    advertise();
    break;
  default:
    break;
  }
  return 0;
}
void radio_confirm(bool accept) {
  if (pairing_pending) {
    struct ble_sm_io io = {.action = BLE_SM_IOACT_NUMCMP,
                           .numcmp_accept = accept};
    ble_sm_inject_io(connection, &io);
    pairing_pending = false;
  }
}
static void sync_cb(void) {
  ble_hs_util_ensure_addr(0);
  ble_hs_id_infer_auto(0, &addr_type);
  ready = true;
}
static void host(void *arg) {
  nimble_port_run();
  nimble_port_freertos_deinit();
}
void radio_init(void) {
  ESP_ERROR_CHECK(nimble_port_init());
  ble_hs_cfg.sync_cb = sync_cb;
  ble_hs_cfg.sm_io_cap = BLE_HS_IO_DISPLAY_YESNO;
  ble_hs_cfg.sm_bonding = 0;
  ble_hs_cfg.sm_mitm = 1;
  ble_hs_cfg.sm_sc = 1;
  ble_svc_gap_init();
  ble_svc_gatt_init();
  ble_svc_gap_device_name_set("MyPat");
  ble_gatts_count_cfg(services);
  ble_gatts_add_svcs(services);
  nimble_port_freertos_init(host);
}
void radio_stop(void) {
  window = false;
  ble_gap_disc_cancel();
  ble_gap_adv_stop();
  if (connection != BLE_HS_CONN_HANDLE_NONE)
    ble_gap_terminate(connection, BLE_ERR_REM_USER_CONN_TERM);
  pairing_pending = false;
  friend_found = false;
  pairing_number = 0;
}
void radio_window(bool is_friend) {
  radio_stop();
  friends = is_friend;
  window = true;
  deadline = esp_timer_get_time() + (friends ? 60000000LL : 180000000LL);
  advertise();
  if (friends) {
    struct ble_gap_disc_params p = {
        .passive = 1, .itvl = 0x80, .window = 0x30, .filter_duplicates = 1};
    ble_gap_disc(addr_type, 60000, &p, gap, NULL);
  }
}
void radio_connect_friend(void) {
  if (!friend_found || connection != BLE_HS_CONN_HANDLE_NONE)
    return;
  uint8_t local[6];
  ble_hs_id_copy_addr(addr_type, local, NULL);
  /* Both users can press OK: the lexicographically smaller radio address
   * initiates. */
  if (memcmp(local, peer.val, 6) > 0)
    return;
  ble_gap_disc_cancel();
  ble_gap_adv_stop();
  central = true;
  if (ble_gap_connect(addr_type, &peer, 10000, NULL, gap, NULL)) {
    central = false;
    advertise();
  }
}
bool radio_busy(void) { return window; }
void radio_poll(void) {
  if (window && esp_timer_get_time() > deadline)
    radio_stop();
}
