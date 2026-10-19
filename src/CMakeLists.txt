idf_component_register(
    SRCS "main.cpp" "printer.cpp" "label.cpp" "logbuf.cpp" "net.cpp" "web.cpp" "store.cpp" "config.cpp"
         "www_pack.S"  # the web interface, packed by tools/pack_www.py (a PlatformIO pre-build hook)
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio console esp_wifi wpa_supplicant esp_netif esp_event esp_timer nvs_flash esp_http_server esp_app_format
             app_update
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_20)
