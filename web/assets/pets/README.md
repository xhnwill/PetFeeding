# 宠物运行精灵

forest.png、nebula.png、sunshine.png分别对应species 0、1、2。96×96透明PNG，31个可见颜色；manifest.json提供名字、类型与形象说明。由tools/build_pet_assets.py从原稿重建，网页和设备使用相同像素与透明遮罩。

网页主画布与pets.html角色预览页使用这些图片；图片由设备HTTP服务同样提供。完整原稿见assets/pets/source，原稿不打包进固件。
