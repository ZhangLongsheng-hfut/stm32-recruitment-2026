/* OLED 驱动的实际实现文件：PB8/PB9 模拟 I2C，依次完成总线写入、
 * 光标定位、ASCII/汉字绘制。菜单显示内容在 System/Menu.c 中组织。
 */
#include "stm32f10x.h"
#include "OLED.h"
#include "OLED_Font.h"

/*引脚配置*/
#define OLED_W_SCL(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)(x))
#define OLED_W_SDA(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)(x))

/*引脚初始化*/
void OLED_I2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C开始
  * @param  无
  * @retval 无
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_W_SDA(0);
	OLED_W_SCL(0);
}

/**
  * @brief  I2C停止
  * @param  无
  * @retval 无
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C发送一个字节
  * @param  Byte 要发送的一个字节
  * @retval 无
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(!!(Byte & (0x80 >> i)));
		OLED_W_SCL(1);
		OLED_W_SCL(0);
	}
	OLED_W_SCL(1);	//额外的一个时钟，不处理应答信号
	OLED_W_SCL(0);
}

/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//从机地址
	OLED_I2C_SendByte(0x00);		//写命令
	OLED_I2C_SendByte(Command); 
	OLED_I2C_Stop();
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		//从机地址
	OLED_I2C_SendByte(0x40);		//写数据
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
/* 此处 Y 是 8 像素高的显存页号；上层一个 16 像素高的文字行占用两页。 */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for(i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{      	
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
	}
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
/* 本函数按 ASCII 字节逐字显示，调用者负责保证行内空间；中文使用 OLED_ShowChinese。 */
void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}



/*==================== 16x16 汉字显示支持 ====================*/

typedef struct
{
	uint8_t UTF8[3];			// UTF-8编码
	uint8_t GBK[2];			// GBK编码
	uint8_t Data[32];		// 16x16字模：前16字节上半页，后16字节下半页
} OLED_ChineseFont_t;

/*
 * 内置字符：你 好 温 度 设 置 菜 单 欢 迎 加 入 电 压 舵 机 角 ！
 * 支持目标文字：欢迎加入！、电压、舵机角度；其中“度”复用原有字模。
 * 每项保存 UTF-8（3字节）、GBK（2字节）和16x16点阵，两种编码共用字模。
 * 字模按列排列，每字节的bit0位于该页最上方；每个汉字占用两个8像素高的页。
 * 本表是有限字库，并非通用汉字库；其他汉字需要按相同格式补充。
 */
