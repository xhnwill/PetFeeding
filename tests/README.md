# 自动化验证

- pet_test.c：实际固件C核心在主机运行的断言回归。
- web.test.mjs：Node内置测试，验证网页规则与无设备操作边界。

仓库根目录运行node --test tests/web.test.mjs。Linux或WSL运行：

```sh
cc -Wall -Wextra -Werror tests/pet_test.c firmware/main/pet.c -o /tmp/mypat-pet-test
/tmp/mypat-pet-test
```

规则模拟不替代实际BLE、屏幕、电源和长时计时验收。
