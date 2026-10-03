/* 菜单与显示模块：保存菜单位置、已确认设置、编辑草稿和测量值。
 * main.c 通过 Menu_Get... 读取设置，通过 Menu_Set... 更新显示数据。
 * 菜单页面只负责绘制；实际 GPIO、PWM、采集和报警判定由其他模块执行。
 */
#include "Menu.h"
#include "OLED.h"
#include "Input.h"
#include "LED.h"


#define MENU_VISIBLE_ROWS  4

/*====================================================
    1. 类型定义
====================================================*/

typedef enum
{
    MENU_STATE_MENU = 0,    /* 浏览菜单列表 */
    MENU_STATE_PAGE,        /* 浏览功能页面 */
    MENU_STATE_ADJUST       /* 修改待确认参数 */
} Menu_State_t;

/* 前五个成员沿用参考项目。
   最后两个成员用于保存每一级菜单的位置。 */
typedef struct MenuItem
{
    const char *name;               /* 菜单名称 */
    void (*action)(void);           /* 页面显示函数 */
    struct MenuItem *parent;        /* 父菜单 */
    struct MenuItem **children;     /* 子菜单数组 */
    uint8_t childCount;             /* 子菜单数量 */

    uint8_t savedSelected;          /* 上次选中的项目 */
    uint8_t savedTopIndex;          /* 上次显示的第一项 */
} MenuItem;


/*====================================================
    2. 内部函数声明
====================================================*/

static void Menu_Show(void);
static void Menu_ShowCursor(void);
static void Menu_Enter(void);
static void Menu_Back(void);
static void Menu_RefreshPage(void);

static void Menu_StartAdjust(void);
static void Menu_AdjustValue(Input_Event_t Event);
static void Menu_SaveAdjust(void);

static void Overview_Page(void);
static void LED_Page(void);
static void Voltage_Page(void);
static void Temperature_Page(void);
static void Servo_Page(void);
static void LowLimit_Page(void);
static void HighLimit_Page(void);

static void Limit_Page(const char *Title, int16_t Value);
static void ShowVoltage(uint8_t Line, uint8_t Column);
static void ShowTemperature(uint8_t Line, uint8_t Column);


/*====================================================
    3. 菜单项提前声明
====================================================*/

static MenuItem RootMenu;
static MenuItem SettingsMenu;

static MenuItem OverviewItem;
static MenuItem LEDItem;
static MenuItem VoltageItem;
static MenuItem TemperatureItem;
static MenuItem ServoItem;

static MenuItem LowLimitItem;
static MenuItem HighLimitItem;


/*====================================================
    4. 子菜单数组
====================================================*/

/* 设置菜单只有温度下限和温度上限。 */
static MenuItem *SettingsChildren[] =
{
    &LowLimitItem,
    &HighLimitItem
};

/* 数组顺序就是根菜单显示顺序；根菜单的 childCount 与这里的项目数对应。 */
static MenuItem *RootChildren[] =
{
    &OverviewItem,
    &LEDItem,
    &VoltageItem,
    &TemperatureItem,
    &ServoItem,
    &SettingsMenu
};


/*====================================================
    5. 菜单数据
====================================================*/

/* 初始化顺序：
   name、action、parent、children、childCount、
   savedSelected、savedTopIndex。 */

static MenuItem RootMenu =
{
    "ROOT", 0, 0, RootChildren, 6, 0, 0
};

static MenuItem OverviewItem =
{
    "Overview", Overview_Page, &RootMenu, 0, 0, 0, 0
};

static MenuItem LEDItem =
{
    "LED", LED_Page, &RootMenu, 0, 0, 0, 0
};

static MenuItem VoltageItem =
{
    "Voltage", Voltage_Page, &RootMenu, 0, 0, 0, 0
};

static MenuItem TemperatureItem =
{
    "Temperature", Temperature_Page, &RootMenu, 0, 0, 0, 0
};

static MenuItem ServoItem =
{
    "Servo Angle", Servo_Page, &RootMenu, 0, 0, 0, 0
};

static MenuItem SettingsMenu =
{
    "Settings", 0, &RootMenu, SettingsChildren, 2, 0, 0
};

static MenuItem LowLimitItem =
{
    "Low Limit", LowLimit_Page, &SettingsMenu, 0, 0, 0, 0
};

