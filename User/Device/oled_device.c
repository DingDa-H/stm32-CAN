/**
  * @file       oled_device.c
  * @author     
  * @version    V1.0.1
  * @date       20260512
  * @brief      OLED 字库
  *
  */ 

#include "oled_device.h"
#include "string.h"
#include "math.h"
#include "oled_font.h"

stOledDeviceParamTdf astOledDeviceParam[OLED_DEV_NUM];

/// @brief      获取 OLED 设备参数
///
/// @param      emDevNum   ：设备号
///
/// @note       注意，返回值是 stOledDeviceParamTdf 型的指针，且指针指向的内容是不可更改的（只读的）
const stOledDeviceParamTdf *c_pstGetOledDeviceParam(emOledDevNumTdf emDevNum)
{
    return &astOledDeviceParam[emDevNum];
}

/// @brief      拷贝运行参数
///
/// @param      emDevNum   ：设备号
///
/// @note       
void vOledDeviceRunningParamInit(stOledRunningParamTdf *pstInit, emOledDevNumTdf emDevNum)
{
    memcpy(&astOledDeviceParam[emDevNum].stRunningParam, pstInit, sizeof(stOledRunningParamTdf) / sizeof(uint8_t));
}


/// @brief      SDA 引脚置高
///
/// @param      emDevNum        ：设备号
///
/// @note
static void s_vOledSdaSet(emOledDevNumTdf emDevNum)
{
	//通过直接对寄存器操作优化程序速度
//	HAL_GPIO_WritePin(astOledDeviceParam[emDevNum].stStaticParam.pstSdaGpioBase
//					  ,astOledDeviceParam[emDevNum].stStaticParam.usSdaGpioPin
//					  ,GPIO_PIN_SET);
	astOledDeviceParam[emDevNum].stStaticParam.pstSdaGpioBase->BSRR = astOledDeviceParam[emDevNum].stStaticParam.usSdaGpioPin;
	__NOP();
}

/// @brief      SDA 引脚置低
///
/// @param      emDevNum        ：设备号
///
/// @note
static void s_vOledSdaReset(emOledDevNumTdf emDevNum)
{
//	HAL_GPIO_WritePin(astOledDeviceParam[emDevNum].stStaticParam.pstSdaGpioBase
//					  ,astOledDeviceParam[emDevNum].stStaticParam.usSdaGpioPin
//					  ,GPIO_PIN_RESET);
	astOledDeviceParam[emDevNum].stStaticParam.pstSdaGpioBase->BSRR = (uint32_t)astOledDeviceParam[emDevNum].stStaticParam.usSdaGpioPin<<16;
	__NOP();
}

/// @brief      SCL 引脚置高
///
/// @param      emDevNum        ：设备号
///
/// @note
static void s_vOledSclSet(emOledDevNumTdf emDevNum)
{
//    HAL_GPIO_WritePin(astOledDeviceParam[emDevNum].stStaticParam.pstSclGpioBase,
//                      astOledDeviceParam[emDevNum].stStaticParam.usSclGpioPin,
//                      GPIO_PIN_SET);
	astOledDeviceParam[emDevNum].stStaticParam.pstSclGpioBase->BSRR = (uint32_t)astOledDeviceParam[emDevNum].stStaticParam.usSclGpioPin;
	__NOP();
}

/// @brief      SCL 引脚置低
///
/// @param      emDevNum        ：设备号
///
/// @note
static void s_vOledSclReset(emOledDevNumTdf emDevNum)
{
//    HAL_GPIO_WritePin(astOledDeviceParam[emDevNum].stStaticParam.pstSclGpioBase,
//                      astOledDeviceParam[emDevNum].stStaticParam.usSclGpioPin,
//                      GPIO_PIN_RESET);
	astOledDeviceParam[emDevNum].stStaticParam.pstSclGpioBase->BSRR = (uint32_t)astOledDeviceParam[emDevNum].stStaticParam.usSclGpioPin<<16;
	__NOP();
}


