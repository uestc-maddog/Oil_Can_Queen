# Oil_Can操作指南 Oil_Can Manual

本套设备由一个主机(queen)和若干从机(drone)组成。Drone主要负责检测油罐的装盛状态，queen负责将该状态上传到服务器。


## queen 

* 设备采用专用适配器供电。
* 设备配备一块电阻式触摸屏和两个物理按键，其中一个为复位键，另一个为屏幕背光灯的开关键。
* 设备上电后自动检测附近的wifi热点，用户可以通过触摸屏来选取将要连入的wifi网络，正确输入密码后设备即可正常工作。
* 设备正常工作后，设备会实时显示、上传各个从设备(drone)的状态（包括锂电池的电量信息）。

### wifi扫描
wifi扫描结果会自动刷新

![wifi_scan.bmp](.\picture\wifi_scan.bmp)

### 输入wifi密码
支持数字及大小写字母输入

![PWD_enter.bmp](.\picture\PWD_enter.bmp)

### wifi连接成功
![wifi_ok.bmp](.\picture\wifi_ok.bmp)

### 从设备状态检测
状态信息包括油管的装盛状态和锂电池的电量信息

![main_UI.bmp](.\picture\main_UI.bmp)
## drone

* 设备采用便携式锂电池供电。
* 设备需正确安装在油罐顶部。
* 设备正常工作后，会定期获取油罐的装盛状态并发送到queen。
* 用户需要关注锂电池的电量，电量不足时请及时充电并更换电池。