static MenuItem HighLimitItem =
{
    "High Limit", HighLimit_Page, &SettingsMenu, 0, 0, 0, 0
};


/*====================================================
    6. 当前菜单及项目数据
====================================================*/

/* CurrentMenu 指向所属列表，CurrentPage 指向当前功能页；两者用途不同。 */
static MenuItem *CurrentMenu = &RootMenu;
static MenuItem *CurrentPage = 0;

static Menu_State_t Menu_State = MENU_STATE_MENU;

/* selected表示整张列表中选中了哪项；
   topIndex表示屏幕第一行对应哪项。 */
static uint8_t selected = 0;
static uint8_t topIndex = 0;

/* 灯和舵机的控制设定。
   本文件只保存设定，不直接控制GPIO或PWM。
   后台硬件程序通过Menu_Get...函数读取。 */
static uint8_t LEDEnabled = 0;
static uint16_t ServoAngle = 90;

/* 温度阈值，单位为整数℃。 */
static int16_t LowLimit = 10;
static int16_t HighLimit = 35;

/* EditValue是待确认值。
   旋转只修改它；ENTER确认后才修改实际设定。 */
static int16_t EditValue = 0;

/* 来自外设驱动的测量数据。
   未取得数据时，页面显示横线。 */
static uint16_t VoltageMv = 0;
static uint8_t VoltageValid = 0;

/* Temperature10 单位为 0.1℃；Valid 单独表示有效性，数值 0 本身也可是真实温度。 */
static int16_t Temperature10 = 0;
static uint8_t TemperatureValid = 0;
static Alarm_State_t AlarmState = ALARM_WAIT;


/*====================================================
    7. 初始化
====================================================*/

void Menu_Init(void)
{
    CurrentMenu = &RootMenu;
    CurrentPage = 0;

    Menu_State = MENU_STATE_MENU;
    selected = 0;
    topIndex = 0;

    RootMenu.savedSelected = 0;
    RootMenu.savedTopIndex = 0;
    SettingsMenu.savedSelected = 0;
    SettingsMenu.savedTopIndex = 0;

    /* 上电默认设置实际在这里生效，会覆盖文件顶部同名变量的静态初值。
     * 当前设定：流水灯关闭、舵机 0°、温度下限 10℃、上限 30℃。
     * 这些设置保存在 RAM，当前工程没有把菜单设定写入 Flash 的流程。
     */
    LEDEnabled = 0;
    ServoAngle = 0;

    LowLimit = 10;
    HighLimit = 30;
    EditValue = 0;

    VoltageValid = 0;
    TemperatureValid = 0;
	AlarmState = ALARM_WAIT;

    Menu_Show();
    Menu_ShowCursor();
}


/*====================================================
    8. 统一处理输入
====================================================*/

/* 同一个事件按当前状态解释：列表中移动/进入，页面中返回/编辑，
 * 编辑中增减草稿/确认/取消。按 BACK 取消编辑后，再按一次才离开页面。
 */
