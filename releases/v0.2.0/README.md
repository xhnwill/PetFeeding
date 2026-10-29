# MyPat v0.2.0

构建日期2026-10-29，ESP32-C3，4MB Flash。详见[联调说明](../../docs/implementation-v0.2.0.md)。

- mypat-v0.2.0-full.bin：首次串口烧录到0x0，含bootloader、分区表、OTA初始元数据和应用。
- mypat-v0.2.0-app.bin：应用OTA镜像，不能作为0x0完整烧录镜像。
- SHA256SUMS.txt：二进制完整性校验。

完整镜像会覆盖旧程序和相应地址范围内的数据。已有宠物需保档升级时使用应用OTA。尚未在实物烧录验证。