/// @brief      IIC 起始
///
/// @param      emDevNum        ：设备号
///
/// @note
static void s_vOledIicStart(emOledDevNumTdf emDevNum)
{
	s_vOledSdaSet(emDevNum);
	s_vOledSclSet(emDevNum);
	
	s_vOledSdaReset(emDevNum);
	s_vOledSclReset(emDevNum);
}

/// @brief      IIC 停止
///
/// @param      emDevNum        ：设备号
///
/// @note
static void s_vOledIicStop(emOledDevNumTdf emDevNum)
{
	s_vOledSclSet(emDevNum);
	s_vOledSdaReset(emDevNum);

	s_vOledSdaSet(emDevNum);
}

/// @brief      IIC 等待应答
///
/// @param      emDevNum        ：设备号
///
/// @note
static void s_vOledIicWaitAck(emOledDevNumTdf emDevNum)
{
	s_vOledSclSet(emDevNum);
	s_vOledSclReset(emDevNum);
}

/// @brief      IIC 发送一个字节
///
/// @param      emDevNum        ：设备号
///
/// @note
static void s_vOledIicSendByte(uint8_t ucData, emOledDevNumTdf emDevNum)
{
	for (int8_t i = 7; i >= 0; i--)
	{
		s_vOledSclReset(emDevNum);
		if(((ucData >> i) & 0x01) == 0)
		{
			s_vOledSdaReset(emDevNum);
		}else
		{
			s_vOledSdaSet(emDevNum);
		}
		s_vOledSclSet(emDevNum);
		s_vOledSclReset(emDevNum);
	}
}

/// @brief      向 OLED 写一个字节命令(0x00)
///
/// @param      ucCmd       ：要写入的命令
///             emDevNum    ：设备号
///
/// @note
static void s_vOledWriteOneByteCmd(uint8_t ucCmd, emOledDevNumTdf emDevNum)
{
	s_vOledIicStart(emDevNum);
	s_vOledIicSendByte(0x78,emDevNum);
	s_vOledIicWaitAck(emDevNum);
	s_vOledIicSendByte(0x00,emDevNum);
	s_vOledIicWaitAck(emDevNum);
	s_vOledIicSendByte(ucCmd,emDevNum);
	s_vOledIicWaitAck(emDevNum);
	s_vOledIicStop(emDevNum);
}


/// @brief      向 OLED 写多个字节数据(0x40)
///
/// @param      ucCmd       ：要写入的命令
///             ulLength    ：数据长度
///             emDevNum    ：设备号
///
/// @note
static void s_vOledWriteFewBytesData(uint8_t *pucData, uint32_t ulLength, emOledDevNumTdf emDevNum)
{
	s_vOledIicStart(emDevNum);
	s_vOledIicSendByte(0x78,emDevNum);
	s_vOledIicWaitAck(emDevNum);
	s_vOledIicSendByte(0x40,emDevNum);
	s_vOledIicWaitAck(emDevNum);
	for(uint8_t i = 0;i < ulLength;i++)
	{
	
		s_vOledIicSendByte(pucData[i],emDevNum);
		s_vOledIicWaitAck(emDevNum);
	}
	s_vOledIicStop(emDevNum);
}

/// @brief      向缓冲区指定位置画 1 个点
///
/// @param      x           ：X 坐标，取值范围 0 ~ OLED_POINT_WIDTH - 1
///             y           ：Y 坐标，取值范围 0 ~ OLED_POINT_HIGH - 1
///             emDevNum    ：设备号
///
/// @note
void vOledDrawOnePointToBuffer(uint32_t x, uint32_t y, emOledDevNumTdf emDevNum)
{
	uint16_t ii, jj, kk;
	
	//计算点属于哪一个page
	ii = y >> 3;	//等价 i=y/8 >>速度比较快
	
	//任何整数 y 除以 8，余数就是它二进制表示中低 3 位的值，
	//因为高于第 3 位的部分都一定是 8 的整数倍。
	
	//计算点属于seg的哪一位,以字节为的单位具体的位
	jj = y & 0x07;	//等价 j =y%8
	
	//计算对应的 seg掩码（具体的y坐标）
	kk = 0x01 << jj;
	
	//对应位置为1
	astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[x + ii * OLED_BUFFER_WIDTH] |= kk;
}

