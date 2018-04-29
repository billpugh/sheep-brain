/************************************************************************************
*  Copyright (c), 2015, HelTec Automatic Technology co.,LTD.
*            All rights reserved.
*
* Http:    www.heltec.cn
* Email:   heltec@heltec.cn
* WebShop: heltec.taobao.com
*
* File name: OLED.c
* Project  : OLED
* Processor: STC89C52
* Compiler : Keil C51 Compiler
* 
* Author : Aaron Lee
* Version: 1.01
* Date   : 2016.5.25
* Email  : support@heltec.com
* Modification: none
* 
* Description:128*64����OLEDģ�������ļ���������heltec.taobao.com���۲�Ʒ
*
* Others: none;
*
* Function List:
*
* 1. void OLED_WrDat(unsigned char dat) -- ��OLED��д����
* 2. void OLED_WrCmd(unsigned char cmd) -- ��OLED��д����
* 3. void OLED_SetPos(unsigned char x, unsigned char y) -- ������ʾ����
* 4. void OLED_Fill(unsigned char bmp_dat) -- ȫ����ʾ(��ʾBMPͼƬʱ�Ż��õ��˹���)
* 5. void OLED_CLS(void) -- ��λ/����
* 6. void OLED_Init(void) -- OLED����ʼ�����򣬴˺���Ӧ�ڲ�����Ļ֮ǰ���ȵ���
* 7. void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t) -- ����
* 8. void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize) -- 8x16������������ʾASCII�룬�ǳ�����
* 9.void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N) -- 16x16������������ʾ���ֵ���С����
* 10.void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]) -- ��128x64���ص�BMPλͼ��ȡ�����������ֱ�Ȼ���Ƶ�codetab�У��˺������ü���
*
* History: ��IIC��ʽ��SPI��ʽ����ͨ�ŷ�ʽ���ϵ���ͬһ���ļ���;
*          ��SH1106��SSD1306�ĳ�ʼ���������ϵ���ͬһ���ļ���;
*          ʹ��֮ǰ����ȷ���Լ�����ͨ�ŷ�ʽ���ͺţ��ٵ�OLED.h�ļ����޸ĺ궨��
*
*************************************************************************************/

#include "OLED.h"
#include "Delay.h"
#include "codetab.h"

#if IIC_SPI_TYPE //SPI���Ŷ���
	sbit OLED_SCL=P1^3; //ʱ��CLK
	sbit OLED_SDA=P1^2; //MISO
	sbit OLED_RST=P1^1; //��λ
	sbit OLED_DC =P1^0; //����/�������
#else //IIC���Ŷ���
	sbit SCL=P1^3; //SCL
	sbit SDA=P1^2; //SDA
#endif

#if !IIC_SPI_TYPE
void IIC_Start(void)
{
   SCL = 1;		
   SDA = 1;
   SDA = 0;
   SCL = 0;
}

/**********************************************
//IIC Stop
**********************************************/
void IIC_Stop(void)
{
   SCL = 0;
   SDA = 0;
   SCL = 1;
   SDA = 1;
}

/**********************************************
// IIC Write byte
**********************************************/
void Write_IIC_Byte(unsigned char IIC_Byte)
{
	unsigned char i;
	for(i=0;i<8;i++)
	{
		if(IIC_Byte & 0x80)
			SDA = 1;
		else
			SDA = 0;
		SCL = 1;
		SCL = 0;
		IIC_Byte<<=1;
	}
	SDA = 1;
	SCL = 1;
	SCL = 0;
}
#endif

/*********************OLEDд����************************************/ 
void OLED_WrDat(unsigned char dat)
{
#if IIC_SPI_TYPE //SPI��ʽд����
	
	unsigned char i;
	OLED_DC=1;  
	for(i=0;i<8;i++) //����һ����λ���� 
	{
		if((dat << i) & 0x80)
		{
			OLED_SDA  = 1;
		}
		else  OLED_SDA  = 0;
		OLED_SCL = 0;
		OLED_SCL = 1;
	}
	
#else // IIC��ʽд����
	
	IIC_Start();
	Write_IIC_Byte(0x78);
	Write_IIC_Byte(0x40);			//write data
	Write_IIC_Byte(dat);
	IIC_Stop();
	
#endif
}

/*********************OLEDд����************************************/										
void OLED_WrCmd(unsigned char cmd)
{
#if IIC_SPI_TYPE//SPI��ʽд����
	
	unsigned char i;
	OLED_DC=0;
	for(i=0;i<8;i++) //����һ����λ����
	{
		if((cmd << i) & 0x80)
		{
			OLED_SDA  = 1;
		}
		else
		{
			OLED_SDA  = 0;
		}
		OLED_SCL = 0;
		OLED_SCL = 1;
	}
	
#else //IIC��ʽд����
	
	IIC_Start();
	Write_IIC_Byte(0x78);            //Slave address,SA0=0
	Write_IIC_Byte(0x00);			//write command
	Write_IIC_Byte(cmd);
	IIC_Stop();
	
#endif
}