void Menu_Process(Input_Event_t Event)
{
    /* 没有输入时，不重复刷新OLED。 */
    if (Event == INPUT_NONE)
    {
        return;
    }

    /*---------------- 浏览菜单 ----------------*/
    if (Menu_State == MENU_STATE_MENU)
    {
        if (Event == INPUT_DOWN)
        {
            /* 已在最后一项时，不继续向下。 */
            if (selected + 1 < CurrentMenu->childCount)
            {
                selected++;

                /* 选中项走出屏幕底部，窗口向下移一项。 */
                if (selected >= topIndex + MENU_VISIBLE_ROWS)
                {
                    topIndex++;
                    Menu_Show();
                }

                Menu_ShowCursor();
            }
        }
        else if (Event == INPUT_UP)
        {
            /* 已在第一项时，不继续向上。 */
            if (selected > 0)
            {
                selected--;

                /* 选中项走出屏幕顶部，窗口向上移一项。 */
                if (selected < topIndex)
                {
                    topIndex--;
                    Menu_Show();
                }

                Menu_ShowCursor();
            }
        }
        else if (Event == INPUT_ENTER)
        {
            Menu_Enter();
        }
        else if (Event == INPUT_BACK)
        {
            Menu_Back();
        }

        return;
    }

    /*---------------- 编辑参数 ----------------*/
    if (Menu_State == MENU_STATE_ADJUST)
    {
        if (Event == INPUT_UP || Event == INPUT_DOWN)
        {
            Menu_AdjustValue(Event);
            Menu_RefreshPage();
        }
        else if (Event == INPUT_ENTER)
        {
            /* 将待确认值写入实际设定。 */
            Menu_SaveAdjust();
            Menu_State = MENU_STATE_PAGE;
            Menu_RefreshPage();
        }
        else if (Event == INPUT_BACK)
        {
            /* 只取消本次编辑，仍留在当前页面。 */
            Menu_State = MENU_STATE_PAGE;
            Menu_RefreshPage();
        }

        return;
    }

    /*---------------- 浏览功能页 ----------------*/
    if (Event == INPUT_BACK)
    {
        Menu_Back();
    }
    else if (Event == INPUT_ENTER)
    {
        if (CurrentPage == &LEDItem)
        {
            /* ENTER切换流水灯的运行设定。 */
            LEDEnabled = !LEDEnabled;
            Menu_RefreshPage();
        }
        else if (CurrentPage == &ServoItem ||
                 CurrentPage == &LowLimitItem ||
                 CurrentPage == &HighLimitItem)
        {
            Menu_StartAdjust();
            Menu_RefreshPage();
        }
    }

    /* 普通功能页的UP/DOWN不执行操作。
       进入编辑后，旋转才用于增减参数。 */
}


/*====================================================
    9. 显示菜单与光标
====================================================*/

static void Menu_Show(void)
{
    uint8_t i;
    uint8_t itemIndex;

    OLED_Clear();

    for (i = 0; i < MENU_VISIBLE_ROWS; i++)
    {
        itemIndex = topIndex + i;

        if (itemIndex >= CurrentMenu->childCount)
        {
            break;
        }

        /* 第1个汉字列留给光标和间隔。
           名称从汉字第2列，即像素X=16开始。

           现有ShowChinese也支持ASCII混合显示，
           因此当前英文名称可以直接使用；
           将来换成中文名称，不必再改显示函数。 */
        OLED_ShowChinese(
            i + 1,
            2,
            CurrentMenu->children[itemIndex]->name
        );
    }
}

static void Menu_ShowCursor(void)
{
    uint8_t i;

    /* 清除旧光标，只清第1个ASCII位置。 */
    for (i = 0; i < MENU_VISIBLE_ROWS; i++)
    {
        OLED_ShowChar(i + 1, 1, ' ');
    }

    /* selected-topIndex是屏幕内的行偏移。
       OLED行号从1开始，所以最后加1。 */
    OLED_ShowChar(selected - topIndex + 1, 1, '>');
}


/*====================================================
    10. ENTER进入菜单或页面
====================================================*/

static void Menu_Enter(void)
{
    MenuItem *item;

    item = CurrentMenu->children[selected];

    /* 离开当前列表前，保存选择与滚动位置。 */
    CurrentMenu->savedSelected = selected;
    CurrentMenu->savedTopIndex = topIndex;

    if (item->childCount > 0)
    {
        /* 进入子菜单，恢复该级上次的位置。 */
        CurrentMenu = item;

        selected = CurrentMenu->savedSelected;
        topIndex = CurrentMenu->savedTopIndex;

        Menu_Show();
        Menu_ShowCursor();
    }
    else if (item->action != 0)
    {
        /* 进入功能页，只显示一次，不在页面中死循环。 */
        CurrentPage = item;
        Menu_State = MENU_STATE_PAGE;

        OLED_Clear();
        CurrentPage->action();
    }
}


/*====================================================
    11. BACK返回
====================================================*/