/// @brief      向缓冲区指定位置清除 1 个点
///
/// @param      x           ：X 坐标，取值范围 0 ~ OLED_POINT_WIDTH - 1
///             y           ：Y 坐标，取值范围 0 ~ OLED_POINT_HIGH - 1
///             emDevNum    ：设备号
///
/// @note
void vOledClearOnePointToBuffer(uint32_t x, uint32_t y, emOledDevNumTdf emDevNum)
{
	uint16_t ii, jj, kk;
	
	//计算点属于哪一个page
	ii = y >> 3;	//等价 i=y/8 >>速度比较快
	
	//任何整数 y 除以 8，余数就是它二进制表示中低 3 位的值，
	//因为高于第 3 位的部分都一定是 8 的整数倍。
	
	//计算点属于seg的哪一位,以字节为的单位具体的位
	jj = y & 0x07;	//等价 j =y%8
	
	//计算对应的 seg掩码（具体的y坐标）
	kk = 0x01 << jj;
	
	//对应位置为1
	astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[x + ii * OLED_BUFFER_WIDTH] &= ~kk;

}

/// @brief      向缓存写入 1 个字符
///
/// @param      x           ：X 坐标，取值范围 0 ~ OLED_POINT_WIDTH - 1
///             y           ：Y 坐标，取值范围 0 ~ OLED_POINT_HEIGHT - 1
///             ucChar      ：要显示的字符
///             emFontSize  ：字号，见 emOledFontSizeTdf 定义
///             emMode      ：显示模式，见 emOledPixelShowModeTdf 定义
///             emDevNum    ：设备号
///
/// @note
void vOledWriteOneCharToBuffer(uint32_t x, uint32_t y, uint8_t ucChar, emOledFontSizeTdf emFontSize, emOledPixelShowModeTdf emMode, emOledDevNumTdf emDevNum)
{
	uint16_t i,j;
	uint16_t y0;
	uint32_t ulSizeTemp,ulCharIndex;
	uint8_t ucData;
	const uint8_t *p;
	
	//1.计算显示1个字符，需要写多少byte
	//ulSizeTemp = ((emFontSize >> 3) + ((emFontSize & 0x07) ? 1 : 0))高度方向总共需要的字节数
	
	//(emFontSize >> 1)字符的宽度
	
	//两者相乘表示一个字符总共占用的字节数
	
	//ulSizeTemp = (((emFontSize / 8) + (emFontSize & 8) ? 1 : 0))*(emFontSize /2);	//除法和求模太慢了，用位移和逻辑运算代替
	ulSizeTemp = ((emFontSize >> 3) + ((emFontSize & 0x07) ? 1 : 0))*(emFontSize >> 1);
	
	//2.计算要显示字符，在字模数组中的索引
	ulCharIndex = ucChar - ' ';
	
	//3.初始化 y0
	y0 = y;
	
	//4将字符写入缓存
	//4.1选择字模数组
	
	if(emFontSize == emOledFontSize_6x12)
	{
		p = (const uint8_t *)c_a2ucOledFont_Ascii_1206;
		//等价p = &c_a2ucOledFont_Ascii_1206[n][0];
	}
	else if(emFontSize == emOledFontSize_8x16)
	{
		p = (const uint8_t *)c_a2ucOledFont_Ascii_1608;
	}
	else if(emFontSize == emOledFontSize_12x24)
	{
		p = (const uint8_t *)c_a2ucOledFont_Ascii_2412;
	}
	else
	{
		return;
	}
	
	//4.2 按字模将数据写入显存
	
	//ulSizeTemp:字符总共占用的字节数,1字节8位
	
	//假设是12X6阵列，一个字符占12字节（可以算出来）
	//12个字节已经自动排好了，我们只需要按规则将12字节写入缓存就行
	//要理解阵列
	for(i = 0;i < ulSizeTemp;i++)
	{
		//4.2.1 更新要写的字符
		
		//ulCharIndex * ulSizeTemp：当前字符的起始字节在数组中的偏移。
		//例如索引为3的字符，每个占12字节，则起始位置为3*12=36。
		ucData = p[ulCharIndex * ulSizeTemp + i];
		
		//4.2.2 写入缓存
		
		//处理一个字节的 8 个位，
		for(j = 0;j < 8;j++)
        {
			if((ucData & 0x80) != 0)
			{
				if(emMode == emOledPixelShowMode_Positive)
				{
//					vOledDrawOnePointToBuffer(x,y,emDevNum);
					//通过减少函数调用优化速度，下面的代码是vOledDrawOnePointToBuffer里面的内容
					uint16_t ii, jj, kk;
	
					//计算点属于哪一个page
					ii = y >> 3;	//等价 i=y/8 >>速度比较快
					
					//任何整数 y 除以 8，余数就是它二进制表示中低 3 位的值，
					//因为高于第 3 位的部分都一定是 8 的整数倍。
					
					//计算点属于seg的哪一位,以字节为的单位具体的位
					jj = y & 0x07;	//等价 j =y%8
					
					//计算对应的 seg掩码（具体的y坐标）
					kk = 0x01 << jj;
					
					//对应位置为1
					astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[x + ii * OLED_BUFFER_WIDTH] |= kk;

				}
				else
				{
//					vOledClearOnePointToBuffer(x,y,emDevNum);
					uint16_t ii, jj, kk;
	
					//计算点属于哪一个page
					ii = y >> 3;	//等价 i=y/8 >>速度比较快
					
					//任何整数 y 除以 8，余数就是它二进制表示中低 3 位的值，
					//因为高于第 3 位的部分都一定是 8 的整数倍。
					
					//计算点属于seg的哪一位,以字节为的单位具体的位
					jj = y & 0x07;	//等价 j =y%8
					
					//计算对应的 seg掩码（具体的y坐标）
					kk = 0x01 << jj;
					
					//对应位置为1
					astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[x + ii * OLED_BUFFER_WIDTH] &= ~kk;

				}
			}
			else
			{
				if(emMode == emOledPixelShowMode_Positive)
				{
//					vOledClearOnePointToBuffer(x,y,emDevNum);
					uint16_t ii, jj, kk;
	
					//计算点属于哪一个page
					ii = y >> 3;	//等价 i=y/8 >>速度比较快
					
					//任何整数 y 除以 8，余数就是它二进制表示中低 3 位的值，
					//因为高于第 3 位的部分都一定是 8 的整数倍。
					
					//计算点属于seg的哪一位,以字节为的单位具体的位
					jj = y & 0x07;	//等价 j =y%8
					
					//计算对应的 seg掩码（具体的y坐标）
					kk = 0x01 << jj;
					
					//对应位置为1
					astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[x + ii * OLED_BUFFER_WIDTH] &= ~kk;

				}
				else
				{
//					vOledDrawOnePointToBuffer(x,y,emDevNum);
					uint16_t ii, jj, kk;
	
					//计算点属于哪一个page
					ii = y >> 3;	//等价 i=y/8 >>速度比较快
					
					//任何整数 y 除以 8，余数就是它二进制表示中低 3 位的值，
					//因为高于第 3 位的部分都一定是 8 的整数倍。
					
					//计算点属于seg的哪一位,以字节为的单位具体的位
					jj = y & 0x07;	//等价 j =y%8
					
					//计算对应的 seg掩码（具体的y坐标）
					kk = 0x01 << jj;
					
					//对应位置为1
					astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[x + ii * OLED_BUFFER_WIDTH] |= kk;

				}
			}
			
			ucData = ucData << 1;	
			
			y++;					//写完一位，下移
			if((y-y0) == (uint16_t)emFontSize)		//emFontSize  ：字号-字体高度
			{
				y = y0;				//重置y回到原来的高度
				x++;				//写完一个列| 。右移（这里的列指的是字符的一整列）
				break;
			}
        }
	}
}


