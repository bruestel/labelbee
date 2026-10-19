#include "printer.h"

#include "board.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <usb/usb_helpers.h>
#include <usb/usb_host.h>

#include <algorithm>
#include <atomic>
#include <mutex>
#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace printer {

static const char *const TAG = "printer";

static constexpr uint16_t VID_DYMO     = 0x0922;
static constexpr uint16_t PID_STORAGE  = 0x1001;
static constexpr uint16_t PID_PRINTER  = 0x1002;
static constexpr size_t   REPORT_SIZE  = 64;
static constexpr uint8_t  ESC          = 0x1B;
static constexpr uint8_t  HID_CLASS    = 0x03;
static constexpr uint8_t  XFER_BULK    = 0x02;
static constexpr uint8_t  XFER_INTR    = 0x03;
static constexpr uint32_t WAIT_MS      = 2000;  // for one transfer; a 64-byte report takes a few ms
static constexpr uint32_t STATUS_MS    = 1000;
static constexpr uint32_t BLOCK_WAIT_MS = 10000;  // while printing the printer answers when it has room
static constexpr uint32_t POLL_MS       = 5000;   // status refresh while the printer is on and idle
static constexpr int64_t  START_TIMEOUT_US = 20 * 1000 * 1000;
static constexpr int64_t  RECOVERY_INTERVAL_US = 60LL * 1000 * 1000;

enum class Mode {
  None,
  Storage,
  Printer,
  Other,
};

/* The connected device. Written by the worker task, read by the console's commands. */
struct Device {
  usb_device_handle_t handle = nullptr;
  Mode                mode   = Mode::None;
  uint16_t            vid    = 0;
  uint16_t            pid    = 0;
  int                 intf   = -1;  // the claimed interface
  uint8_t             ep_out = 0;   // interrupt OUT; 0: output reports go by SET_REPORT
  uint16_t            out_mps = 0;
  uint8_t             ep_in   = 0;  // interrupt IN; 0: none
  uint16_t            in_mps  = 0;
};

struct Event {
  enum Kind : uint8_t {
    NewDevice,
    DeviceGone,
  } kind;
  uint8_t             address;
  usb_device_handle_t handle;
};

static usb_host_client_handle_t s_client;
static QueueHandle_t            s_events;
static SemaphoreHandle_t        s_lock;  // guards s_dev and s_status, held only briefly
static SemaphoreHandle_t        s_io;    // held for a whole transfer: the device is not closed meanwhile
static bool                     s_drain;  // a status query timed out: its late answer may still arrive
static SemaphoreHandle_t        s_done;  // given by a transfer's callback
static Device                   s_dev;
static SemaphoreHandle_t        s_job;  // one user of the printer at a time: a print, a status query, the poll
static Status                   s_status;  // guarded by s_lock
static int64_t                  s_power_on_us;
static int64_t                  s_recovery_us = -RECOVERY_INTERVAL_US;  // the last automatic power cycle

static esp_err_t query(std::vector<uint8_t> &reply, uint32_t wait_ms);
static void      apply_power(bool on, uint32_t expected);
static std::atomic<uint32_t> s_power_requests{0};   // see apply_power
static std::mutex            s_power_mutex;

/* --- USB host client ------------------------------------------------------------------------- */

/* Runs in the client task, inside usb_host_client_handle_events: hand the work to the worker. */
static void client_event(const usb_host_client_event_msg_t *msg, void *arg) {
  Event e = {};
  if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
    e.kind    = Event::NewDevice;
    e.address = msg->new_dev.address;
  } else if (msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
    e.kind   = Event::DeviceGone;
    e.handle = msg->dev_gone.dev_hdl;
  } else {
    return;
  }
  xQueueSend(s_events, &e, 0);
}

/* Transfer callbacks run here too, so the worker and the console may block on a transfer. */
static void client_task(void *) {
  for (;;)
    usb_host_client_handle_events(s_client, portMAX_DELAY);
}

/* Who frees a transfer. A control transfer that timed out cannot be cancelled (the host library
   has no timeout for endpoint 0), so it is left to finish on its own: run() and the callback agree
   with one atomic exchange which of them owns it, and an abandoned transfer is freed by its
   callback. run() then returns ESP_ERR_NOT_FINISHED and the caller must not touch it again. */
struct Pending {
  enum : int { Running, Done, Abandoned };
  std::atomic<int> state{Running};
};

