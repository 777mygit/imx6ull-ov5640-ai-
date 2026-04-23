设置日期和豆包火山大模型的api（可在火山大模型申请，这里采用Doubao-Seedream-5.0-lite视觉模型）
命令行输入下面命令修改时间和设置api
date -s "2026-03-14 12:00:00"
export DOUBAO_API_KEY='d7********5'

传往开发板传文件
ifconfig eth1 192.168.15.120 netmask 255.255.255.0 up
这个地址是qt编译后生成可执行文件的地址
Z:\qt_demo\EmbeddedQtTutorial\Qt\03\build-05_opencv_camera-ATK_I_MX6U-Debug
具体可以在中找到->>>>>>>方法 1：Qt Creator 一键打开（最推荐）
点击左侧边栏的 Projects（项目） 图标（就是你截图里绿色 QC 下面那个文件夹图标）
在弹出的界面里，找到 Build Settings（构建设置）
直接点击 Build directory（构建目录） 右边的 文件夹图标 📂
→ 会自动打开生成文件所在的文件夹
scp传输
scp -o HostKeyAlgorithms=+ssh-rsa -o PubkeyAcceptedKeyTypes=+ssh-rsa 05_opencv_camera root@192.168.15.120:/home/root/05

问题
1.传输文件到开发板要网线 但是上传云端默认是网线 所以如果想使用wifi上传ai 改一下使用wifi而不是网线
2.美颜过程很慢
3.相册预览界面和在相册中点击具体照片细致浏览界面没有返回按钮

4.21更新
修复1
修复3
问题2仍然存在，目前认为是-》》》》》
1.网络传输？可能性不大 因为base64编码后数据量不是很大而且
正在美颜中，请稍候...

绑定wlan0地址 "10.64.251.131"

API响应已接收完整，立即处理响应
111111这一步响应很快，说明ai模型端收到请求卡在生成
正在美颜中，请稍候...

绑定wlan0地址 "10.64.251.131"

API响应已接收完整，立即处理响应

绑定wlan0地址 "10.64.251.131"

下载响应已接收完整，立即处理响应
222222这是正常流程 说明模型端一直没有返回照片
2.基于1，认为可能是关键词选取导致生成的照片过慢（有些比如赛博风、机械风有时模型生成的较为18+带一些恐怖血腥）导致模型端返回过慢
或者是模型波动？或者网络波动导致返回下载相应过慢？
为了验证是网络还是模型 下次更改模型对比11111111



问题

偶然复现usbwifi连接不上 但是重新插拔之后就好了-》》》》》》》没找到原因 wifi那边没写好

4.22
针对问题2（相册返回过慢）
加了两个处理：

下载完成判断加了图片格式兜底
现在如果 HTTP body 里已经出现完整 PNG 结束标记，就直接判定下载完成。
如果以后返回成 JPG，也顺手支持了 JPEG 结束标记判断。

下载阶段增加累计字节日志
现在会打印：

下载响应累计字节 ...
下载响应已接收完整，立即处理响应


针对usbwifi-》》》》》》
关键报错是这类：

nl80211: Driver does not support authentication/association or connect commands
ioctl[SIOCSIWAP]: Operation not permitted
不是“密码错了”，而是 USB Wi‑Fi 驱动和 nl80211 不匹配。很多老一点的 imx6ull + USB Wi‑Fi 组合，更适合直接走 wext。

做了这几个调整：

wpa_supplicant 启动顺序改成了
先 wext，再回退 nl80211,wext
不再先试 nl80211。

打开 Wi‑Fi 时先清理旧的 wpa_supplicant，避免之前残留进程影响。

Wi‑Fi 页面状态提示做了收敛
不再把驱动探测阶段那种底层错误原样显示成“连接失败”，避免看起来像一打开就报错。

连接失败提示也做了过滤
像你这种 does not support... / Operation not permitted 这类驱动适配信息，不再直接显示给你当最终错误，而是转成更正常的“请检查密码或热点是否可用”这类提示。
测试一下相册美颜



git推送 

PS C:\Users\ git --version
git version 2.51.0.windows.1
PS C:\Users\> git config --global user.name "777mygit"
PS C:\Users\> git config --global user.email "25s001027@stu.hit.edu.cn"
cd进入目标文件夹
PS Z:\qt_demo\EmbeddedQtTutorial\05_opencv_camera> git add .
PS Z:\qt_demo\EmbeddedQtTutorial\05_opencv_camera> git commit -m "Initial commit: 上传Qt OpenCV摄像头项目"
PS Z:\qt_demo\EmbeddedQtTutorial\05_opencv_camera> git remote add origin https://github.com/777mygit/imx6ull-ov5640-ai-.git
PS Z:\qt_demo\EmbeddedQtTutorial\05_opencv_camera> git push -u origin main
