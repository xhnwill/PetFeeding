# 开发工具

- build.ps1：设置本地ESP-IDF工具环境、构建并合并完整镜像。
- generate_font.py：由NotoSansSC.ttf生成固件用18像素中文子集；输入字体不提交，生成的font.h提交。
- FONT-LICENSE.txt：生成字体的SIL Open Font License。

字体来源：https://github.com/google/fonts/tree/main/ofl/notosanssc。重新生成时从该目录获取NotoSansSC[wght].ttf并另存为NotoSansSC.ttf；需要Python Pillow。修改界面文字后运行生成器。
