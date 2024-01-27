/*
在线测试键盘：https://www.zfrontier.com/lab/keyboardTester
*/

/* 头文件 */
#include <stdlib.h>
#include "USBHIDComposite.h"
#include "WS2812.h"

/* 用户配置 */
//设备配置
#define LED_BRIGHTNESS    8       //LED亮度等级 值为 0~8，0为完全不亮，8为最亮
#define MAX_KEY_COUNT     3       //最大按键个数 此值取决于PCB，不要更改   
#define MAX_VAL_COUNT     5       //按键允许使用键值的最大个数，通常是三个按键中键值列表长度最大的列表的长度值
#define LONG_PRESS_TIME   300     //长按多少毫秒以上认为是长按

#define RAW_KEY_VAL(v)    ((v) + 136)

/*
可切换按键模式下不建议使用随机颜色，所以没有随机颜色的设置
定义两组按钮对应的LED的颜色，切换键组的时候也会切换颜色，用户可以根据按键颜色辨别键组
每组数据存有三个颜色，格式为RGB字符串，且如果有A~F必须是大写，不带“#”
例如 天依蓝 66CCFF 而不是 66ccff
如果不希望led点亮，把每个灯的颜色设置成黑色即 000000 即可，或者将亮度设置成0，但是不推荐，这会使单片机做多余的工作
每个颜色组内下标 0~2 分别为 KEY1~3 的颜色
*/
//键组颜色1
char Key_ColorList_1[MAX_KEY_COUNT][7] = {
  "FF0000", //键组1中按键1的颜色
  "00FF00", //键组1中按键2的颜色
  "0000FF", //键组1中按键3的颜色
};
//键组颜色2
char Key_ColorList_2[MAX_KEY_COUNT][7] = {
  "00FFFF", //键组2中按键1的颜色
  "FF00FF", //键组2中按键2的颜色
  "FFFF00", //键组2中按键3的颜色
};

/*
键值配置
按下按键后，单片机向电脑发送键值
  KEYx_VAL_COUNT 是该按键按下后将会触发几个键值，一般是需要多少填多少
  Keyx_ValList_x 是存放该组键键值的二维数组，内部的三个一维数组从下标 0~2 分别为 Key1~3 的键值
  例如先填写 KEY_LEFT_CTRL，再填写 KEY_LEFT_ALT ，发送给电脑时就会先发送 KEY_LEFT_CTRL，再发送 KEY_LEFT_ALT
  每个键值用“,”隔开，遵循C语言数组的规定
一共有三个按键，所以一共有三组配置，按键的序号1、2、3为从左到右的顺序
! ! ! 注意 ! ! !
所有键值都来自 @USBHIDComposite.h 文件内的宏定义
在按键配置内，只能使用宏定义内的“键盘键值”（@USBHIDComposite.h：10~58 行）或者ASCII码内的值，即
如果需要键盘按下A键，对应的键值就是 'a' ，也就是单引号引上这个字符，
但是不是所有值都可以用，具体哪些字符可以，参考 @USBHIDComposite.c: 21~151行，该名为 _asciimap 的数组内记录了所有可以直接使用ASCII码代替键值的按键

更多按键：参考文件 “HID Usage Tables V1.4.pdf”内的章节“10 Keyboard/Keypad Page (0x07)”
第一列“Usage ID”为16进制键值，输入到程序内是应加‘0x’；前缀，并且使用 RAW_KEY_VAL(v) 转化
例如键值 4C 为 “Keyboard Delete Forward” 的键值，输入到程序内应该是 RAW_KEY_VAL(0x4c)，此处的16进制由编译器转换，不区分大小写
不能使用键值超过 0x77 的按键，如果一定要使用，发送键值的函数必须从 Keyboard_press 换成 Keyboard_pressRaw，且键值不需要使用 RAW_KEY_VAL(v) 转化
*/ 

#define KEY1_VAL_COUNT_G1 1
#define KEY2_VAL_COUNT_G1 1
#define KEY3_VAL_COUNT_G1 1
__xdata uint8_t Key_ValList_1[MAX_KEY_COUNT][MAX_VAL_COUNT] = {
  //键组1中按键1的键值列表
  {
    KEY_LEFT_CTRL,
  },
  //键组1中按键2的键值列表
  {
    'c',
  },
  //键组1中按键3的键值列表
  {
    'v',
  },
};

#define KEY1_VAL_COUNT_G2 1
#define KEY2_VAL_COUNT_G2 3
#define KEY3_VAL_COUNT_G2 3
__xdata uint8_t Key_ValList_2[MAX_KEY_COUNT][MAX_VAL_COUNT] = {
  //键组2中按键1的键值列表
  {
    RAW_KEY_VAL(0x4c),
  },
  //键组2中按键2的键值列表
  {
    KEY_LEFT_CTRL,
    KEY_LEFT_ALT,
    'a',
  },
  //键组2中按键3的键值列表
  {
    KEY_LEFT_CTRL,
    KEY_LEFT_ALT,
    'u',
  },
};

