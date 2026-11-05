# 开发工具

- build.ps1：设置本地ESP-IDF工具环境、构建并合并完整镜像。
- generate_font.py：由NotoSansSC.ttf生成固件用18像素中文子集；输入字体不提交，生成的font.h提交。
- build_pet_assets.py：由三张立绘与三张动作原稿编译96×96精灵、四帧图集、GIF、清单和设备索引色头文件。
- preview.py：本机动作网页预览，默认端口8766，避免反向DNS查询阻塞。
- FONT-LICENSE.txt：生成字体的SIL Open Font License。

字体来源：https://github.com/google/fonts/tree/main/ofl/notosanssc。重新生成时从该目录获取NotoSansSC[wght].ttf并另存为NotoSansSC.ttf；需要Python Pillow。修改界面文字后运行生成器。