/// @brief      向缓存写入字符串
///
/// @param      x           ：X 坐标，取值范围 0 ~ OLED_POINT_WIDTH - 1
///             y           ：Y 坐标，取值范围 0 ~ OLED_POINT_HEIGHT - 1	
///             pucChar     ：要显示的字符串首地址指针
///             emFontSize  ：字号，见 emOledFontSizeTdf 定义
///             emMode      : 显示模式，见 emOledPixelShowModeTdf 定义
///             emDevNum    ：设备号
///
/// @note
void vOledWriteStringToBuffer(uint16_t x, uint16_t y, const uint8_t *c_pucChar, emOledFontSizeTdf emFontSize, emOledPixelShowModeTdf emMode, emOledDevNumTdf emDevNum)
{
	while ((*c_pucChar) >=' '&&(*c_pucChar) <='~')		//判断字符是否合法
    {
		vOledWriteOneCharToBuffer(x,y,*c_pucChar,emFontSize,emMode,emDevNum);
		
		x += emFontSize>>1;								//emFontSize字符高度，这里设置的字符矩阵都是 宽度=高度/2
		
		if(x > (OLED_POINT_WIDTH - emFontSize))			//防止字符超出x轴方向的屏幕
		{
			x = 0;
			y += emFontSize;
		}
		
		c_pucChar = c_pucChar+1;
    }
}