/*
编码器配置 不建议更改
编码器可以顺时针和逆时针旋转，还可以按下去，按下去后仍然可以旋转
下面内容的 CW：Clockwise 顺时针；CCW：Counter Clockwise 逆时针
! ! ! 所有键值都来自 @USBHIDComposite.h 文件内的宏定义，在按键配置内，只能使用宏定义内的“多媒体键值”（@USBHIDComposite.h：60~64 行） ! ! !

更多按键：参考文件 “HID Usage Tables V1.4.pdf”内的章节“15 Consumer Page (0x0C)”
第一列“Usage ID”为16进制键值，输入到程序内是应加‘0x’；前缀，不需要使用 RAW_KEY_VAL(v) 转化
*/
#define EC11_CW_VAL           MUL_Volume_Increment        //编码器顺时针旋转时按下的键值
#define EC11_CW_PRESSED_VAL   MUL_Brightness_Increment    //编码器按下后顺时针旋转时按下的键值
#define EC11_CCW_VAL          MUL_Volume_Decrement        //编码器逆时针旋转时按下的键值
#define EC11_CCW_PRESSED_VAL  MUL_Brightness_Decrement    //编码器按下后逆时针旋转时按下的键值

/*
按下松开策略配置
! ! ! 此配置进针对三个按键生效，且必须3选1 ! ! !
解释
  SEND_CFG_NONBLOCK_HAND（非阻塞手控）：三键没有使用冲突，类似于普通键盘，按下该按键后，不影响其他按键的使用，手控的意思是按下触发，抬起释放
  SEND_CFG_SINGLE_CLICK（单次点击）：按下该按键后，单片机会以 SEND_CFG_INTERVAL 为间隔（单位：ms）向电脑发送按键，类似于快速点击，直到松开该按键，停止发送
  SEND_CFG_HAND（阻塞手控）：按下按键后，程序被阻塞，但是按键只会发送一次，直到松开该按键，才会释放按下的按键
需要的取消注释，不需要的注释即可
*/
#define SEND_CFG_NONBLOCK_HAND 1  //非阻塞手控
//#define SEND_CFG_SINGLE_CLICK   1     //单次点击
//#define SEND_CFG_HAND           1     //阻塞手控
#define SEND_CFG_INTERVAL 15  //发送按键间隔（单位：ms），影响 单次点击 模式下的按键值发送速度

/*
防抖延时
为了消除按键抖动导致的误发送，程序判断到按下按键后，当延时该值定义的时间（单位：ms）后，按键还处于按下状态时，才会触发按键事件
该值会影响按键的灵敏度，如果设定过大，会导致快速点按时相应不迅速，如果设定过小，会导致消抖不明显，进而导致误触发
该值的推荐范围为 10~50
*/
#define DITHERING_DELAY 20  //防抖延时

/*板载资源配置
该配置通常固定内容，不需要更改
*/
#define NUM_LEDS 3
#define COLOR_PER_LEDS 3
#define NUM_BYTES (NUM_LEDS * COLOR_PER_LEDS)
#define KEY1_PIN 15
#define KEY2_PIN 16
#define KEY3_PIN 17
#define EC11_A_PIN 34
#define EC11_B_PIN 33
#define EC11_D_PIN 14
#define LED_PIN 32
#define SET_BIT(val, bits)      ((val) |= (1 << (bits)))
#define CLEAR_BIT(val, bits)    ((val) &= ~(1 << (bits)))
#define	GET_BIT(val, bit)	      (((val) & (1 << (bit))) >> (bit)) 

/*配置检查
检查用户配置是否有问题
*/
#if (SEND_CFG_NONBLOCK_HAND + SEND_CFG_SINGLE_CLICK + SEND_CFG_HAND != 1)
#error "键盘发送配置必须3选1"
#endif

//打开 Arduino IDE 之后在 菜单栏->工具->“USB Settings:” (展开) -> 选择“USER CODE w/ 148B USB ram”（第二个选项），否者会触发下面的报错
#ifndef USER_USB_RAM
#error "This example needs to be compiled with a USER USB setting"
#endif

/* 全局变量 */
__xdata uint8_t LedData[NUM_BYTES];  //LED颜色数组
unsigned long LastColorUpdate = 0;   //记录上次LED颜色更新时间
int EC11_Flag = 0;                   //标志位
bool EC11_CW_1 = 0;
bool EC11_CW_2 = 0;
int EC11_Cnt = 0;
uint8_t EC11_OldState;
uint8_t CurCfg = 0;
uint8_t LongPress = 0;
#if SEND_CFG_NONBLOCK_HAND
uint8_t Key1_Changed, Key2_Changed, Key3_Changed;
#endif  //SEND_CFG_NONBLOCK_HAND


