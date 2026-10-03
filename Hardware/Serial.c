#include "Serial.h"
#include <string.h>

char Serial_RxPacket[100];          // 保存接收到的字符串
volatile uint8_t Serial_RxFlag = 0; // 中断置1，主循环处理后清零


/**
  * 函    数：串口初始化
  * 参    数：无
  * 返 回 值：无
  */
void Serial_Init(void)
{
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);	//开启USART1的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//开启GPIOA的时钟
	
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA9引脚初始化为复用推挽输出
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//将PA10引脚初始化为上拉输入
	
	/*USART初始化*/
	USART_InitTypeDef USART_InitStructure;					//定义结构体变量
	USART_InitStructure.USART_BaudRate = 9600;				//波特率
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制，不需要
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;	//模式，发送模式和接收模式均选择
	USART_InitStructure.USART_Parity = USART_Parity_No;		//奇偶校验，不需要
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//停止位，选择1位
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//字长，选择8位
	USART_Init(USART1, &USART_InitStructure);				//将结构体变量交给USART_Init，配置USART1
	
	/*中断输出配置*/
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);			//开启串口接收数据的中断
	
	/*NVIC中断分组*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);			//配置NVIC为分组2
	
	/*NVIC配置*/
	NVIC_InitTypeDef NVIC_InitStructure;					//定义结构体变量
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;		//选择配置NVIC的USART1线
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//指定NVIC线路使能
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;		//指定NVIC线路的抢占优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//指定NVIC线路的响应优先级为1
	NVIC_Init(&NVIC_InitStructure);							//将结构体变量交给NVIC_Init，配置NVIC外设
	
	/*USART使能*/
	USART_Cmd(USART1, ENABLE);								//使能USART1，串口开始运行
}

/**
  * 函    数：串口发送一个字节
  * 参    数：Byte 要发送的一个字节
  * 返 回 值：无
  */
void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1, Byte);		//将字节数据写入数据寄存器，写入后USART自动生成时序波形
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);	//等待发送完成
	/*下次写入数据寄存器会自动清除发送完成标志位，故此循环后，无需清除标志位*/
}


/**
  * 函    数：串口发送一个字符串
  * 参    数：String 要发送字符串的首地址
  * 返 回 值：无
  */
void Serial_SendString(char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i ++)//遍历字符数组（字符串），遇到字符串结束标志位后停止
	{
		Serial_SendByte(String[i]);		//依次调用Serial_SendByte发送每个字节数据
	}
}

/**
 * 串口接收中断：
 * 按行接收，支持LF和CRLF结尾，最多保存99个字符。
 * 中断只接收数据，命令解析和舵机控制由主循环完成。
 */
void USART1_IRQHandler(void)
{
    static uint8_t RxLength = 0;     /* 当前已接收的字符数 */
    static uint8_t RxBadLine = 0;    /* 当前这一行是否错误 */
    uint8_t RxData;

    if (USART_GetITStatus(USART1, USART_IT_RXNE) == RESET)
    {
        return;
    }

    /* 读取收到的字节，同时清除RXNE标志 */
    RxData = (uint8_t)USART_ReceiveData(USART1);

    /* 上一条命令尚未处理，先保持缓冲区内容 */
    if (Serial_RxFlag == 1)
    {
        return;
    }

    /* 收到换行，表示这一条命令接收完成 */
    if (RxData == '\n')
    {
        if (RxBadLine == 1)
        {
            /* 错误命令置为空字符串，交给主循环报错 */
            Serial_RxPacket[0] = '\0';
        }
        else
        {
            /* 如果结尾是CRLF，去掉最后的CR */
            if (RxLength > 0 &&
                Serial_RxPacket[RxLength - 1] == '\r')
            {
                RxLength--;
            }

            /* 补上字符串结束符 */
            Serial_RxPacket[RxLength] = '\0';
        }

        /* 为下一条命令准备 */
        RxLength = 0;
        RxBadLine = 0;

        /* 通知主循环：收到了一条完整命令 */
        Serial_RxFlag = 1;
    }
    else if (RxBadLine == 0)
    {
        /*
         * 保留一个位置放字符串结束符。
         * 超长或含零字节时，丢弃本行剩余内容。
         */
        if (RxData == '\0' ||
            RxLength >= sizeof(Serial_RxPacket) - 1)
        {
            RxBadLine = 1;
        }
        else
        {
            Serial_RxPacket[RxLength] = (char)RxData;
            RxLength++;
        }
    }
}

/**
 * 函数：解析ANGLE命令，例如ANGLE=90
 * 返回：0～180表示合法目标角度；-1表示命令错误
 */
int16_t Serial_ParseAngle(const char *Command)
{
    uint8_t i;
    uint16_t Angle = 0;

    /* 必须以ANGLE=开头，且后面有数字 */
    if (strncmp(Command, "ANGLE=", 6) != 0 ||
        Command[6] == '\0')
    {
        return -1;
    }

    /* 从第7个字符开始，将数字字符串转换成角度 */
    for (i = 6; Command[i] != '\0'; i++)
    {
        /* 角度部分只能包含数字 */
        if (Command[i] < '0' || Command[i] > '9')
        {
            return -1;
        }

        /*
         * 示例：解析120
         * 先得到1，再得到12，最后得到120。
         */
        Angle = Angle * 10 + (Command[i] - '0');

        /* 每读入一位就检查范围，避免数值不断增大 */
        if (Angle > 180)
        {
            return -1;
        }
    }

    return (int16_t)Angle;
}