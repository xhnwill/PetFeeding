# 配套网页

版本v0.2.0，日期2026-10-29。无需依赖安装或打包，运行python -m http.server 8765 --bind 127.0.0.1 --directory web后打开localhost。也由固件嵌入并在设备IP提供服务。

- index.html：宠物房间、家长设置、说明和小游戏。
- app.js：演示养成模型、BLE/HTTP通信与界面控制。
- style.css：响应式布局。
- package.json：版本与Node内置测试入口。

Web Bluetooth需要支持的浏览器与安全上下文；普通局域网HTTP仅提供设备同源控制。详见[联调说明](../docs/implementation-v0.2.0.md)。
