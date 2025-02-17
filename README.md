| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- |
# Communication Module


Menuconfig set partition size to custom partition and increase flash size to maximum


### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT build flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

 esptool.py --chip esp32c3 write_flash -z 0x210000 spiffs.bin
 python $IDF_PATH/components/spiffs/spiffsgen.py 2031616 build/ spiffs.bin

 menuconfig -> set flash size to 4mb
 menuconfig -> set custom partition file
 menufoconfi -> set bluetooth to NimBLE