/// @brief      将一个虚线矩形框写入缓存
///
/// @param      x           ：X 坐标，取值范围 0 ~ OLED_POINT_WIDTH - 1,需要框选字体的x
///             y           ：Y 坐标，取值范围 0 ~ OLED_POINT_HEIGHT - 1,需要框选字体的y
///             ulLenght    ：需要框选字体的矩形框的长度
///             emFontSize  ：需要框选字体的字号，矩形框的高度
///             emDevNum    ：设备号
///
/// @note		实现用矩形框框住按钮
void vOledDrawRectangle(uint32_t x, uint32_t y,uint32_t ulLenght,emOledFontSizeTdf emFontSize,emOledDevNumTdf emDevNum)
{
	uint32_t i,j;
	uint32_t x0 = x - 1,y0 = y - 1;
	uint32_t ulLenght0 = ulLenght + 2;
	//画横线
	for(i = 0;i < ulLenght0;i++)
    {
		if((i & 0x01) == 0)//判断奇偶
		{
			vOledDrawOnePointToBuffer(x0 + i,y0,emDevNum);
		}
		else
		{
			vOledClearOnePointToBuffer(x0 + i,y0,emDevNum);
		}
    }
	//画横线
	for(i = 0;i < ulLenght0;i++)
    {
		if((i & 0x01) == 0)//判断奇偶
		{
			vOledDrawOnePointToBuffer(x0 + i,y0 + emFontSize + 1,emDevNum);
		}
		else
		{
			vOledClearOnePointToBuffer(x0 + i,y0 + emFontSize + 1,emDevNum);
		}
    }
	//画竖线
	for(j = 0;j < emFontSize + 2;j++)
    {
		if((j & 0x01) == 0)//判断奇偶
		{
			vOledClearOnePointToBuffer(x0,y0 + j,emDevNum);
		}
		else
		{
			
			vOledDrawOnePointToBuffer(x0,y0 + j,emDevNum);
		}
    }
	//画竖线
	for(j = 0;j < emFontSize + 2;j++)
    {
		if((j & 0x01) == 0)//判断奇偶
		{
			
			vOledClearOnePointToBuffer(x0 + ulLenght0 - 1,y0 + j,emDevNum);
		}
		else
		{
			vOledDrawOnePointToBuffer(x0 + ulLenght0 - 1,y0 + j,emDevNum);
		}
    }
}