static void transfer_done(usb_transfer_t *t) {
  auto *p = static_cast<Pending *>(t->context);
  if (p->state.exchange(Pending::Done) == Pending::Abandoned) {
    delete p;
    usb_host_transfer_free(t);
    return;
  }
  xSemaphoreGive(s_done);
}

/* Frees a transfer after run(), unless run() handed it to its callback. */
static void release(usb_transfer_t *t, esp_err_t err) {
  if (t && err != ESP_ERR_NOT_FINISHED)
    usb_host_transfer_free(t);
}

static const char *transfer_status_name(usb_transfer_status_t s) {
  switch (s) {
    case USB_TRANSFER_STATUS_COMPLETED:
      return "completed";
    case USB_TRANSFER_STATUS_ERROR:
      return "error";
    case USB_TRANSFER_STATUS_TIMED_OUT:
      return "timed out";
    case USB_TRANSFER_STATUS_CANCELED:
      return "canceled";
    case USB_TRANSFER_STATUS_STALL:
      return "stall";
    case USB_TRANSFER_STATUS_OVERFLOW:
      return "overflow";
    case USB_TRANSFER_STATUS_SKIPPED:
      return "skipped";
    case USB_TRANSFER_STATUS_NO_DEVICE:
      return "no device";
  }
  return "?";
}

/* Submits a transfer and waits for it. Non-control transfers have no timeout of their own:
   one that does not finish in time is cancelled by halting and flushing its endpoint. */
static esp_err_t run(usb_transfer_t *t, uint32_t wait_ms) {
  const bool control = t->bEndpointAddress == 0;
  auto      *pending = new Pending;
  t->callback        = transfer_done;
  t->context         = pending;
  xSemaphoreTake(s_done, 0);  // a stale give from an earlier, cancelled transfer

  esp_err_t err = control ? usb_host_transfer_submit_control(s_client, t) : usb_host_transfer_submit(t);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "submit to ep 0x%02x: %s", t->bEndpointAddress, esp_err_to_name(err));
    delete pending;
    t->context = nullptr;
    return err;
  }
  if (xSemaphoreTake(s_done, pdMS_TO_TICKS(wait_ms)) != pdTRUE) {
    if (!control) {  // cancelled by halting and flushing the endpoint: the callback follows
      usb_host_endpoint_halt(t->device_handle, t->bEndpointAddress);
      usb_host_endpoint_flush(t->device_handle, t->bEndpointAddress);
      xSemaphoreTake(s_done, pdMS_TO_TICKS(WAIT_MS));
    }
    if (pending->state.exchange(Pending::Abandoned) == Pending::Running) {
      ESP_LOGW(TAG, "transfer on ep 0x%02x timed out, left to finish on its own", t->bEndpointAddress);
      return ESP_ERR_NOT_FINISHED;   // the callback frees it
    }
    xSemaphoreTake(s_done, 0);   // it did finish meanwhile: its give
    delete pending;
    t->context = nullptr;
    if (!control)
      usb_host_endpoint_clear(t->device_handle, t->bEndpointAddress);
    return ESP_ERR_TIMEOUT;
  }
  delete pending;
  t->context = nullptr;
  if (t->status != USB_TRANSFER_STATUS_COMPLETED) {
    ESP_LOGW(TAG, "transfer on ep 0x%02x: %s", t->bEndpointAddress, transfer_status_name(t->status));
    return ESP_FAIL;
  }
  return ESP_OK;
}

/* --- Descriptors ----------------------------------------------------------------------------- */

struct Endpoints {
  int      intf    = -1;
  uint8_t  intf_class = 0;
  uint8_t  bulk_out   = 0;
  uint8_t  intr_out   = 0;
  uint16_t intr_out_mps = 0;
  uint8_t  intr_in      = 0;
  uint16_t intr_in_mps  = 0;
  uint16_t hid_report_len = 0;  // from the HID class descriptor
};