static void Menu_Back(void)
{
    if (Menu_State == MENU_STATE_PAGE)
    {
        /* 功能页回到进入前的列表。
           CurrentMenu一直指向该列表，不需要重新初始化。 */
        CurrentPage = 0;
        Menu_State = MENU_STATE_MENU;

        selected = CurrentMenu->savedSelected;
        topIndex = CurrentMenu->savedTopIndex;

        Menu_Show();
        Menu_ShowCursor();
        return;
    }

    if (CurrentMenu->parent != 0)
    {
        /* 保存子菜单位置，以便下次进入时恢复。 */
        CurrentMenu->savedSelected = selected;
        CurrentMenu->savedTopIndex = topIndex;

        /* 回父菜单，并恢复父菜单原来的显示窗口。 */
        CurrentMenu = CurrentMenu->parent;

        selected = CurrentMenu->savedSelected;
        topIndex = CurrentMenu->savedTopIndex;

        Menu_Show();
        Menu_ShowCursor();
    }

    /* 根菜单没有父菜单，BACK不执行操作。 */
}


/*====================================================
    12. 参数编辑
====================================================*/

/* 先复制已确认值到 EditValue；调整草稿时硬件仍使用原来的确认值。 */
static void Menu_StartAdjust(void)
{
    if (CurrentPage == &ServoItem)
    {
        EditValue = ServoAngle;
    }
    else if (CurrentPage == &LowLimitItem)
    {
        EditValue = LowLimit;
    }
    else if (CurrentPage == &HighLimitItem)
    {
        EditValue = HighLimit;
    }
    else
    {
        return;
    }

    Menu_State = MENU_STATE_ADJUST;
}

static void Menu_AdjustValue(Input_Event_t Event)
{
    int16_t minimum;
    int16_t maximum;
    int16_t step = 1;       /* 默认每次增减1 */

    /* 根据正在编辑的项目，设置范围和步长 */
    if (CurrentPage == &ServoItem)
    {
        minimum = 0;
        maximum = 180;
        step = 10;         /* 舵机每次增减10° */
    }
    else if (CurrentPage == &LowLimitItem)
    {
        minimum = -55;
        /* 编辑下限时至少比上限低 2℃；编辑上限时也保持相同间隔。 */
        maximum = HighLimit - 2;
    }
    else if (CurrentPage == &HighLimitItem)
    {
        minimum = LowLimit + 2;
        maximum = 125;
    }
    else
    {
        return;
    }

    /* 顺时针增加 */
    if (Event == INPUT_DOWN)
    {
        EditValue += step;

        /* 超出上限时，停在上限 */
        if (EditValue > maximum)
        {
            EditValue = maximum;
        }
    }
    /* 逆时针减少 */
    else if (Event == INPUT_UP)
    {
        EditValue -= step;

        /* 超出下限时，停在下限 */
        if (EditValue < minimum)
        {
            EditValue = minimum;
        }
    }
}

/* 只有确认操作才提交 EditValue，主循环随后读取新设定并执行。 */
static void Menu_SaveAdjust(void)
{
    if (CurrentPage == &ServoItem)
    {
        ServoAngle = EditValue;
    }
    else if (CurrentPage == &LowLimitItem)
    {
        LowLimit = EditValue;
    }
    else if (CurrentPage == &HighLimitItem)
    {
        HighLimit = EditValue;
    }
}


/*====================================================
    13. 页面刷新
====================================================*/

static void Menu_RefreshPage(void)
{
    /* 页面函数里不清全屏、不等待按键，
       每次画完当前内容就返回。 */
    if (Menu_State != MENU_STATE_MENU &&
        CurrentPage != 0 &&
        CurrentPage->action != 0)
    {
        CurrentPage->action();
    }
}


/*====================================================
    14. 电压和温度的数字显示
====================================================*/

static void ShowVoltage(uint8_t Line, uint8_t Column)
{
    uint16_t Voltage100;

    if (!VoltageValid)
    {
        OLED_ShowString(Line, Column, "-.--V");
        return;
    }

    /* 毫伏换算成0.01V，先加5再除10用于四舍五入。
       例如1650mV -> 165 -> 1.65V。 */
    Voltage100 = (VoltageMv + 5) / 10;

    OLED_ShowNum(Line, Column, Voltage100 / 100, 1);
    OLED_ShowChar(Line, Column + 1, '.');
    OLED_ShowNum(Line, Column + 2, Voltage100 % 100, 2);
    OLED_ShowChar(Line, Column + 4, 'V');
}