/* 函数声明 */
void RGBStr2Val(const char* str, uint8_t* R, uint8_t* G, uint8_t* B);
void USBPressKeyList(uint8_t pin, uint8_t* keyList, uint8_t keyCnt);
void MySetPixelColor(__xdata uint8_t* addr, uint8_t index, uint8_t R, uint8_t G, uint8_t B);
void USBReleaseKeyList(uint8_t* keyList, uint8_t keyCnt);
void UpdateColor();
void EC11_INT();
void EC11_CW();
void EC11_CCW();

void setup() {
  USBInit();                      //初始化USB HID
  memset(LedData, 0, NUM_BYTES);  //清空LedData，并发送，即重置所有LED
  neopixel_show_P3_2(LedData, NUM_BYTES);
  EC11_OldState = digitalRead(EC11_A_PIN);  //记录EC11旧状态，用于更新判断
  UpdateColor();
}

void loop() {
  if (digitalRead(KEY1_PIN) == LOW) {
    delay(DITHERING_DELAY);
    if (digitalRead(KEY1_PIN) == LOW) {
#if SEND_CFG_NONBLOCK_HAND
      Key1_Changed = 1;
#endif  //SEND_CFG_NONBLOCK_HAND
      USBPressKeyList(KEY1_PIN, CurCfg ? Key_ValList_2[0] : Key_ValList_1[0], CurCfg ? KEY1_VAL_COUNT_G2 : KEY1_VAL_COUNT_G1);
    }
  }

  if (digitalRead(KEY2_PIN) == LOW) {
    delay(DITHERING_DELAY);
    if (digitalRead(KEY2_PIN) == LOW) {
#if SEND_CFG_NONBLOCK_HAND
      Key2_Changed = 1;
#endif  //SEND_CFG_NONBLOCK_HAND
      USBPressKeyList(KEY2_PIN, CurCfg ? Key_ValList_2[1] : Key_ValList_1[1], CurCfg ? KEY2_VAL_COUNT_G2 : KEY2_VAL_COUNT_G1);
    }
  }

  if (digitalRead(KEY3_PIN) == LOW) {
    delay(DITHERING_DELAY);
    if (digitalRead(KEY3_PIN) == LOW) {
#if SEND_CFG_NONBLOCK_HAND
      Key3_Changed = 1;
#endif  //SEND_CFG_NONBLOCK_HAND
      USBPressKeyList(KEY3_PIN, CurCfg ? Key_ValList_2[2] : Key_ValList_1[2], CurCfg ? KEY3_VAL_COUNT_G2 : KEY3_VAL_COUNT_G1);
    }
  }


#if SEND_CFG_NONBLOCK_HAND
  //非阻塞手动控制 - 抬手检测
  if (Key1_Changed && digitalRead(KEY1_PIN) == HIGH) {
    Key1_Changed = 0;
    USBReleaseKeyList(CurCfg ? Key_ValList_2[0] : Key_ValList_1[0], CurCfg ? KEY1_VAL_COUNT_G2 : KEY1_VAL_COUNT_G1);
  }

  if (Key2_Changed && digitalRead(KEY2_PIN) == HIGH) {
    Key2_Changed = 0;
    USBReleaseKeyList(CurCfg ? Key_ValList_2[1] : Key_ValList_1[1], CurCfg ? KEY2_VAL_COUNT_G2 : KEY2_VAL_COUNT_G1);
  }

  if (Key3_Changed && digitalRead(KEY3_PIN) == HIGH) {
    Key3_Changed = 0;
    USBReleaseKeyList(CurCfg ? Key_ValList_2[2] : Key_ValList_1[2], CurCfg ? KEY3_VAL_COUNT_G2 : KEY3_VAL_COUNT_G1);
  }
#endif  //SEND_CFG_NONBLOCK_HAND


  if (digitalRead(EC11_D_PIN) == LOW && !LongPress)
  {
    delay(LONG_PRESS_TIME);
    if (digitalRead(EC11_D_PIN) == HIGH)
    {
      Keyboard_releaseAll();
      MulCtrl_releaseAll();
      CurCfg = !CurCfg;

      UpdateColor();
    }
    else //按键时间 > LONG_PRESS_TIME 认定为长按
    {
      LongPress = 1;
    }
  }

  if (digitalRead(EC11_A_PIN) != EC11_OldState)
  {
    EC11_INT();
    EC11_OldState = digitalRead(EC11_A_PIN);
  }

  //重置LongPress状态
  if (LongPress && digitalRead(EC11_D_PIN) == HIGH)
  {
    LongPress = 0;
  }
}

