# 宠物运行精灵

forest.png、nebula.png、sunshine.png分别对应species 0、1、2。96×96透明PNG，31个可见颜色；manifest.json提供名字、类型与形象说明。由tools/build_pet_assets.py从原稿重建，网页和设备使用相同像素与透明遮罩。

网页主画布与pets.html角色预览页使用这些图片；图片由设备HTTP服务同样提供。完整原稿见assets/pets/source，原稿不打包进固件。

*-actions.png为384×288图集，4列3行，每帧96×96；从上到下睡觉、吃饭、玩耍，每行共用15色与透明色。manifest.json的motions提供行号、帧数和时长。*-sleep.gif、*-eat.gif、*-play.gif为九个透明放大预览，不嵌入固件；设备网页使用PNG图集播放。