static void ShowTemperature(uint8_t Line, uint8_t Column)
{
    uint16_t magnitude;
    uint8_t digitColumn;

    /* 清除上次内容，避免正负号或位数变化后残留。 */
    OLED_ShowString(Line, Column, "      ");

    if (!TemperatureValid)
    {
        OLED_ShowString(Line, Column, "--.-C");
        return;
    }

    digitColumn = Column;

    if (Temperature10 < 0)
    {
        OLED_ShowChar(Line, digitColumn, '-');
        digitColumn++;
        magnitude = (uint16_t)(-Temperature10);
    }
    else
    {
        magnitude = (uint16_t)Temperature10;
    }

    /* 不强制补前导零：
       25.3、-5.2、125.0都显示正确的位数。 */
    if (magnitude >= 1000)
    {
        OLED_ShowNum(Line, digitColumn, magnitude / 10, 3);
        digitColumn += 3;
    }
    else if (magnitude >= 100)
    {
        OLED_ShowNum(Line, digitColumn, magnitude / 10, 2);
        digitColumn += 2;
    }
    else
    {
        OLED_ShowNum(Line, digitColumn, magnitude / 10, 1);
        digitColumn++;
    }

    OLED_ShowChar(Line, digitColumn, '.');
    OLED_ShowNum(Line, digitColumn + 1, magnitude % 10, 1);
    OLED_ShowChar(Line, digitColumn + 2, 'C');
}


/*====================================================
    15. 项目功能页面
====================================================*/

static void Overview_Page(void)
{
    OLED_ShowString(1, 1, "Voltage:");
    ShowVoltage(1, 9);

    OLED_ShowString(2, 1, "Temp:");
    ShowTemperature(2, 7);

    OLED_ShowString(3, 1, "LED:");
    OLED_ShowString(3, 5, LEDEnabled ? "ON " : "OFF");

    OLED_ShowString(3, 9, "A:");
    OLED_ShowNum(3, 11, ServoAngle, 3);

    OLED_ShowString(4, 1, "BACK: Return");
}

static void LED_Page(void)
{
    OLED_ShowString(1, 1, "LED");

    OLED_ShowString(2, 1, "State:");
    OLED_ShowString(2, 8, LEDEnabled ? "ON " : "OFF");

    OLED_ShowString(3, 1, "ENTER: ON/OFF");
    OLED_ShowString(4, 1, "BACK: Return");
}

static void Voltage_Page(void)
{
    OLED_ShowChinese(1, 1, "电压");
    ShowVoltage(2, 6);

    OLED_ShowString(4, 1, "BACK: Return");
}

static void Temperature_Page(void)
{
    OLED_ShowChinese(1, 1, "温度");
    ShowTemperature(2, 6);

    OLED_ShowString(3, 1, "L:");
    OLED_ShowSignedNum(3, 3, LowLimit, 3);

    OLED_ShowString(3, 8, "H:");
    OLED_ShowSignedNum(3, 10, HighLimit, 3);

    /* 显示后台实际报警状态，与蜂鸣器保持一致。 */
    switch (AlarmState)
    {
    case ALARM_LOW:
        OLED_ShowString(4, 1, "LOW!   B:Back   ");
        break;

    case ALARM_HIGH:
        OLED_ShowString(4, 1, "HIGH!  B:Back   ");
        break;

    case ALARM_FAULT:
        OLED_ShowString(4, 1, "FAULT! B:Back   ");
        break;

    case ALARM_NORMAL:
        OLED_ShowString(4, 1, "Normal B:Back   ");
        break;

    default:
        OLED_ShowString(4, 1, "WAIT   B:Back   ");
        break;
    }
}

static void Servo_Page(void)
{
    OLED_ShowChinese(1, 1, "舵机角度");

    OLED_ShowString(2, 1, "Target:");
    OLED_ShowNum(2, 10, ServoAngle, 3);

    /* 编辑和浏览的第三、四行不同，
       先清这两行，避免旧提示残留。 */
    OLED_ShowString(3, 1, "                ");
    OLED_ShowString(4, 1, "                ");

    if (Menu_State == MENU_STATE_ADJUST)
    {
        OLED_ShowString(3, 1, "Draft:");
        OLED_ShowNum(3, 10, EditValue, 3);

        OLED_ShowString(4, 1, "E:Save B:Cancel");
    }
    else
    {
        OLED_ShowString(3, 1, "ENTER: Edit");
        OLED_ShowString(4, 1, "BACK: Return");
    }
}

