# 固件模块

- main.c与app.h：唯一宠物状态写入任务、指令队列、快照、界面动作和功耗调度。
- pet.c与pet.h：与ESP-IDF无关的养成规则，可在主机运行回归测试。
- board.c：RCQB屏幕、ADC按键、中文绘图与GPIO2深睡。
- radio.c：NimBLE认证特征、有限发现窗口、配对确认、双机互访与分块传输。
- network.c：Wi-Fi、SNTP、内嵌HTTP网页与HTTPS应用OTA。
- font.h：由tools/generate_font.py生成的Noto Sans SC位图子集，许可见tools/FONT-LICENSE.txt。
- pet_art.h：由tools/build_pet_assets.py生成的三只96×96幼体索引像素与调色板，原稿在assets/pets/source。
- pet_motion.h：生成的36帧4位索引动作与共用调色板。
- pet_animation.c与pet_animation.h：不依赖ESP-IDF的临时动作调度，不进入存档。
