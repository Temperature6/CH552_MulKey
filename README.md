# CH552 多功能键盘

![](.\pics\1706341786840.jpg)

### 简介

复刻自 https://oshwhub.com/pomin/dial-san-jian-xiao-jian-pan ，把USB Micro接口改成了USB-TypeC，加入了烧录按键。采用Arduino开发，更简单。

#### 功能

给出了两种代码模板：

**CH552_Keyboard_Normal**：普通三键小键盘，自定义按键（需要重新编译固件）支持随机颜色灯光，指定颜色灯光，旋转旋钮调整声音，按下并旋转旋钮调整亮度

**CH552_Keyboard_Switch**：支持使用点按编码器的方式切换两套按键，以不同的LED颜色（可自定义）表示，仍然支持，旋转旋钮调整声音，按下并旋转旋钮调整亮度

根据自己需要更改代码中的宏定义即可自定义多种功能

### 编译|烧录

参考 [基于CH552G主控的开源九键小键盘(资料齐全)_ch552g自制键盘-CSDN博客](https://blog.csdn.net/qq_53381910/article/details/132516628)配置CH552G的Arduino开发环境，烧录文中也有提到

### 自定义功能以及按键

程序的特点在于使用大量的宏定义来配置程序，用户根据自己的需要配置即可，代码内也有大量的注释引导用户配置

### 参考

https://oshwhub.com/pomin/dial-san-jian-xiao-jian-pan

https://blog.csdn.net/qq_28145393/article/details/125516852

https://blog.csdn.net/qq_53381910/article/details/132516628

https://github.com/wagiminator/CH552-USB-Knob

https://github.com/xuan25/HIDMediaController-ArduinoMicroPro

https://github.com/DeqingSun/ch55xduino