/* The first interface (alternate setting 0) that `want` accepts, with its endpoints. */
template <typename Want> static Endpoints find_interface(const usb_config_desc_t *cfg, Want want) {
  for (int i = 0; i < cfg->bNumInterfaces; i++) {
    int                    offset = 0;
    const usb_intf_desc_t *intf   = usb_parse_interface_descriptor(cfg, i, 0, &offset);
    if (!intf)
      continue;

    Endpoints e;
    e.intf       = intf->bInterfaceNumber;
    e.intf_class = intf->bInterfaceClass;

    // The HID class descriptor (type 0x21) follows the interface descriptor; bytes 7-8 are the
    // length of the report descriptor.
    const uint8_t *p = reinterpret_cast<const uint8_t *>(intf) + intf->bLength;
    if (intf->bInterfaceClass == HID_CLASS && p[1] == 0x21 && p[0] >= 9)
      e.hid_report_len = static_cast<uint16_t>(p[7] | (p[8] << 8));

    for (int n = 0; n < intf->bNumEndpoints; n++) {
      int                  ep_offset = offset;
      const usb_ep_desc_t *ep = usb_parse_endpoint_descriptor_by_index(intf, n, cfg->wTotalLength, &ep_offset);
      if (!ep)
        continue;
      const uint8_t  type = ep->bmAttributes & 0x03;
      const bool     in   = ep->bEndpointAddress & 0x80;
      const uint16_t mps  = ep->wMaxPacketSize & 0x07FF;
      if (type == XFER_BULK && !in && !e.bulk_out)
        e.bulk_out = ep->bEndpointAddress;
      if (type == XFER_INTR && !in && !e.intr_out) {
        e.intr_out     = ep->bEndpointAddress;
        e.intr_out_mps = mps;
      }
      if (type == XFER_INTR && in && !e.intr_in) {
        e.intr_in     = ep->bEndpointAddress;
        e.intr_in_mps = mps;
      }
    }
    if (want(e))
      return e;
  }
  return {};
}

/* A string descriptor (UTF-16LE) as ASCII; other characters become '?'. */
static std::string ascii(const usb_str_desc_t *s) {
  std::string out;
  if (!s)
    return out;
  for (int i = 0; i < (s->bLength - 2) / 2; i++)
    out += s->wData[i] < 0x80 ? static_cast<char>(s->wData[i]) : '?';
  return out;
}

static const char *transfer_type_name(uint8_t type) {
  static const char *const names[] = {"control", "isochronous", "bulk", "interrupt"};
  return names[type & 0x03];
}

/* Everything the device reports about itself. Through ESP_LOG, not ESP-IDF's usb_print_*
   helpers (plain printf), so it also reaches the log on the test page. */
static void log_device(usb_device_handle_t dev, const usb_device_desc_t *desc, const usb_config_desc_t *cfg) {
  usb_device_info_t info;
  if (usb_host_device_info(dev, &info) == ESP_OK) {
    static const char *const speeds[] = {"low", "full", "high"};
    ESP_LOGI(TAG, "  speed %s, address %u", info.speed <= 2 ? speeds[info.speed] : "?", info.dev_addr);
    ESP_LOGI(TAG, "  manufacturer \"%s\", product \"%s\", serial \"%s\"", ascii(info.str_desc_manufacturer).c_str(),
             ascii(info.str_desc_product).c_str(), ascii(info.str_desc_serial_num).c_str());
  }
  ESP_LOGI(TAG, "  device: USB %x.%02x, class 0x%02x/0x%02x/0x%02x, ep0 %u bytes, release %x.%02x, %u configuration(s)",
           desc->bcdUSB >> 8, desc->bcdUSB & 0xFF, desc->bDeviceClass, desc->bDeviceSubClass, desc->bDeviceProtocol,
           desc->bMaxPacketSize0, desc->bcdDevice >> 8, desc->bcdDevice & 0xFF, desc->bNumConfigurations);
  ESP_LOGI(TAG, "  configuration %u: %u interface(s), %u bytes, attributes 0x%02x, max power %u mA",
           cfg->bConfigurationValue, cfg->bNumInterfaces, cfg->wTotalLength, cfg->bmAttributes, cfg->bMaxPower * 2);

  // walk all descriptors of the configuration: interfaces, class descriptors, endpoints
  const uint8_t *p   = reinterpret_cast<const uint8_t *>(cfg);
  const uint8_t *end = p + cfg->wTotalLength;
  for (p += cfg->bLength; p + 2 <= end && p[0] >= 2 && p + p[0] <= end; p += p[0]) {
    if (p[1] == USB_B_DESCRIPTOR_TYPE_INTERFACE) {
      const auto *i = reinterpret_cast<const usb_intf_desc_t *>(p);
      ESP_LOGI(TAG, "  interface %u (alt %u): class 0x%02x/0x%02x/0x%02x, %u endpoint(s)", i->bInterfaceNumber,
               i->bAlternateSetting, i->bInterfaceClass, i->bInterfaceSubClass, i->bInterfaceProtocol,
               i->bNumEndpoints);
    } else if (p[1] == USB_B_DESCRIPTOR_TYPE_ENDPOINT) {
      const auto *e = reinterpret_cast<const usb_ep_desc_t *>(p);
      ESP_LOGI(TAG, "    endpoint 0x%02x %s %s, %u bytes, interval %u", e->bEndpointAddress,
               (e->bEndpointAddress & 0x80) ? "IN" : "OUT", transfer_type_name(e->bmAttributes),
               e->wMaxPacketSize & 0x07FF, e->bInterval);
    } else if (p[1] == 0x21 && p[0] >= 9) {
      ESP_LOGI(TAG, "    HID %x.%02x, report descriptor %u bytes", p[3], p[2], p[7] | (p[8] << 8));
    } else {
      ESP_LOGI(TAG, "    descriptor type 0x%02x, %u bytes", p[1], p[0]);
    }
  }
}

