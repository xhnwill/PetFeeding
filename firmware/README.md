# ESP32-C3固件

版本v0.2.0，构建日期2026-10-29。使用ESP-IDF 5.5.4；[构建、引脚和联调说明](../docs/implementation-v0.2.0.md)。

- main/：应用、硬件、宠物核心、BLE及网络代码，详细职责见目录说明。
- CMakeLists.txt：ESP-IDF工程入口。
- sdkconfig.defaults：可复现默认配置，实际sdkconfig为忽略的本地配置。
- partitions.csv：4MB Flash分区与独立宠物存档。
- build/：忽略的生成物，完整发布副本见releases/v0.2.0。
