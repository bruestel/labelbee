/*
   labelbee: firmware for a network label printer (ESP32-S3 + DYMO LabelManager PnP).

   A console on UART0 (the USB-C socket with the USB-serial chip, 115200 baud) switches the
   printer's power and talks to it:

     relay [on|off]          printer power (the relay on GPIO4); without an argument: its state
     usb                     what is connected on the USB host port
     status                  ask the printer for its status (ESC A)
     print [6|9|12]          print a test label on 6, 9 or 12 mm tape (default 12)
     wifi [<ssid> <password>] store the WiFi credentials and join; without arguments: the state
     restart                 restart the ESP32

   Everything else is in the browser: the designer at http://labelbee.local/, the settings and the
   log at /settings (web.cpp, www/).
*/

#include "board.h"
#include "config.h"
#include "label.h"
#include "logbuf.h"
#include "net.h"
#include "printer.h"
#include "store.h"
#include "web.h"

#include <esp_app_desc.h>
#include <esp_chip_info.h>
#include <esp_console.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include <esp_intr_alloc.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <usb/usb_host.h>

#include <cinttypes>
#include <cstdio>
#include <cstring>

static const char *const TAG = "labelbee";

/* A test label's length: ~42 mm at 180 dpi. */
static constexpr uint32_t TEST_LABEL_WIDTH = 300;

static void log_banner() {
  const esp_app_desc_t *app = esp_app_get_description();

  esp_chip_info_t chip;
  esp_chip_info(&chip);

  uint32_t flash_size = 0;
  esp_flash_get_size(nullptr, &flash_size);

  ESP_LOGI(TAG, "labelbee %s (%s %s)", app->version, app->date, app->time);
  ESP_LOGI(TAG, "ESP-IDF %s, %d core(s), rev v%d.%d, %" PRIu32 " MB flash, %u KB PSRAM", IDF_VER, chip.cores,
           chip.revision / 100, chip.revision % 100, flash_size / (1024 * 1024),
           static_cast<unsigned>(heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / 1024));
}

/* Installs the USB host library and runs its event loop: enumeration, connects and disconnects. */
static void usb_host_task(void *main_task) {
  usb_host_config_t config = {
      .skip_phy_setup = false,
      .intr_flags     = ESP_INTR_FLAG_LOWMED,
  };
  ESP_ERROR_CHECK(usb_host_install(&config));
  xTaskNotifyGive(static_cast<TaskHandle_t>(main_task));

  for (;;) {
    uint32_t flags = 0;
    usb_host_lib_handle_events(portMAX_DELAY, &flags);
  }
}

/* --- Console commands ------------------------------------------------------------------------ */


static int cmd_relay(int argc, char **argv) {
  if (argc >= 2 && std::strcmp(argv[1], "on") == 0)
    printer::set_power(true);
  else if (argc >= 2 && std::strcmp(argv[1], "off") == 0)
    printer::set_power(false);
  else if (argc >= 2)
    std::printf("usage: relay [on|off]\n");
  std::printf("relay %s\n", board::printer_power() ? "on" : "off");
  return 0;
}

static int cmd_usb(int argc, char **argv) {
  std::printf("%s\n", printer::describe().c_str());
  return 0;
}

static int cmd_status(int argc, char **argv) {
  std::vector<uint8_t> reply;
  const esp_err_t      err = printer::status(reply);
  if (err != ESP_OK) {
    std::printf("no status: %s\n", esp_err_to_name(err));
    return 1;
  }
  std::printf("status (%u bytes):", static_cast<unsigned>(reply.size()));
  for (uint8_t b : reply)
    std::printf(" %02x", b);
  std::printf("\n");
  return 0;
}

static int cmd_print(int argc, char **argv) {
  const auto tape = label::parse_tape(argc >= 2 ? argv[1] : "12");
  if (!tape) {
    std::printf("usage: print [6|9|12]\n");
    return 1;
  }
  std::string     error;
  const esp_err_t err = printer::print(label::test_pattern(*tape, TEST_LABEL_WIDTH), 1, error);
  std::printf("%s\n", err == ESP_OK ? "printed" : error.c_str());
  return err == ESP_OK ? 0 : 1;
}

static int cmd_wifi(int argc, char **argv) {
  if (argc == 2 || argc > 3) {
    std::printf("usage: wifi [<ssid> <password>]  (quote names with spaces: wifi \"My Net\" secret)\n");
    return 1;
  }
  if (argc == 3) {
    config::Config c = config::get();
    c.wifi.ssid      = argv[1];
    c.wifi.password  = argv[2];
    if (config::save(c) != ESP_OK) {
      std::printf("could not save the settings\n");
      return 1;
    }
    std::printf("saved, restarting\n");
    esp_restart();
  }
  std::printf("%s\n", net::describe().c_str());
  return 0;
}

static int cmd_restart(int argc, char **argv) {
  esp_restart();
  return 0;
}

static void register_command(const char *name, const char *help, const char *hint, esp_console_cmd_func_t func) {
  const esp_console_cmd_t cmd = {
      .command = name,
      .help    = help,
      .hint    = hint,
      .func    = func,
  };
  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

static void start_console() {
  esp_console_repl_t       *repl        = nullptr;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  repl_config.prompt                    = "labelbee>";

  esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

  esp_console_register_help_command();
  register_command("relay", "Printer power: switch the relay on GPIO4, or show its state", "[on|off]", cmd_relay);
  register_command("usb", "Show what is connected on the USB host port", nullptr, cmd_usb);
  register_command("status", "Ask the printer for its status (ESC A)", nullptr, cmd_status);
  register_command("print", "Print a test label on 6, 9 or 12 mm tape", "[6|9|12]", cmd_print);
  register_command("wifi", "Store the WiFi credentials and restart, or show the WiFi state", "[<ssid> <password>]",
                   cmd_wifi);
  register_command("restart", "Restart the ESP32", nullptr, cmd_restart);

  ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

extern "C" void app_main() {
  board::init();
  logbuf::begin();
  log_banner();
  config::load();

  xTaskCreatePinnedToCore(usb_host_task, "usb_host", 4096, xTaskGetCurrentTaskHandle(), 2, nullptr, 0);
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  printer::start();
  printer::set_power(true);  // always on; off only from the settings
  store::start();
  net::start();
  web::start();
  ESP_LOGI(TAG, "ready: the designer at http://%s.local/, the settings at /settings", config::get().name.c_str());
  ESP_LOGI(TAG, "free RAM: %u KB internal (largest block %u KB), %u KB PSRAM",
           static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
           static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024),
           static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));

  start_console();
}