/* GET_DESCRIPTOR(Report) on the HID interface: says how large the output reports really are. */
static void log_hid_report_descriptor(usb_device_handle_t dev, int intf, uint16_t len) {
  const uint16_t  want = len ? len : 256;
  usb_transfer_t *t    = nullptr;
  if (usb_host_transfer_alloc(sizeof(usb_setup_packet_t) + want, 0, &t) != ESP_OK)
    return;
  auto *setup          = reinterpret_cast<usb_setup_packet_t *>(t->data_buffer);
  setup->bmRequestType = 0x81;  // device to host, standard, interface
  setup->bRequest      = 0x06;  // GET_DESCRIPTOR
  setup->wValue        = 0x2200;  // report descriptor, index 0
  setup->wIndex        = static_cast<uint16_t>(intf);
  setup->wLength       = want;
  t->num_bytes         = sizeof(usb_setup_packet_t) + want;
  t->device_handle     = dev;
  t->bEndpointAddress  = 0;
  t->timeout_ms        = WAIT_MS;

  const esp_err_t err = run(t, WAIT_MS);
  if (err == ESP_OK) {
    const int n = t->actual_num_bytes - static_cast<int>(sizeof(usb_setup_packet_t));
    ESP_LOGI(TAG, "HID report descriptor (%d bytes):", n);
    ESP_LOG_BUFFER_HEX(TAG, t->data_buffer + sizeof(usb_setup_packet_t), n);
  } else {
    ESP_LOGW(TAG, "could not read the HID report descriptor");
  }
  release(t, err);
}

/* --- Device handling (worker task) ----------------------------------------------------------- */

/* Storage mode: ESC Z 1 makes the printer come back as a pure HID printer. In storage mode the
   PnP has two interfaces, HID (interrupt OUT 0x01) and mass storage (bulk OUT 0x02); the command
   goes to the HID one, as usb_modeswitch's MessageEndpoint=0x01 does. Sent to the bulk endpoint
   it is ignored (tried 2026-10-18). */
static void switch_mode(usb_device_handle_t dev, const usb_config_desc_t *cfg) {
  Endpoints e  = find_interface(cfg, [](const Endpoints &e) { return e.intf_class == HID_CLASS && e.intr_out; });
  uint8_t   ep = e.intr_out;
  if (!ep) {
    e  = find_interface(cfg, [](const Endpoints &e) { return e.bulk_out != 0; });
    ep = e.bulk_out;
  }
  if (!ep) {
    ESP_LOGE(TAG, "storage mode, but neither an interrupt nor a bulk OUT endpoint: cannot switch");
    return;
  }
  if (usb_host_interface_claim(s_client, dev, e.intf, 0) != ESP_OK) {
    ESP_LOGE(TAG, "cannot claim interface %d", e.intf);
    return;
  }
  usb_transfer_t *t = nullptr;
  ESP_ERROR_CHECK(usb_host_transfer_alloc(64, 0, &t));
  const uint8_t cmd[] = {ESC, 'Z', 1};
  std::memcpy(t->data_buffer, cmd, sizeof(cmd));
  t->num_bytes        = sizeof(cmd);
  t->device_handle    = dev;
  t->bEndpointAddress = ep;
  const esp_err_t err = run(t, WAIT_MS);
  release(t, err);
  usb_host_interface_release(s_client, dev, e.intf);

  if (err == ESP_OK)
    ESP_LOGI(TAG, "sent ESC Z 1 to ep 0x%02x (interface %d): the printer should come back as %04x:%04x", ep, e.intf,
             VID_DYMO, PID_PRINTER);
  else
    ESP_LOGE(TAG, "mode switch failed: %s", esp_err_to_name(err));
}

