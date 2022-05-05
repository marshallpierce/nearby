// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "internal/platform/implementation/windows/ble_v2.h"

#include <iostream>
#include <memory>
#include <string>

#include "absl/synchronization/mutex.h"
#include "internal/platform/implementation/ble_v2.h"
#include "internal/platform/implementation/shared/count_down_latch.h"
#include "internal/platform/logging.h"
#include "winrt/Windows.Devices.Bluetooth.Advertisement.h"
#include "winrt/Windows.Devices.Bluetooth.h"
#include "winrt/Windows.Foundation.Collections.h"

namespace location {
namespace nearby {
namespace windows {

namespace {

using ::location::nearby::api::ble_v2::BleAdvertisementData;
using ::location::nearby::api::ble_v2::BleSocket;
using ::location::nearby::api::ble_v2::BleSocketLifeCycleCallback;
using ::location::nearby::api::ble_v2::ClientGattConnection;
using ::location::nearby::api::ble_v2::PowerMode;
using ::location::nearby::api::ble_v2::ServerGattConnectionCallback;
using ::winrt::Windows::Devices::Bluetooth::BluetoothError;
using ::winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisement;
using ::winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisementDataSection;
using ::winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisementDataTypes;
using ::winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisementPublisher;
using ::winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisementPublisherStatus;
using ::winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisementPublisherStatusChangedEventArgs;
using ::winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisementReceivedEventArgs;
using ::winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisementWatcher;
using ::winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisementWatcherStatus;
using ::winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisementWatcherStoppedEventArgs;
using ::winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEScanningMode;
using ::winrt::Windows::Storage::Streams::DataWriter;

template <typename T>
using IVector = winrt::Windows::Foundation::Collections::IVector<T>;

std::string PowerModeToName(PowerMode power_mode) {
  switch (power_mode) {
    case PowerMode::kUltraLow:
      return "UltraLow";
    case PowerMode::kLow:
      return "Low";
    case PowerMode::kMedium:
      return "Medium";
    case PowerMode::kHigh:
      return "High";
    case PowerMode::kUnknown:
      return "Unknown";
  }
}

}  // namespace

std::string BleV2Peripheral::GetId() const { return ""; }

BleV2Medium::BleV2Medium(api::BluetoothAdapter& adapter)
    : adapter_(dynamic_cast<BluetoothAdapter*>(&adapter)) {}

BleV2Medium::~BleV2Medium() {}

// TODO(edwinwu): Modify advertising abstraction APIs and fit all data into one
// advertisement packet and populate accordingly
bool BleV2Medium::StartAdvertising(
    const BleAdvertisementData& advertising_data,
    const BleAdvertisementData& scan_response_data, PowerMode power_mode) {
  NEARBY_LOGS(INFO)
      << "Windows Ble StartAdvertising:, advertising_data.service_uuids size="
      << advertising_data.service_uuids.size()
      << ", scan_response_data.service_data size="
      << scan_response_data.service_data.size()
      << ", power_mode=" << PowerModeToName(power_mode);

  absl::MutexLock lock(&mutex_);

  // (AD type 0x16) Service Data
  constexpr uint8_t kCopresenceServiceUuid[] = {0xf3, 0xfe};
  DataWriter data_writer;

  // (2 bytes) 16-bit Service UUID 0xf3fe
  data_writer.WriteByte(kCopresenceServiceUuid[1]);  // 0xfe
  data_writer.WriteByte(kCopresenceServiceUuid[0]);  // 0xf3

  // (1 byte) version [3-bits] + socket_version [3-bits] +
  // fast_advertisement_flag [1-bit] + reserved [1-bit]
  data_writer.WriteByte(0x00);

  // (1 byte) body_length
  data_writer.WriteByte(0x00);

  // (1 byte) Nearby Connection version [3-bits] + pcp [5-bits]
  data_writer.WriteByte(0x00);

  // (4 bytes) endpoint_id
  for (int i = 0; i < 4; ++i) {
    data_writer.WriteByte(0x00);
  }

  // (1 byte) endpoint_info_size
  data_writer.WriteByte(0x11);  // always 17-bytes for Fast Advertisement

  // =========endpoint_info [17-bytes]============
  // (1 byte) Nearby Share version [3-bits] + visibility [1-bit] + reserved
  // [4-bits]
  data_writer.WriteByte(0x00);

  // (2 bytes) salt
  for (int i = 0; i < 2; ++i) {
    data_writer.WriteByte(0x00);
  }

  // (14 bytes) encrypted_metadata_key
  for (int i = 0; i < 14; ++i) {
    data_writer.WriteByte(0x00);
  }
  // =========endpoint_info [17-bytes]============

  // (2 bytes) device_token
  for (int i = 0; i < 2; ++i) {
    data_writer.WriteByte(0x00);
  }

  BluetoothLEAdvertisementDataSection service_data =
      BluetoothLEAdvertisementDataSection(0x16, data_writer.DetachBuffer());

  IVector<BluetoothLEAdvertisementDataSection> data_sections =
      advertisement_.DataSections();
  data_sections.Append(service_data);
  advertisement_.DataSections() = data_sections;

  publisher_ = BluetoothLEAdvertisementPublisher(advertisement_);

  publisher_started_callback_ = [this]() {
    advertising_started_ = true;
    advertising_stopped_ = false;
  };
  publisher_error_callback_ = [this]() {
    advertising_error_ = true;
    advertising_started_ = false;
    advertising_stopped_ = false;
  };

  publisher_token_ =
      publisher_.StatusChanged({this, &BleV2Medium::PublisherHandler});

  publisher_.Start();

  while (!advertising_started_) {
    if (advertising_error_) {
      return false;
    }
    if (publisher_.Status() ==
        BluetoothLEAdvertisementPublisherStatus::Started) {
      return true;
    }
  }

  return true;
}

bool BleV2Medium::StopAdvertising() {
  NEARBY_LOGS(INFO) << "Windows Ble StopAdvertising";
  absl::MutexLock lock(&mutex_);
  publisher_stopped_callback_ = [this]() {
    advertising_stopped_ = true;
    advertising_started_ = false;
  };
  publisher_error_callback_ = [this]() {
    advertising_error_ = true;
    advertising_stopped_ = false;
    advertising_started_ = false;
  };

  publisher_.Stop();

  while (!advertising_stopped_) {
    if (advertising_error_) {
      return false;
    }
    if (publisher_.Status() ==
        BluetoothLEAdvertisementPublisherStatus::Stopped) {
      return true;
    }
  }
  return true;
}

bool BleV2Medium::StartScanning(const std::vector<std::string>& service_uuids,
                                PowerMode power_mode, ScanCallback callback) {
  NEARBY_LOGS(INFO) << "Windows Ble StartScanning";
  absl::MutexLock lock(&mutex_);
  watcher_started_callback_ = [this]() {
    scanning_started_ = true;
    scanning_stopped_ = false;
  };
  watcher_error_callback_ = [this]() {
    scanning_error_ = true;
    scanning_started_ = false;
    scanning_stopped_ = false;
  };

  watcher_token_ = watcher_.Stopped({this, &BleV2Medium::WatcherHandler});
  advertisement_received_token_ =
      watcher_.Received({this, &BleV2Medium::AdvertisementReceivedHandler});

  // Active mode indicates that scan request packes will be sent to query for
  // Scan Response
  watcher_.ScanningMode(BluetoothLEScanningMode::Active);
  watcher_.Start();

  while (!scanning_started_) {
    if (scanning_error_) {
      return false;
    }
    if (watcher_.Status() == BluetoothLEAdvertisementWatcherStatus::Created) {
      return true;
    }
  }
  return true;
}

bool BleV2Medium::StopScanning() {
  NEARBY_LOGS(INFO) << "Windows Ble StopScanning";
  absl::MutexLock lock(&mutex_);

  watcher_stopped_callback_ = [this]() {
    scanning_stopped_ = true;
    scanning_started_ = false;
  };
  watcher_error_callback_ = [this]() {
    scanning_error_ = true;
    scanning_stopped_ = false;
    scanning_started_ = false;
  };

  watcher_.Stop();

  while (!scanning_stopped_) {
    if (scanning_error_) {
      return false;
    }
    if (watcher_.Status() == BluetoothLEAdvertisementWatcherStatus::Stopped) {
      return true;
    }
  }
  return true;
}

std::unique_ptr<api::ble_v2::GattServer> BleV2Medium::StartGattServer(
    ServerGattConnectionCallback callback) {
  return nullptr;
}

bool BleV2Medium::StartListeningForIncomingBleSockets(
    const api::ble_v2::ServerBleSocketLifeCycleCallback& callback) {
  return false;
}

void BleV2Medium::StopListeningForIncomingBleSockets() {}

std::unique_ptr<ClientGattConnection> BleV2Medium::ConnectToGattServer(
    api::ble_v2::BlePeripheral& peripheral, Mtu mtu, PowerMode power_mode,
    api::ble_v2::ClientGattConnectionCallback callback) {
  return nullptr;
}

std::unique_ptr<BleSocket> BleV2Medium::EstablishBleSocket(
    api::ble_v2::BlePeripheral* peripheral,
    const BleSocketLifeCycleCallback& callback) {
  return nullptr;
}

void BleV2Medium::PublisherHandler(
    BluetoothLEAdvertisementPublisher publisher,
    BluetoothLEAdvertisementPublisherStatusChangedEventArgs args) {
  absl::MutexLock lock(&mutex_);
  switch (args.Status()) {
    case BluetoothLEAdvertisementPublisherStatus::Started:
      publisher_started_callback_();
      break;
    case BluetoothLEAdvertisementPublisherStatus::Stopped:
      publisher_stopped_callback_();
      publisher_.StatusChanged(publisher_token_);
      break;
    case BluetoothLEAdvertisementPublisherStatus::Aborted:
      switch (args.Error()) {
        case BluetoothError::RadioNotAvailable:
          NEARBY_LOGS(ERROR) << "Nearby BLE Medium advertising failed due to "
                                "radio not available.";
          break;
        case BluetoothError::ResourceInUse:
          NEARBY_LOGS(ERROR)
              << "Nearby BLE Medium advertising failed due to resource in use.";
          break;
        case BluetoothError::DisabledByPolicy:
          NEARBY_LOGS(ERROR) << "Nearby BLE Medium advertising failed due to "
                                "disabled by policy.";
          break;
        case BluetoothError::DisabledByUser:
          NEARBY_LOGS(ERROR) << "Nearby BLE Medium advertising failed due to "
                                "disabled by user.";
          break;
        case BluetoothError::NotSupported:
          NEARBY_LOGS(ERROR) << "Nearby BLE Medium advertising failed due to "
                                "hardware not supported.";
          break;
        case BluetoothError::TransportNotSupported:
          NEARBY_LOGS(ERROR) << "Nearby BLE Medium advertising failed due to "
                                "transport not supported.";
          break;
        case BluetoothError::ConsentRequired:
          NEARBY_LOGS(ERROR) << "Nearby BLE Medium advertising failed due to "
                                "consent required.";
          break;
        case BluetoothError::OtherError:
          NEARBY_LOGS(ERROR)
              << "Nearby BLE Medium advertising failed due to unknown errors.";
          break;
        default:
          NEARBY_LOGS(ERROR)
              << "Nearby BLE Medium advertising failed due to unknown errors.";
          break;
      }
      publisher_error_callback_();
      break;
    default:
      break;
  }
}

void BleV2Medium::WatcherHandler(
    BluetoothLEAdvertisementWatcher watcher,
    BluetoothLEAdvertisementWatcherStoppedEventArgs args) {
  absl::MutexLock lock(&mutex_);
  switch (args.Error()) {
    case BluetoothError::RadioNotAvailable:
      NEARBY_LOGS(ERROR)
          << "Nearby BLE Medium scanning failed due to radio not available.";
      break;
    case BluetoothError::ResourceInUse:
      NEARBY_LOGS(ERROR)
          << "Nearby BLE Medium scanning failed due to resource in use.";
      break;
    case BluetoothError::DisabledByPolicy:
      NEARBY_LOGS(ERROR)
          << "Nearby BLE Medium scanning failed due to disabled by policy.";
      break;
    case BluetoothError::DisabledByUser:
      NEARBY_LOGS(ERROR)
          << "Nearby BLE Medium scanning failed due to disabled by user.";
      break;
    case BluetoothError::NotSupported:
      NEARBY_LOGS(ERROR)
          << "Nearby BLE Medium scanning failed due to hardware not supported.";
      break;
    case BluetoothError::TransportNotSupported:
      NEARBY_LOGS(ERROR) << "Nearby BLE Medium scanning failed due to "
                            "transport not supported.";
      break;
    case BluetoothError::ConsentRequired:
      NEARBY_LOGS(ERROR)
          << "Nearby BLE Medium scanning failed due to consent required.";
      break;
    case BluetoothError::OtherError:
      NEARBY_LOGS(ERROR)
          << "Nearby BLE Medium scanning failed due to unknown errors.";
      break;
    case BluetoothError::Success:
      if (watcher_stopped_callback_ != nullptr &&
          watcher_.Status() == BluetoothLEAdvertisementWatcherStatus::Started) {
        watcher_started_callback_();
      }
      if (watcher_stopped_callback_ != nullptr &&
          watcher_.Status() == BluetoothLEAdvertisementWatcherStatus::Stopped) {
        watcher_stopped_callback_();
        watcher_.Stopped(watcher_token_);
        watcher_.Received(advertisement_received_token_);
      }
    default:
      NEARBY_LOGS(ERROR)
          << "Nearby BLE Medium scanning failed due to unknown errors.";
      break;
  }
}

void BleV2Medium::AdvertisementReceivedHandler(
    BluetoothLEAdvertisementWatcher watcher,
    BluetoothLEAdvertisementReceivedEventArgs args) {
  // Handle all BLE advertisements and determine whether the BLE Medium
  // Advertisement Scan Response packet (containing Copresence UUID 0xFEF3) has
  // been received in the handler
  std::array<uint8_t, 8> bluetooth_base_array = {
      static_cast<uint8_t>(0x80), static_cast<uint8_t>(0x00),
      static_cast<uint8_t>(0x00), static_cast<uint8_t>(0x80),
      static_cast<uint8_t>(0x5F), static_cast<uint8_t>(0x9B),
      static_cast<uint8_t>(0x34), static_cast<uint8_t>(0xFB)};
  winrt::guid kCopresenceServiceUuid128bit(
      static_cast<uint32_t>(0x0000FEF3), static_cast<uint16_t>(0x0000),
      static_cast<uint16_t>(0x1000), bluetooth_base_array);

  absl::MutexLock lock(&mutex_);

  if (args.IsScanResponse()) {
    IVector<winrt::guid> guids = args.Advertisement().ServiceUuids();
    bool scan_response_found = false;
    for (const winrt::guid& uuid : guids) {
      if (uuid == kCopresenceServiceUuid128bit) {
        scan_response_found = true;
      }
    }
    if (scan_response_found == true) {
      BleV2Peripheral peripheral;
      api::ble_v2::BleAdvertisementData advertisement_data;
      scan_response_received_callback_.advertisement_found_cb(
          peripheral, advertisement_data);
    }
  }
}

}  // namespace windows
}  // namespace nearby
}  // namespace location