/// @brief      为被选中的按钮添加选中箭头的函数
///
/// @param      x           ：X 坐标，取值范围 0 ~ OLED_POINT_WIDTH - 1,需要指向字体的x
///             y           ：Y 坐标，取值范围 0 ~ OLED_POINT_HEIGHT - 1,需要指向字体的y
///             ulLenght    ：需要指向字体的长度
///             emFontSize  ：需要指向字体的字号，的高度
///             emDevNum    ：设备号
///
/// @note		实现用箭头指向被选中按钮
void vOledDrawArrow(uint32_t x, uint32_t y,emOledFontSizeTdf emFontSize,emOledDevNumTdf emDevNum)
{
//	uint32_t i;
	char ucdata[] = "->";
	
	vOledWriteStringToBuffer(x - strlen(ucdata) * emFontSize / 2 - 1,y,(uint8_t *)&ucdata,emFontSize,emOledPixelShowMode_Positive,emDevNum);
}


/// @brief      清屏函数
///
/// @param      emDevNum        ：设备号
///
/// @note
void vOledClearBuffer(emOledDevNumTdf emDevNum)
{
	memset(astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer, 0,
           OLED_BUFFER_WIDTH * OLED_BUFFER_HIGH);
}

/// @brief      向缓存写入十六进制数据
///
/// @param      x           ：X 坐标，取值范围 0 ~ OLED_POINT_WIDTH - 1
///             y           ：Y 坐标，取值范围 0 ~ OLED_POINT_HEIGHT - 1
///             pucData     ：要显示的字节数组首地址
///             ucLength    ：字节数组长度
///             emFontSize  ：字号，见 emOledFontSizeTdf 定义
///             emMode      ：显示模式，见 emOledPixelShowModeTdf 定义
///             emDevNum    ：设备号
///
/// @note       每个字节显示为 2 位十六进制字符，字节之间用空格分隔
void vOledWriteHexToBuffer(uint16_t x, uint16_t y, const uint8_t *pucData, uint8_t ucLength, emOledFontSizeTdf emFontSize, emOledPixelShowModeTdf emMode, emOledDevNumTdf emDevNum)
{
	uint8_t i;
	uint8_t ucNibble;
	uint8_t ucChar;
	uint16_t usX = x;

	for(i = 0; i < ucLength; i++)
	{
		/* 高 4 位 */
		ucNibble = (pucData[i] >> 4) & 0x0F;
		ucChar = (ucNibble < 10) ? ('0' + ucNibble) : ('A' + ucNibble - 10);
		vOledWriteOneCharToBuffer(usX, y, ucChar, emFontSize, emMode, emDevNum);
		usX += emFontSize >> 1;

		/* 低 4 位 */
		ucNibble = pucData[i] & 0x0F;
		ucChar = (ucNibble < 10) ? ('0' + ucNibble) : ('A' + ucNibble - 10);
		vOledWriteOneCharToBuffer(usX, y, ucChar, emFontSize, emMode, emDevNum);
		usX += emFontSize >> 1;

		/* 空格分隔（最后一个字节后不加） */
		if(i < ucLength - 1)
		{
			vOledWriteOneCharToBuffer(usX, y, ' ', emFontSize, emMode, emDevNum);
			usX += emFontSize >> 1;
		}
	}
}