/* Printer mode: claim the HID interface and remember its endpoints. */
static void open_printer(usb_device_handle_t dev, const usb_config_desc_t *cfg) {
  const Endpoints e = find_interface(cfg, [](const Endpoints &e) { return e.intf_class == HID_CLASS; });
  if (e.intf < 0) {
    ESP_LOGE(TAG, "printer mode, but no HID interface");
    return;
  }
  ESP_LOGI(TAG, "HID interface %d: interrupt OUT 0x%02x (%u bytes), interrupt IN 0x%02x (%u bytes)", e.intf,
           e.intr_out, e.intr_out_mps, e.intr_in, e.intr_in_mps);
  if (!e.intr_out)
    ESP_LOGW(TAG, "no interrupt OUT endpoint: output reports go by SET_REPORT on the control endpoint");

  if (usb_host_interface_claim(s_client, dev, e.intf, 0) != ESP_OK) {
    ESP_LOGE(TAG, "cannot claim interface %d", e.intf);
    return;
  }
  log_hid_report_descriptor(dev, e.intf, e.hid_report_len);

  xSemaphoreTake(s_lock, portMAX_DELAY);
  s_dev.mode    = Mode::Printer;
  s_dev.intf    = e.intf;
  s_dev.ep_out  = e.intr_out;
  s_dev.out_mps = e.intr_out_mps;
  s_dev.ep_in   = e.intr_in;
  s_dev.in_mps  = e.intr_in_mps;
  xSemaphoreGive(s_lock);
  ESP_LOGI(TAG, "printer ready");
}

static void on_new_device(uint8_t address) {
  usb_device_handle_t dev = nullptr;
  if (usb_host_device_open(s_client, address, &dev) != ESP_OK) {
    ESP_LOGE(TAG, "cannot open the device at address %u", address);
    return;
  }
  const usb_device_desc_t *desc = nullptr;
  const usb_config_desc_t *cfg  = nullptr;
  ESP_ERROR_CHECK(usb_host_get_device_descriptor(dev, &desc));
  ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(dev, &cfg));

  ESP_LOGI(TAG, "device connected: %04x:%04x", desc->idVendor, desc->idProduct);
  log_device(dev, desc, cfg);

  xSemaphoreTake(s_lock, portMAX_DELAY);
  s_dev        = {};
  s_dev.handle = dev;
  s_dev.vid    = desc->idVendor;
  s_dev.pid    = desc->idProduct;
  s_dev.mode   = Mode::Other;
  if (desc->idVendor == VID_DYMO && desc->idProduct == PID_STORAGE)
    s_dev.mode = Mode::Storage;
  const Mode mode = s_dev.mode;
  xSemaphoreGive(s_lock);

  if (mode == Mode::Storage)
    switch_mode(dev, cfg);
  else if (desc->idVendor == VID_DYMO && desc->idProduct == PID_PRINTER)
    open_printer(dev, cfg);
  else
    ESP_LOGW(TAG, "not a LabelManager PnP: left alone");
}

static void on_device_gone(usb_device_handle_t dev) {
  xSemaphoreTake(s_io, portMAX_DELAY);   // a running transfer ends first (it fails: the device is gone)
  xSemaphoreTake(s_lock, portMAX_DELAY);
  if (s_dev.handle == dev) {
    if (s_dev.mode == Mode::Printer && s_dev.intf >= 0)
      usb_host_interface_release(s_client, dev, s_dev.intf);
    s_dev    = {};
    s_status = {};
  }
  xSemaphoreGive(s_lock);
  usb_host_device_close(s_client, dev);
  xSemaphoreGive(s_io);
  ESP_LOGI(TAG, "device disconnected");
}

/* Between events, while the printer is on: keep the cached status fresh, and power-cycle a
   printer that does not come up (once a minute at most). Skipped while a job runs. */