static const OLED_ChineseFont_t OLED_ChineseFont[] =
{
	/* 你 */
	{{0xE4, 0xBD, 0xA0}, {0xC4, 0xE3},
	 {0x80,0xC0,0x70,0xFC,0x07,0xC0,0x70,0x1F,0x10,0x10,0xF0,0x10,0x10,0x10,0xF0,0x00,
	  0x00,0x00,0x00,0xFF,0x00,0x10,0x1C,0x07,0x80,0x80,0xFF,0x00,0x01,0x07,0x38,0x00}},

	/* 好 */
	{{0xE5, 0xA5, 0xBD}, {0xBA, 0xC3},
	 {0x00,0x88,0xF8,0x0F,0x08,0xF8,0x18,0x41,0x41,0x41,0xF1,0xF9,0x4D,0x47,0x41,0x00,
	  0x00,0x43,0x23,0x1E,0x0E,0x0B,0x10,0x00,0x40,0x40,0x7F,0x3F,0x00,0x00,0x00,0x00}},

	/* 温 */
	{{0xE6, 0xB8, 0xA9}, {0xCE, 0xC2},
	 {0x00,0x20,0x42,0x46,0x04,0x00,0x7E,0x52,0x52,0x52,0x52,0x52,0x52,0x7E,0x00,0x00,
	  0x00,0x40,0x70,0x1C,0x44,0x7E,0x7E,0x42,0x7E,0x42,0x42,0x7E,0x42,0x7E,0x40,0x00}},

	/* 度 */
	{{0xE5, 0xBA, 0xA6}, {0xB6, 0xC8},
	 {0x00,0x00,0xFC,0x04,0x24,0x24,0xFC,0x24,0x27,0x24,0x24,0xFC,0x24,0x24,0x24,0x00,
	  0x00,0xF0,0x1F,0x00,0x84,0xC4,0x4D,0x55,0x65,0x25,0x65,0x55,0x4C,0xC4,0x80,0x00}},

	/* 设 */
	{{0xE8, 0xAE, 0xBE}, {0xC9, 0xE8},
	 {0x00,0x20,0x21,0xE3,0x06,0x00,0xA0,0xB0,0x8F,0x81,0x81,0x81,0x9F,0x90,0x10,0x00,
	  0x00,0x00,0x20,0x3F,0x10,0x08,0x60,0x23,0x26,0x1C,0x18,0x3C,0x26,0x61,0x40,0x00}},

	/* 置 */
	{{0xE7, 0xBD, 0xAE}, {0xD6, 0xC3},
	 {0x00,0x10,0x97,0x15,0x15,0xD5,0x57,0x75,0x5D,0x57,0x55,0x55,0xD5,0xD7,0x10,0x00,
	  0x00,0x00,0x7F,0x20,0x20,0x2F,0x2D,0x2D,0x2D,0x2D,0x2D,0x2D,0x2F,0x2F,0x20,0x00}},

	/* 菜 */
	{{0xE8, 0x8F, 0x9C}, {0xB2, 0xCB},
	 {0x00,0x02,0x22,0xA2,0x22,0x27,0x22,0xE2,0xB2,0x32,0x17,0x12,0xD2,0x72,0x02,0x00,
	  0x00,0x44,0x44,0x65,0x35,0x14,0x0C,0xFE,0xFE,0x0C,0x14,0x25,0x24,0x44,0x44,0x00}},

	/* 单 */
	{{0xE5, 0x8D, 0x95}, {0xB5, 0xA5},
	 {0x00,0x00,0xF8,0xF8,0x4B,0x4E,0x48,0xF8,0xF8,0x48,0x4C,0x4B,0x48,0xF8,0x00,0x00,
	  0x00,0x10,0x13,0x13,0x12,0x12,0x12,0xFF,0xFF,0x12,0x12,0x12,0x12,0x13,0x10,0x00}},

	/* 欢：16x16，前16字节为上半页，后16字节为下半页。 */
	{{0xE6,0xAC,0xA2}, {0xBB,0xB6},
	 {0x04,0x24,0x44,0x84,0x64,0x9C,0x40,0x30,0x0F,0xC8,0x08,0x08,0x28,0x18,0x00,0x00,
	  0x10,0x08,0x06,0x01,0x82,0x4C,0x20,0x18,0x06,0x01,0x06,0x18,0x20,0x40,0x80,0x00}},

	/* 迎：16x16，前16字节为上半页，后16字节为下半页。 */
	{{0xE8,0xBF,0x8E}, {0xD3,0xAD},
	 {0x40,0x40,0x42,0xCC,0x00,0x00,0xFC,0x04,0x02,0x00,0xFC,0x04,0x04,0xFC,0x00,0x00,
	  0x00,0x40,0x20,0x1F,0x20,0x40,0x4F,0x44,0x42,0x40,0x7F,0x42,0x44,0x43,0x40,0x00}},

	/* 加：16x16，前16字节为上半页，后16字节为下半页。 */
	{{0xE5,0x8A,0xA0}, {0xBC,0xD3},
	 {0x10,0x10,0x10,0xFF,0x10,0x10,0xF0,0x00,0x00,0xF8,0x08,0x08,0x08,0xF8,0x00,0x00,
	  0x80,0x40,0x30,0x0F,0x40,0x80,0x7F,0x00,0x00,0x7F,0x20,0x20,0x20,0x7F,0x00,0x00}},

	/* 入：16x16，前16字节为上半页，后16字节为下半页。 */
	{{0xE5,0x85,0xA5}, {0xC8,0xEB},
	 {0x00,0x00,0x00,0x00,0x00,0x01,0xE2,0x1C,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	  0x80,0x40,0x20,0x10,0x0C,0x03,0x00,0x00,0x00,0x03,0x0C,0x30,0x40,0x80,0x80,0x00}},

	/* 电：16x16，前16字节为上半页，后16字节为下半页。 */
	{{0xE7,0x94,0xB5}, {0xB5,0xE7},
	 {0x00,0x00,0xF8,0x88,0x88,0x88,0x88,0xFF,0x88,0x88,0x88,0x88,0xF8,0x00,0x00,0x00,
	  0x00,0x00,0x1F,0x08,0x08,0x08,0x08,0x7F,0x88,0x88,0x88,0x88,0x9F,0x80,0xF0,0x00}},

	/* 压：16x16，前16字节为上半页，后16字节为下半页。 */
	{{0xE5,0x8E,0x8B}, {0xD1,0xB9},
	 {0x00,0x00,0xFF,0x01,0x41,0x41,0x41,0x41,0xFD,0x41,0x41,0x41,0x41,0x41,0x01,0x00,
	  0x40,0x30,0x0F,0x20,0x20,0x20,0x20,0x20,0x3F,0x20,0x20,0x22,0x2C,0x20,0x20,0x00}},

	/* 舵：16x16，前16字节为上半页，后16字节为下半页。 */
	{{0xE8,0x88,0xB5}, {0xB6,0xE6},
	 {0x80,0x80,0xFC,0x96,0xE5,0x84,0xFC,0x00,0x38,0xC8,0x09,0x0E,0x08,0xA8,0x18,0x00,
	  0x80,0x60,0x1F,0x02,0x4C,0x80,0x7F,0x00,0x00,0x3F,0x44,0x42,0x41,0x40,0x78,0x00}},

	/* 机：16x16，前16字节为上半页，后16字节为下半页。 */
	{{0xE6,0x9C,0xBA}, {0xBB,0xFA},
	 {0x10,0x10,0xD0,0xFF,0x90,0x10,0x00,0xFE,0x02,0x02,0x02,0xFE,0x00,0x00,0x00,0x00,
	  0x04,0x03,0x00,0xFF,0x00,0x83,0x60,0x1F,0x00,0x00,0x00,0x3F,0x40,0x40,0x78,0x00}},

	/* 角：16x16，前16字节为上半页，后16字节为下半页。 */
	{{0xE8,0xA7,0x92}, {0xBD,0xC7},
	 {0x20,0x10,0xE8,0x24,0x27,0x24,0x24,0xE4,0x24,0x34,0x2C,0x20,0xE0,0x00,0x00,0x00,
	  0x80,0x60,0x1F,0x09,0x09,0x09,0x09,0x7F,0x09,0x09,0x49,0x89,0x7F,0x00,0x00,0x00}},

	/* ！：16x16，前16字节为上半页，后16字节为下半页。 */
	{{0xEF,0xBC,0x81}, {0xA3,0xA1},
	 {0x00,0x00,0x00,0xFE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	  0x00,0x00,0x00,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}
};

#define OLED_CHINESE_FONT_COUNT \
	(sizeof(OLED_ChineseFont) / sizeof(OLED_ChineseFont[0]))

/**
  * @brief  在指定像素X位置显示一个16x16汉字字模
  * @param  Line 行位置，范围：1~4
  * @param  X 横向像素坐标，范围：0~112
  * @param  Index 汉字字库下标
  * @retval 无
  */
static void OLED_ShowChineseIndex(uint8_t Line, uint8_t X, uint8_t Index)
{
	uint8_t i;

	OLED_SetCursor((Line - 1) * 2, X);
	for (i = 0; i < 16; i++)
	{
		OLED_WriteData(OLED_ChineseFont[Index].Data[i]);
	}

	OLED_SetCursor((Line - 1) * 2 + 1, X);
	for (i = 0; i < 16; i++)
	{
		OLED_WriteData(OLED_ChineseFont[Index].Data[i + 16]);
	}
}

/**
  * @brief  查找UTF-8汉字
  * @retval 找到返回字库下标，未找到返回0xFF
  */
static uint8_t OLED_FindChineseUTF8(const uint8_t *String)
{
	uint8_t i;

	for (i = 0; i < OLED_CHINESE_FONT_COUNT; i++)
	{
		if (String[0] == OLED_ChineseFont[i].UTF8[0] &&
			String[1] == OLED_ChineseFont[i].UTF8[1] &&
			String[2] == OLED_ChineseFont[i].UTF8[2])
		{
			return i;
		}
	}
	return 0xFF;
}

/**
  * @brief  查找GBK汉字
  * @retval 找到返回字库下标，未找到返回0xFF
  */
static uint8_t OLED_FindChineseGBK(const uint8_t *String)
{
	uint8_t i;

	for (i = 0; i < OLED_CHINESE_FONT_COUNT; i++)
	{
		if (String[0] == OLED_ChineseFont[i].GBK[0] &&
			String[1] == OLED_ChineseFont[i].GBK[1])
		{
			return i;
		}
	}
	return 0xFF;
}

/**
  * @brief  OLED显示中文字符串，兼容UTF-8和GBK编码
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始汉字列位置，范围：1~8
  * @param  String 字库中已有的中文，可混合ASCII，例如："欢迎加入！"、"电压"、"舵机角度"
  * @note   汉字/全角感叹号宽16像素，ASCII宽8像素；到达右边界后停止，不自动换行。
  * @note   Column按汉字列计数，1对应X=0，2对应X=16；未收录字符会被跳过。
  * @note   中文感叹号请使用全角“！”，半角“!”仍按ASCII字符显示。
  * @retval 无
  */
void OLED_ShowChinese(uint8_t Line, uint8_t Column, const char *String)
{
	const uint8_t *p;
	uint8_t X;
	uint8_t Index;

	/* 无效坐标或空指针直接返回，避免访问无效字符串。 */
	if (String == 0 || Line < 1 || Line > 4 || Column < 1 || Column > 8)
	{
		return;
	}

	p = (const uint8_t *)String;
	X = (Column - 1) * 16;

	while (*p != '\0' && X <= 127)
	{
		/* 先按UTF-8的3字节编码查找 */
		if (p[1] != '\0' && p[2] != '\0')
		{
			Index = OLED_FindChineseUTF8(p);
			if (Index != 0xFF)
			{
				if (X > 112) break;
				OLED_ShowChineseIndex(Line, X, Index);
				X += 16;
				p += 3;
				continue;
			}
		}

		/* 再按GBK的2字节编码查找 */
		if (p[1] != '\0')
		{
			Index = OLED_FindChineseGBK(p);
			if (Index != 0xFF)
			{
				if (X > 112) break;
				OLED_ShowChineseIndex(Line, X, Index);
				X += 16;
				p += 2;
				continue;
			}
		}

		/* ASCII字符也允许混合显示 */
		if (*p >= ' ' && *p <= '~')
		{
			if (X > 120) break;
			OLED_ShowChar(Line, X / 8 + 1, (char)*p);
			X += 8;
			p++;
			continue;
		}

		/* 字库里没有的字符：跳过一个可能的中文编码 */
		if ((*p & 0xF0) == 0xE0 && p[1] != '\0' && p[2] != '\0')
		{
			p += 3;
		}
		else if (p[1] != '\0')
		{
			p += 2;
		}
		else
		{
			p++;
		}
	}
}

/*==================== 16x16 汉字显示支持结束 ====================*/

/**
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
  * @brief  OLED初始化
  * @param  无
  * @retval 无
  */
void OLED_Init(void)
{
	uint32_t i, j;
	
	for (i = 0; i < 1000; i++)			//上电延时
	{
		for (j = 0; j < 1000; j++);
	}
	
	OLED_I2C_Init();			//端口初始化
	
	OLED_WriteCommand(0xAE);	//关闭显示
	
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示
		
	OLED_Clear();				//OLED清屏
}
