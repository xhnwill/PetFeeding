# MyPat 随身电子宠物机

面向7岁儿童的ESP32-C3随身电子宠物机：随机孵化、吃饭玩耍、清洁睡眠、真实时间成长、BLE近场结伴与家长配网。

当前版本：v0.2.0，构建日期2026-10-29（北京时间）。固件和网页已实现并完成编译及规则回归，尚未烧录实物验收。适配RCQB ESP32-C3已有硬件：240×280 ST7789屏、ADC按键、4MB Flash、无独立RTC。

## 阅读入口

- [可行性设计](docs/design/feasibility-v0.1.0.md)：玩法、成长算法、硬件、电源、固件、无线协议、工作量与验收。
- [项目日志](docs/logbook.md)：设计理由、关键决定及每次变更。
- [文档目录说明](docs/README.md)：按用途组织的文档索引。

## 目录结构

- 根目录：项目入口、版本信息与仓库忽略规则。
- firmware/：ESP-IDF 5.5.4工程与固件源代码。
- web/：无需构建的家长网页，支持演示、BLE和设备同源HTTP。
- tools/：构建与字体生成工具。
- tests/：C和网页规则回归。
- releases/：版本镜像与交付说明。
- docs/design/：当前设计基线，结构说明见该目录README。
- docs/：文档索引与项目日志。

构建：pwsh -NoProfile -File tools/build.ps1。网页：python -m http.server 8765 --bind 127.0.0.1 --directory web，然后访问http://localhost:8765。ESP32工程使用VS Code与ESP-IDF，合并镜像包含bootloader、分区表、OTA元数据与内嵌网页应用。首次完整烧录会覆盖旧数据，后续使用应用OTA保留宠物。

## 仓库维护

本仓库沿用PetFeeding远程地址，旧版实体宠物喂食器计划已按项目所有者指示替换，历史保留于Git。
每次变更同步文档与日志，详细说明提交原因并推送；固件开发后每次提交同步版本与构建日期。
密钥、Wi-Fi凭据、签名私钥及设备运行存档不得提交。
- [实现与联调](docs/implementation-v0.2.0.md)：实际引脚、首次烧录、网页连接、协议、测试与边界。
- [完整发布包](releases/v0.2.0/README.md)：完整烧录镜像、应用OTA镜像与校验值。