static void housekeeping() {
  if (!board::printer_power() || xSemaphoreTake(s_job, 0) != pdTRUE)
    return;
  const int64_t now = esp_timer_get_time();
  if (state() == State::Missing && now - s_recovery_us > RECOVERY_INTERVAL_US) {
    ESP_LOGW(TAG, "the printer does not come up: switching it off and on again");
    s_recovery_us          = now;
    const uint32_t request = s_power_requests;
    apply_power(false, request);
    vTaskDelay(pdMS_TO_TICKS(3000));
    apply_power(true, request);   // only if nobody switched it meanwhile
  } else if (ready()) {
    std::vector<uint8_t> reply;
    query(reply, STATUS_MS);
  }
  xSemaphoreGive(s_job);
}

static void worker_task(void *) {
  for (;;) {
    Event e;
    if (xQueueReceive(s_events, &e, pdMS_TO_TICKS(POLL_MS)) != pdTRUE) {
      housekeeping();
      continue;
    }
    if (e.kind == Event::NewDevice)
      on_new_device(e.address);
    else
      on_device_gone(e.handle);
  }
}

/* --- Public ---------------------------------------------------------------------------------- */

void start() {
  s_events = xQueueCreate(8, sizeof(Event));
  s_lock   = xSemaphoreCreateMutex();
  s_io     = xSemaphoreCreateMutex();
  s_job    = xSemaphoreCreateMutex();
  s_done   = xSemaphoreCreateBinary();

  usb_host_client_config_t config = {
      .is_synchronous    = false,
      .max_num_event_msg = 5,
      .async =
          {
              .client_event_callback = client_event,
              .callback_arg          = nullptr,
          },
  };
  ESP_ERROR_CHECK(usb_host_client_register(&config, &s_client));

  xTaskCreatePinnedToCore(client_task, "usb_client", 4096, nullptr, 3, nullptr, 0);
  xTaskCreatePinnedToCore(worker_task, "printer", 6144, nullptr, 2, nullptr, 0);
}

std::string describe() {
  xSemaphoreTake(s_lock, portMAX_DELAY);
  const Device d = s_dev;
  xSemaphoreGive(s_lock);

  char line[160];
  switch (d.mode) {
    case Mode::None:
      return "no USB device";
    case Mode::Storage:
      return "LabelManager PnP in storage mode (0922:1001), switching";
    case Mode::Printer:
      std::snprintf(line, sizeof(line),
                    "LabelManager PnP ready (0922:1002): interface %d, OUT 0x%02x/%u bytes%s, IN 0x%02x/%u bytes",
                    d.intf, d.ep_out, d.out_mps, d.ep_out ? "" : " (SET_REPORT)", d.ep_in, d.in_mps);
      return line;
    case Mode::Other:
      std::snprintf(line, sizeof(line), "other USB device %04x:%04x", d.vid, d.pid);
      return line;
  }
  return "?";
}

bool ready() {
  xSemaphoreTake(s_lock, portMAX_DELAY);
  const bool r = s_dev.mode == Mode::Printer;
  xSemaphoreGive(s_lock);
  return r;
}

/* One output report: on the interrupt OUT endpoint, or as SET_REPORT(Output, id 0). */
static esp_err_t send_report(const Device &d, usb_transfer_t *t, const uint8_t *data, size_t len) {
  t->device_handle = d.handle;
  if (d.ep_out) {
    std::memcpy(t->data_buffer, data, len);
    t->num_bytes        = static_cast<int>(len);
    t->bEndpointAddress = d.ep_out;
  } else {
    auto *setup          = reinterpret_cast<usb_setup_packet_t *>(t->data_buffer);
    setup->bmRequestType = 0x21;    // host to device, class, interface
    setup->bRequest      = 0x09;    // SET_REPORT
    setup->wValue        = 0x0200;  // output report, id 0
    setup->wIndex        = static_cast<uint16_t>(d.intf);
    setup->wLength       = static_cast<uint16_t>(len);
    std::memcpy(t->data_buffer + sizeof(usb_setup_packet_t), data, len);
    t->num_bytes        = static_cast<int>(sizeof(usb_setup_packet_t) + len);
    t->bEndpointAddress = 0;
    t->timeout_ms       = WAIT_MS;
  }
  return run(t, WAIT_MS);
}

static Device current_device() {
  xSemaphoreTake(s_lock, portMAX_DELAY);
  const Device d = s_dev;
  xSemaphoreGive(s_lock);
  return d;
}