/*********************OLED ��������************************************/
void OLED_SetPos(unsigned char x, unsigned char y)
{ 
	OLED_WrCmd(0xb0+y);
	OLED_WrCmd(((x&0xf0)>>4)|0x10);
	OLED_WrCmd((x&0x0f)|0x01); 
}

/*********************OLEDȫ��************************************/
void OLED_Fill(unsigned char bmp_dat) 
{
	unsigned char y,x;
	for(y=0;y<8;y++)
	{
		OLED_WrCmd(0xb0+y);
		OLED_WrCmd(0x01);
		OLED_WrCmd(0x10);
		for(x=0;x<X_WIDTH;x++)
		OLED_WrDat(bmp_dat);
	}
}

/*********************OLED����************************/
void OLED_CLS(void)
{
	OLED_Fill(0x00);
}

/*************����OLED��ʾ*************/
void OLED_ON(void)
{
	OLED_WrCmd(0X8D);  //���õ�ɱ�
	OLED_WrCmd(0X14);  //������ɱ�
	OLED_WrCmd(0XAF);  //OLED����       
}

/*************�ر�OLED��ʾ -- ʹOLED��������ģʽ*************/
void OLED_OFF(void)
{
	OLED_WrCmd(0X8D);  //���õ�ɱ�
	OLED_WrCmd(0X10);  //�رյ�ɱ�
	OLED_WrCmd(0XAE);  //OLED����
}

/*********************OLED��ʼ��************************************/
void OLED_Init(void)
{
	DelayMs(500);
	
#if CHIP_IS_SSD1306
	
	OLED_WrCmd(0xae);//--turn off oled panel
	OLED_WrCmd(0x00);//---set low column address
	OLED_WrCmd(0x10);//---set high column address
	OLED_WrCmd(0x40);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	OLED_WrCmd(0x81);//--set contrast control register
	OLED_WrCmd(Brightness); // Set SEG Output Current Brightness
	OLED_WrCmd(0xa1);//--Set SEG/Column Mapping     0xa0���ҷ��� 0xa1����
	OLED_WrCmd(0xc8);//Set COM/Row Scan Direction   0xc0���·��� 0xc8����
	OLED_WrCmd(0xa6);//--set normal display
	OLED_WrCmd(0xa8);//--set multiplex ratio(1 to 64)
	OLED_WrCmd(0x3f);//--1/64 duty
	OLED_WrCmd(0xd3);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	OLED_WrCmd(0x00);//-not offset
	OLED_WrCmd(0xd5);//--set display clock divide ratio/oscillator frequency
	OLED_WrCmd(0x80);//--set divide ratio, Set Clock as 100 Frames/Sec
	OLED_WrCmd(0xd9);//--set pre-charge period
	OLED_WrCmd(0xf1);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	OLED_WrCmd(0xda);//--set com pins hardware configuration
	OLED_WrCmd(0x12);
	OLED_WrCmd(0xdb);//--set vcomh
	OLED_WrCmd(0x40);//Set VCOM Deselect Level
	OLED_WrCmd(0x20);//-Set Page Addressing Mode (0x00/0x01/0x02)
	OLED_WrCmd(0x02);//
	OLED_WrCmd(0x8d);//--set Charge Pump enable/disable
	OLED_WrCmd(0x14);//--set(0x10) disable
	OLED_WrCmd(0xa4);// Disable Entire Display On (0xa4/0xa5)
	OLED_WrCmd(0xa6);// Disable Inverse Display On (0xa6/a7) 
	OLED_WrCmd(0xaf);//--turn on oled panel
	
#else //1106�ĳ�ʼ��
	
	OLED_WrCmd(0xAE);    /*display off*/
	OLED_WrCmd(0x02);    /*set lower column address*/
	OLED_WrCmd(0x10);    /*set higher column address*/
	OLED_WrCmd(0x40);    /*set display start line*/
	OLED_WrCmd(0xB0);    /*set page address*/
	OLED_WrCmd(0x81);    /*contract control*/
	OLED_WrCmd(Brightness);    /*128*/
	OLED_WrCmd(0xA1);    /*set segment remap*/
	OLED_WrCmd(0xA6);    /*normal / reverse*/
	OLED_WrCmd(0xA8);    /*multiplex ratio*/
	OLED_WrCmd(0x3F);    /*duty = 1/32*/
	OLED_WrCmd(0xad);    /*set charge pump enable*/
	OLED_WrCmd(0x8b);    /*    0x8a    ??VCC   */
	OLED_WrCmd(0x30);    /*0X30---0X33  set VPP   9V liangdu!!!!*/
	OLED_WrCmd(0xC8);    /*Com scan direction*/
	OLED_WrCmd(0xD3);    /*set display offset*/
	OLED_WrCmd(0x00);    /*   0x20  */
	OLED_WrCmd(0xD5);    /*set osc division*/
	OLED_WrCmd(0x80);    
	OLED_WrCmd(0xD9);    /*set pre-charge period*/
	OLED_WrCmd(0x1f);    /*0x22*/
	OLED_WrCmd(0xDA);    /*set COM pins*/
	OLED_WrCmd(0x12);		 //0x02 -- duanhang xianshi,0x12 -- lianxuhang xianshi!!!!!!!!!
	OLED_WrCmd(0xdb);    /*set vcomh*/
	OLED_WrCmd(0x40);     
	OLED_WrCmd(0xAF);    /*display ON*/
	
#endif
	
	OLED_Fill(0x00);  //��ʼ����
	OLED_SetPos(0,0); 	
} 

