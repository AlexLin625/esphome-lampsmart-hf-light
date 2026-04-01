#include "ble_advertiser.h"

#include <cassert>
#include <cstring>

#include "esp_log.h"
#include "nvs_flash.h"

namespace esphome {
namespace lampsmarthf {

static const char *const TAG = "lampsmart_hf_adv";

std::vector<uint8_t> BleAdvertiser::msd_data_;
uint8_t BleAdvertiser::own_addr_type_;
bool BleAdvertiser::advertising_ = false;
bool BleAdvertiser::synced_ = false;
bool BleAdvertiser::initialized_ = false;

BleAdvertiser &BleAdvertiser::get_instance() {
  static BleAdvertiser instance;
  return instance;
}

esp_err_t BleAdvertiser::init() {
  if (initialized_) {
    return ESP_OK;
  }

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to init NVS: %d", ret);
    return ret;
  }

  ret = nimble_port_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to init NimBLE: %d", ret);
    return ret;
  }

  ble_hs_cfg.reset_cb = &BleAdvertiser::on_reset_;
  ble_hs_cfg.sync_cb = &BleAdvertiser::on_sync_;

  nimble_port_freertos_init(&BleAdvertiser::host_task_);
  initialized_ = true;
  return ESP_OK;
}

void BleAdvertiser::host_task_(void *param) {
  (void) param;
  ESP_LOGI(TAG, "BLE host task started");
  nimble_port_run();
  nimble_port_freertos_deinit();
  initialized_ = false;
}

void BleAdvertiser::on_reset_(int reason) {
  ESP_LOGW(TAG, "BLE reset: %d", reason);
  synced_ = false;
  advertising_ = false;
}

void BleAdvertiser::on_sync_() {
  int rc = ble_hs_util_ensure_addr(0);
  assert(rc == 0);

  rc = ble_hs_id_infer_auto(0, &own_addr_type_);
  if (rc != 0) {
    ESP_LOGE(TAG, "Failed to infer BLE address type: %d", rc);
    return;
  }

  synced_ = true;
  if (!msd_data_.empty()) {
    advertise_();
  }
}

static int gap_event(struct ble_gap_event *event, void *arg) {
  (void) event;
  (void) arg;
  return 0;
}

void BleAdvertiser::start_advertising(const std::vector<uint8_t> &msd_data) {
  msd_data_.clear();
  msd_data_.push_back(0xFF);
  msd_data_.push_back(0xFF);
  msd_data_.push_back(0xFF);
  msd_data_.insert(msd_data_.end(), msd_data.begin(), msd_data.end());

  advertising_ = false;
  if (ble_hs_synced()) {
    advertise_();
  }
}

esp_err_t BleAdvertiser::stop_advertising() {
  const int rc = ble_gap_adv_stop();
  if (rc != 0) {
    ESP_LOGW(TAG, "Unable to stop advertising: %d", rc);
    return ESP_FAIL;
  }
  advertising_ = false;
  return ESP_OK;
}

void BleAdvertiser::advertise_() {
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields fields;

  std::memset(&fields, 0, sizeof(fields));
  fields.flags = 0;
  fields.tx_pwr_lvl_is_present = 0;
  fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

  if (!msd_data_.empty()) {
    fields.mfg_data = msd_data_.data();
    fields.mfg_data_len = msd_data_.size();
  }

  int rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0) {
    ESP_LOGE(TAG, "Failed to set advertising fields: %d", rc);
    advertising_ = false;
    return;
  }

  ble_gap_adv_stop();

  std::memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  rc = ble_gap_adv_start(own_addr_type_, nullptr, BLE_HS_FOREVER, &adv_params, gap_event, nullptr);
  if (rc != 0) {
    ESP_LOGE(TAG, "Failed to start advertising: %d", rc);
    advertising_ = false;
    return;
  }

  advertising_ = true;
  ESP_LOGD(TAG, "Advertising started with %u bytes of manufacturer data", static_cast<unsigned>(msd_data_.size()));
}

bool BleAdvertiser::is_ready() const { return synced_ && ble_hs_synced(); }

bool BleAdvertiser::is_advertising() const { return advertising_; }

}  // namespace lampsmarthf
}  // namespace esphome