/* Sends bytes as output reports of up to 64 bytes. */
static esp_err_t send_locked(const Device &d, const uint8_t *data, size_t size) {
  if (d.mode != Mode::Printer)
    return ESP_ERR_INVALID_STATE;
  usb_transfer_t *t   = nullptr;
  esp_err_t       err = usb_host_transfer_alloc(sizeof(usb_setup_packet_t) + REPORT_SIZE, 0, &t);
  for (size_t pos = 0; err == ESP_OK && pos < size; pos += REPORT_SIZE) {
    const size_t len = std::min(REPORT_SIZE, size - pos);
    err              = send_report(d, t, data + pos, len);
    if (err != ESP_OK)
      ESP_LOGE(TAG, "report at byte %u of %u: %s", static_cast<unsigned>(pos), static_cast<unsigned>(size),
               esp_err_to_name(err));
  }
  if (t)
    release(t, err);
  return err;
}

/* The same, taking the transfer lock (s_io held: the device stays open). */
static esp_err_t send_bytes(const uint8_t *data, size_t size) {
  xSemaphoreTake(s_io, portMAX_DELAY);
  const esp_err_t err = send_locked(current_device(), data, size);
  xSemaphoreGive(s_io);
  return err;
}

/* One read from the IN endpoint into reply. */
static esp_err_t read_in(const Device &d, std::vector<uint8_t> &reply, uint32_t wait_ms) {
  usb_transfer_t *t   = nullptr;
  esp_err_t       err = usb_host_transfer_alloc(d.in_mps, 0, &t);
  if (err != ESP_OK)
    return err;
  t->num_bytes        = d.in_mps;  // IN transfers: a multiple of the packet size
  t->device_handle    = d.handle;
  t->bEndpointAddress = d.ep_in;
  err                 = run(t, wait_ms);
  if (err == ESP_OK)
    reply.assign(t->data_buffer, t->data_buffer + t->actual_num_bytes);
  release(t, err);
  return err;
}

/* ESC A and the printer's answer (8 bytes), within wait_ms. Updates the cached status. */
static esp_err_t query(std::vector<uint8_t> &reply, uint32_t wait_ms) {
  reply.clear();
  // s_io for the whole exchange: the device cannot be closed between ESC A and the answer
  xSemaphoreTake(s_io, portMAX_DELAY);
  const Device d = current_device();
  if (d.mode != Mode::Printer || !d.ep_in || !d.in_mps) {
    xSemaphoreGive(s_io);
    return ESP_ERR_NOT_SUPPORTED;
  }
  if (s_drain) {
    // The answer to an earlier, timed-out query may still come: take it away first, so that it
    // is not mistaken for the answer to this one.
    std::vector<uint8_t> late;
    read_in(d, late, 50);
    s_drain = false;
  }
  const uint8_t cmd[] = {ESC, 'A'};
  esp_err_t     err   = send_locked(d, cmd, sizeof(cmd));
  if (err == ESP_OK) {
    err = read_in(d, reply, wait_ms);
    if (err == ESP_ERR_TIMEOUT || err == ESP_ERR_NOT_FINISHED)
      s_drain = true;
  }
  xSemaphoreGive(s_io);

  xSemaphoreTake(s_lock, portMAX_DELAY);
  if (err == ESP_OK && reply.size() >= 6) {
    // The status bytes, as measured: docs/protocol.md.
    s_status.valid      = true;
    s_status.cassette   = reply[0] & 0x40;
    s_status.busy       = reply[0] & 0x20;
    s_status.charging   = reply[2] & 0x10;
    s_status.battery_percent = reply[3];
    s_status.battery_mv      = static_cast<uint16_t>(reply[4] << 8 | reply[5]);
  }
  xSemaphoreGive(s_lock);
  return err;
}

esp_err_t status(std::vector<uint8_t> &reply) {
  xSemaphoreTake(s_job, portMAX_DELAY);
  const esp_err_t err = query(reply, STATUS_MS);
  xSemaphoreGive(s_job);
  return err;
}

esp_err_t send(const std::vector<uint8_t> &data) {
  xSemaphoreTake(s_job, portMAX_DELAY);
  const esp_err_t err = send_bytes(data.data(), data.size());
  xSemaphoreGive(s_job);
  return err;
}

/* --- Power and jobs -------------------------------------------------------------------------- */