//--------------------------------------------------------------
// Prototype      : void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t)
// Calls          : 
// Parameters     : x:0~127,y:0~63,t:1 ��� 0,���
// Description    : ����
//--------------------------------------------------------------
/*void OLED_DrawPoint(unsigned char x,unsigned char y,unsigned char t)
{
	unsigned char pos,bx,temp=0;
	unsigned char OLED_GRAM[128][8];
	
	if(x>127||y>63)
		return;//������Χ��
	pos = y/8;
	bx = y%8;
	temp = 1<<bx;
	if(t)
		OLED_GRAM[x][pos] |= temp;
	else
		OLED_GRAM[x][pos] &= ~temp;
}*/

//--------------------------------------------------------------
// Prototype      : void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
// Calls          : 
// Parameters     : x,y -- ��ʼ������(x:0~127, y:0~7); ch[]:Ҫ��ʾ���ַ���; TextSize:1 -- 6*8����,2 -- 8*16����
// Description    : ��ʾ��ͬ�����ASCII�ַ�
//--------------------------------------------------------------
/*void OLED_Char(unsigned char x, unsigned char y, unsigned char ch)
{
	unsigned char c = 0,i = 0;
	while(ch != '\0')
	{
		c = ch - 32;
		OLED_SetPos(x,y);
		for(i=0;i<6;i++)
			OLED_WrDat(F6x8[c][i]);
	}
}*/

void OLED_ShowChar(unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
{
	unsigned char c = 0,i = 0,j = 0;
	switch(TextSize)
	{
		case 1:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 126)
				{
					x = 0;
					y++;
				}
				OLED_SetPos(x,y);
				for(i=0;i<6;i++)
					OLED_WrDat(F6x8[c][i]);
				x += 6;
				j++;
			}
		}break;
		case 2:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 120)
				{
					x = 0;
					y++;
				}
				OLED_SetPos(x,y);
				for(i=0;i<8;i++)
					OLED_WrDat(F8X16[c*16+i]);
				OLED_SetPos(x,y+1);
				for(i=0;i<8;i++)
					OLED_WrDat(F8X16[c*16+i+8]);
				x += 8;
				j++;
			}
		}break;
	}
}

//--------------------------------------------------------------
// Prototype      : void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N)
// Calls          : 
// Parameters     : x,y -- ��ʼ������(x:0~127, y:0~7); N:������codetab.h�е�����
// Description    : ��ʾcodetab.h�еĺ���,16*16����
//--------------------------------------------------------------
void OLED_ShowCN(unsigned char x, unsigned char y, unsigned char N)
{
	unsigned char wm=0;
	unsigned int  adder=32*N;
	OLED_SetPos(x , y);
	for(wm = 0;wm < 16;wm++)
	{
		OLED_WrDat(F16x16[adder]);
		adder += 1;
	}
	OLED_SetPos(x,y + 1);
	for(wm = 0;wm < 16;wm++)
	{
		OLED_WrDat(F16x16[adder]);
		adder += 1;
	}
}

//--------------------------------------------------------------
// Prototype      : void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[]);
// Calls          : 
// Parameters     : x0,y0 -- ��ʼ������(x0:0~127, y0:0~7); x1,y1 -- ���Խ���(������)������(x1:1~128,y1:1~8)
// Description    : ��ʾBMPλͼ
//--------------------------------------------------------------
void OLED_DrawBMP(unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[])
{
	unsigned int j=0;
	unsigned char x,y;

  if(y1%8==0)
		y = y1/8;
  else
		y = y1/8 + 1;
	for(y=y0;y<y1;y++)
	{
		OLED_SetPos(x0,y);
    for(x=x0;x<x1;x++)
		{
			OLED_WrDat(BMP[j++]);
		}
	}
}