/*
@brief RGB字符串转 uint8_t 分量
@param str RGB 字符串
@param R 红色分量指针
@param G 绿色分量指针
@param B 蓝色分量指针
*/
void RGBStr2Val(const char* str, uint8_t* R, uint8_t* G, uint8_t* B) {
  *R = (str[0] <= '9' ? str[0] - '0' : (str[0] - 'A' + 10)) * 16 + (str[1] <= '9' ? str[1] - '0' : (str[1] - 'A' + 10));
  *G = (str[2] <= '9' ? str[2] - '0' : (str[2] - 'A' + 10)) * 16 + (str[3] <= '9' ? str[3] - '0' : (str[3] - 'A' + 10));
  *B = (str[4] <= '9' ? str[4] - '0' : (str[4] - 'A' + 10)) * 16 + (str[5] <= '9' ? str[5] - '0' : (str[5] - 'A' + 10));
}

/*
@brief 依次发送键值列表里的值
@param pin 该按键的pin脚
@param keyList 键值列表
@param keyCnt 该键值列表内的键值个数
*/
void USBPressKeyList(uint8_t pin, uint8_t* keyList, uint8_t keyCnt) {
  for (uint8_t i = 0; i < keyCnt; i++) {
    Keyboard_press(keyList[i]);
    delay(SEND_CFG_INTERVAL);
  }

  //策略1 - 单次发送
#if SEND_CFG_SINGLE_CLICK
  delay(100);
  Keyboard_releaseAll();
#endif  //SEND_CFG_SINGLE_CLICK

  //策略2 - 手动控制
#if SEND_CFG_HAND
  while (digitalRead(pin) == LOW);
  Keyboard_releaseAll();
#endif  //SEND_CFG_HAND

  //策略3 - 非阻塞手动控制
#if SEND_CFG_NONBLOCK_HAND

#endif  //SEND_CFG_NONBLOCK_HAND
}

/*
@brief 依次释放键值列表里的值
@param keyList 键值列表
@param keyCnt 该键值列表内的键值个数
*/
void USBReleaseKeyList(uint8_t* keyList, uint8_t keyCnt) {
  for (uint8_t i = 0; i < keyCnt; i++) {
    Keyboard_release(keyList[i]);
  }
}

/*
@brief 编码器顺时针旋转时触发的函数
*/
void EC11_CW() {
  if (digitalRead(EC11_D_PIN) == LOW) {
    MulCtrl_press(EC11_CW_PRESSED_VAL);
  } else {
    MulCtrl_press(EC11_CW_VAL);
  }
  delay(5);
  MulCtrl_releaseAll();
}

/*
@brief 编码器逆时针旋转时触发的函数
*/
void EC11_CCW() {
  if (digitalRead(EC11_D_PIN) == LOW) {
    MulCtrl_press(EC11_CCW_PRESSED_VAL);
  } else {
    MulCtrl_press(EC11_CCW_VAL);
  }
  delay(5);
  MulCtrl_releaseAll();
}

/*
@brief 伪中断服务函数：编码器变化时触发的函数
*/
void EC11_INT() {
  // 只要处理一个脚的外部中断--上升沿&下降沿
  int alv = digitalRead(EC11_A_PIN);
  int blv = digitalRead(EC11_B_PIN);
  if (EC11_Flag == 0 && alv == LOW) {
    EC11_CW_1 = blv;
    EC11_Flag = 1;
  }
  if (EC11_Flag && alv) {
    EC11_CW_2 = !blv;  //取反是因为 alv,blv必然异步，一高一低
    if (EC11_CW_1 && EC11_CW_2) {
      EC11_Cnt++;
      EC11_CCW();
    }
    if (EC11_CW_1 == 0 && EC11_CW_2 == 0) {
      EC11_Cnt--;
      EC11_CW();
    }
    EC11_Flag = 0;
  }
}

void MySetPixelColor(__xdata uint8_t* addr, uint8_t index, uint8_t R, uint8_t G, uint8_t B)
{
  set_pixel_for_GRB_LED(addr, index, R >> (8 - LED_BRIGHTNESS), G >> (8 - LED_BRIGHTNESS), B >> (8 - LED_BRIGHTNESS));
}

void UpdateColor()
{
  uint8_t R = 0, G = 0, B = 0;
  RGBStr2Val(CurCfg ? Key_ColorList_2[0] : Key_ColorList_1[0], &R, &G, &B);
  MySetPixelColor(LedData, 0, R, G, B);
  RGBStr2Val(CurCfg ? Key_ColorList_2[1] : Key_ColorList_1[1], &R, &G, &B);
  MySetPixelColor(LedData, 1, R, G, B);
  RGBStr2Val(CurCfg ? Key_ColorList_2[2] : Key_ColorList_1[2], &R, &G, &B);
  MySetPixelColor(LedData, 2, R, G, B);
  neopixel_show_P3_2(LedData, NUM_BYTES);
}