/* Switches the relay. The user's switch (set_power) counts up s_power_requests, so the automatic
   power cycle can tell whether someone switched the printer off while it waited. */

static constexpr uint32_t ANY_REQUEST = UINT32_MAX;

/* expected: switch only if no user request came since that count (ANY_REQUEST: always). The
   check and the switch happen under one lock, so a user's "off" cannot slip in between. */
static void apply_power(bool on, uint32_t expected) {
  {
    std::lock_guard<std::mutex> lock(s_power_mutex);
    if (expected != ANY_REQUEST && s_power_requests != expected)
      return;
    if (on == board::printer_power())
      return;
    board::set_printer_power(on);
  }
  if (on) {
    s_power_on_us = esp_timer_get_time();
  } else {
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_status = {};
    xSemaphoreGive(s_lock);
  }
  ESP_LOGI(TAG, "printer power %s", on ? "on" : "off");
}

void set_power(bool on) {
  {
    std::lock_guard<std::mutex> lock(s_power_mutex);
    s_power_requests++;
  }
  apply_power(on, ANY_REQUEST);
}

State state() {
  if (!board::printer_power())
    return State::Off;
  if (ready())
    return State::Ready;
  return esp_timer_get_time() - s_power_on_us < START_TIMEOUT_US ? State::Starting : State::Missing;
}

Status last_status() {
  xSemaphoreTake(s_lock, portMAX_DELAY);
  const Status st = s_status;
  xSemaphoreGive(s_lock);
  return st;
}

/* Waits until the printer answers: it enumerates ~0.4 s after power
   on, switches mode, comes back after ~5.5 s and does not answer ESC A for a few seconds more. */
static esp_err_t bring_up(std::string &error) {
  if (!board::printer_power()) {
    error = "The printer is switched off. Switch it on in the settings.";
    return ESP_ERR_INVALID_STATE;
  }
  while (!ready()) {
    if (esp_timer_get_time() - s_power_on_us > START_TIMEOUT_US) {
      error = "The printer does not respond. Is it connected?";
      return ESP_ERR_TIMEOUT;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  std::vector<uint8_t> reply;
  for (int attempt = 0; attempt < 10; attempt++)
    if (query(reply, STATUS_MS) == ESP_OK)
      return ESP_OK;
  error = "The printer does not answer.";
  return ESP_ERR_TIMEOUT;
}

esp_err_t print(const label::Stream &stream, int copies, std::string &error) {
  if (stream.data.empty() || stream.blocks.empty()) {
    error = "The label is empty.";
    return ESP_ERR_INVALID_ARG;
  }
  xSemaphoreTake(s_job, portMAX_DELAY);
  esp_err_t err = bring_up(error);
  if (err == ESP_OK && !last_status().cassette) {
    error = "No tape inserted.";
    err   = ESP_ERR_INVALID_STATE;
  }

  const int64_t             started = esp_timer_get_time();
  const usb_device_handle_t printer = current_device().handle;
  std::vector<uint8_t>      reply;
  for (int copy = 0; err == ESP_OK && copy < copies; copy++) {
    for (size_t b = 0; err == ESP_OK && b < stream.blocks.size(); b++) {
      // switched off or unplugged meanwhile: a printer that came back never got this label's start
      if (current_device().handle != printer) {
        error = "The printer was disconnected while printing.";
        err   = ESP_ERR_INVALID_STATE;
        break;
      }
      // flow control: the printer answers when it has room for the next block
      if ((err = query(reply, BLOCK_WAIT_MS)) != ESP_OK) {
        error = "The printer stopped answering.";
        break;
      }
      const size_t from = stream.blocks[b];
      const size_t to   = b + 1 < stream.blocks.size() ? stream.blocks[b + 1] : stream.data.size();
      if ((err = send_bytes(stream.data.data() + from, to - from)) != ESP_OK)
        error = "Sending to the printer failed.";
    }
  }
  if (err == ESP_OK)
    ESP_LOGI(TAG, "printed %d label(s), %u bytes each, in %lld ms", copies,
             static_cast<unsigned>(stream.data.size()), (esp_timer_get_time() - started) / 1000);
  else
    ESP_LOGE(TAG, "print failed: %s (%s)", error.c_str(), esp_err_to_name(err));
  xSemaphoreGive(s_job);
  return err;
}

}  // namespace printer