/* 上下限页面布局相同，复用这一小段显示代码。 */
static void Limit_Page(const char *Title, int16_t Value)
{
    OLED_ShowString(1, 1, Title);

    OLED_ShowString(2, 1, "Current:");
    OLED_ShowSignedNum(2, 10, Value, 3);

    OLED_ShowString(3, 1, "                ");
    OLED_ShowString(4, 1, "                ");

    if (Menu_State == MENU_STATE_ADJUST)
    {
        OLED_ShowString(3, 1, "Draft:");
        OLED_ShowSignedNum(3, 10, EditValue, 3);

        OLED_ShowString(4, 1, "E:Save B:Cancel");
    }
    else
    {
        OLED_ShowString(3, 1, "ENTER: Edit");
        OLED_ShowString(4, 1, "BACK: Return");
    }
}

static void LowLimit_Page(void)
{
    Limit_Page("Low Limit", LowLimit);
}

static void HighLimit_Page(void)
{
    Limit_Page("High Limit", HighLimit);
}


/*====================================================
    16. 后续硬件接入接口
====================================================*/

void Menu_SetVoltage(uint16_t Millivolts)
{
    /* 本项目PA0测量范围为0～3.3V。 */
    if (Millivolts > 3300)
    {
        Millivolts = 3300;
    }

    /* 数据没有改变，不重复写OLED。 */
    if (VoltageValid && VoltageMv == Millivolts)
    {
        return;
    }

    VoltageMv = Millivolts;
    VoltageValid = 1;

    /* 只更新使用电压数据的页面。 */
    if (CurrentPage == &VoltageItem ||
        CurrentPage == &OverviewItem)
    {
        Menu_RefreshPage();
    }
}

/* Value10=253 表示 25.3℃；Valid=0 时仅显示占位符，不把它当实测温度。 */
void Menu_SetTemperature(int16_t Value10, uint8_t Valid)
{
    Valid = Valid ? 1 : 0;

    /* 超过DS18B20测量范围的数据视为无效。 */
    if (Value10 < -550 || Value10 > 1250)
    {
        Valid = 0;
    }

    if (Temperature10 == Value10 &&
        TemperatureValid == Valid)
    {
        return;
    }

    Temperature10 = Value10;
    TemperatureValid = Valid;

    if (CurrentPage == &TemperatureItem ||
        CurrentPage == &OverviewItem)
    {
        Menu_RefreshPage();
    }
}

uint8_t Menu_GetLEDEnabled(void)
{
    return LEDEnabled;
}

uint16_t Menu_GetServoAngle(void)
{
    return ServoAngle;
}

uint8_t Menu_SetServoAngle(uint16_t Angle)
{
    if (Angle > 180)
    {
        return 0;
    }

    ServoAngle = Angle;

    /* 串口修改角度时，取消舵机页的旧编辑值。
       温度阈值编辑不受影响。 */
    if (CurrentPage == &ServoItem &&
        Menu_State == MENU_STATE_ADJUST)
    {
        Menu_State = MENU_STATE_PAGE;
    }

    if (CurrentPage == &ServoItem ||
        CurrentPage == &OverviewItem)
    {
        Menu_RefreshPage();
    }

    return 1;
}

int16_t Menu_GetLowLimit(void)
{
    return LowLimit;
}

int16_t Menu_GetHighLimit(void)
{
    return HighLimit;
}

/* 右上角共用一格：! 表示超温/低温，? 表示采样故障，空格表示正常或等待。 */
void Menu_ShowAlarmIndicator(void)
{
    char mark = ' ';

    if (AlarmState == ALARM_LOW || AlarmState == ALARM_HIGH)
    {
        mark = '!';
    }
    else if (AlarmState == ALARM_FAULT)
    {
        mark = '?';
    }

    /* 所有页面共用右上角标记。 */
    OLED_ShowChar(1, 16, mark);
}

void Menu_SetAlarmState(Alarm_State_t State)
{
    if (AlarmState == State)
    {
        return;
    }

    AlarmState = State;

    /* 状态变化时，立即更新温度页正文。 */
    if (CurrentPage == &TemperatureItem)
    {
        Menu_RefreshPage();
    }

    Menu_ShowAlarmIndicator();
}
