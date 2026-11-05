# 配套网页

版本v0.4.0，日期2026-11-05。无需依赖安装或打包，运行python -m http.server 8765 --bind 127.0.0.1 --directory web后打开localhost。也由固件嵌入并在设备IP提供服务。

- index.html：宠物房间、家长设置、说明和小游戏。
- app.js：演示养成模型、BLE/HTTP通信与界面控制。
- style.css：响应式布局。
- pets.html：三只幼体形象及96像素原尺寸预览，不会切换现有宠物。
- assets/pets/：与设备相同像素和调色板的透明PNG与资源清单。
- pet-animation.js：共用动作加载、四帧裁切、时长、优先级和播放。
- package.json：版本与Node内置测试入口。

Web Bluetooth需要支持的浏览器与安全上下文；普通局域网HTTP仅提供设备同源控制。详见[联调说明](../docs/implementation-v0.2.0.md)。