/// @brief      更新显存显示
///
/// @param      emDevNum        ：设备号
///
/// @note
void vOledRefreshFromBuffer(emOledDevNumTdf emDevNum)
{
	uint8_t i;
//	astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[0] = 0xFF;
//	astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[10] = 0xFF;
//	astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[20] = 0xFF;

	for(i = 0;i < OLED_BUFFER_HIGH;i++)
	{
		s_vOledWriteOneByteCmd(0xB0 + i,emDevNum);	//写入设置当前列地址命令
		s_vOledWriteOneByteCmd(0x00,emDevNum);		//写入设置当前行地址命令
		s_vOledWriteOneByteCmd(0x10,emDevNum);		//写入设置当前行地址命令

		s_vOledWriteFewBytesData(&(astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[i * OLED_BUFFER_WIDTH]),
								  OLED_BUFFER_WIDTH,
								  emDevNum);		//写入多个数据
	}
}

/// @brief      OLED 命令初始化
///
/// @param      emDevNum    ：设备号
///
/// @note
void vOledCmdInit(emOledDevNumTdf emDevNum)
{
	s_vOledWriteOneByteCmd(0xAE, emDevNum);//--turn off oled panel
	s_vOledWriteOneByteCmd(0x00, emDevNum);//---set low column address
	s_vOledWriteOneByteCmd(0x10, emDevNum);//---set high column address
	s_vOledWriteOneByteCmd(0x40, emDevNum);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	s_vOledWriteOneByteCmd(0x81, emDevNum);//--set contrast control register
	s_vOledWriteOneByteCmd(0xCF, emDevNum);// Set SEG Output Current Brightness
	s_vOledWriteOneByteCmd(0xA1, emDevNum);//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	s_vOledWriteOneByteCmd(0xC8, emDevNum);//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	s_vOledWriteOneByteCmd(0xA6, emDevNum);//--set normal display
	s_vOledWriteOneByteCmd(0xA8, emDevNum);//--set multiplex ratio(1 to 64)
	s_vOledWriteOneByteCmd(0x3f, emDevNum);//--1/64 duty
	s_vOledWriteOneByteCmd(0xD3, emDevNum);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	s_vOledWriteOneByteCmd(0x00, emDevNum);//-not offset
	s_vOledWriteOneByteCmd(0xd5, emDevNum);//--set display clock divide ratio/oscillator frequency
	s_vOledWriteOneByteCmd(0x80, emDevNum);//--set divide ratio, Set Clock as 100 Frames/Sec
	s_vOledWriteOneByteCmd(0xD9, emDevNum);//--set pre-charge period
	s_vOledWriteOneByteCmd(0xF1, emDevNum);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	s_vOledWriteOneByteCmd(0xDA, emDevNum);//--set com pins hardware configuration
	s_vOledWriteOneByteCmd(0x12, emDevNum);
	s_vOledWriteOneByteCmd(0xDB, emDevNum);//--set vcomh
	s_vOledWriteOneByteCmd(0x40, emDevNum);//Set VCOM Deselect Level
	s_vOledWriteOneByteCmd(0x20, emDevNum);//-Set Page Addressing Mode (0x00/0x01/0x02)
	s_vOledWriteOneByteCmd(0x02, emDevNum);//
	s_vOledWriteOneByteCmd(0x8D, emDevNum);//--set Charge Pump enable/disable
	s_vOledWriteOneByteCmd(0x14, emDevNum);//--set(0x10) disable
	s_vOledWriteOneByteCmd(0xA4, emDevNum);// Disable Entire Display On (0xa4/0xa5)
	s_vOledWriteOneByteCmd(0xA6, emDevNum);// Disable Inverse Display On (0xa6/a7) 
	s_vOledWriteOneByteCmd(0xAF, emDevNum);
	
}



/// @brief      OLED 设备初始化
///
/// @param      pstInit     ：初始化参数结构体的首地址
/// @param      emDevNum    ：设备编号
///
/// @note
void vOledDeviceInit(stOledStaticParamTdf *pstInit, emOledDevNumTdf emDevNum)
{
    // 1. 初始化静态参数
    memcpy(&astOledDeviceParam[emDevNum].stStaticParam, pstInit, sizeof(stOledStaticParamTdf) / sizeof(uint8_t));

    // 2. 初始化寄存器
    vOledCmdInit(emDevNum);
}

