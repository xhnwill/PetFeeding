# 自动化验证

- pet_test.c：实际固件C核心在主机运行的断言回归。
- web.test.mjs：Node内置测试，验证网页规则与无设备操作边界。
- pet_art_test.py：运行python tests/pet_art_test.py核对透明度、色数、轮廓与网页/设备RGB565像素一致性。
- pet-animation.test.mjs：网页动画优先级、循环、到期及重启动作；随Node测试运行。
- pet_animation_test.c：C动画调度，覆盖持续游戏、睡眠优先和32位计时器回绕；与firmware/main/pet_animation.c一同编译运行。

仓库根目录运行node --test tests/web.test.mjs。Linux或WSL运行：

```sh
cc -Wall -Wextra -Werror tests/pet_test.c firmware/main/pet.c -o /tmp/mypat-pet-test
/tmp/mypat-pet-test
```

规则模拟不替代实际BLE、屏幕、电源和长时计时验收。
