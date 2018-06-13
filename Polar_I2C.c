#include "stm32f10x.h"
#include "Polar_I2C.h"
#include "PeriphDef.h"
#include <stdio.h>

#define uint unsigned int
#define uchar unsigned char
	
#define I2CT_FLAG_TIMEOUT         ((uint32_t)0x1000)                                //等待超时时间标志  
#define I2CT_LONG_TIMEOUT         ((uint32_t)(10 * I2CT_FLAG_TIMEOUT))              //等待超时时间
#define EEPROM_ERROR(fmt,arg...)   printf("<<-EEPROM-ERROR->> "fmt"\n",##arg)       //错误输出位  （摘录自秉火/野火）
#define I2C_PageSize           8

static __IO uint32_t  I2CTimeout = I2CT_LONG_TIMEOUT;                              //设置最长等待时间


uint16_t EQUIP_ADDRESS;                                                             //设备地址




/**************************************************************************************/
         /*         POWER BY POLAR FRAMEWORK            */
         /*          Team  Advanced Studio              */
         /*               Polar_Tech                    */
/*
用户调用提示：
参量声明  
缓冲区：ReadBuffer[....] WriteBuffer[....]
 
函数调用：
        初始化函数  EEPROM_I2C_Config()   or     Std_I2C_Config()
        装载控制字（注意 设备地址 和 读写地址）  buffer装载
        发送函数   I2C_EE_PageWrite(u8* pBuffer, u8 WriteAddr, u8 NumByteToWrite,uint16_t Address)
            or     Std_I2C_Write(uint16_t Address,u8 WriteAddr,u8 NumByte,u8* pBuffer)
        读取相应值 I2C_EE_BufferRead(u8* pBuffer, u8 ReadAddr, u16 NumByteToRead, uint16_t Address)
            or     Std_I2C_Read(uint16_t Address,u8 ReadAddr,u8 NumByte,u8* pBuffer)
        可以中途对I2C进行控制，调用函数
                  I2C_COL(I2C_TypeDef* I2Cx,FunctionalState STATE)
*/
/*****************************************************************************************/
/*
I2C用户函数
不需要用户访问
*/
static  uint32_t I2C_TIMEOUT_UserCallback(uint8_t errorCode)
{
  /* Block communication and all processes */
  EEPROM_ERROR("I2C 等待超时!errorCode = %d",errorCode);
  
  return 0;
}

/*
等待EEPROM的Standby信号
不需要用户访问
*/
void I2C_EE_WaitEepromStandbyState(uint16_t Address)      
{
  vu16 SR1_Tmp = 0;

  do
  {
    /* Send START condition */
    I2C_GenerateSTART(I2C1, ENABLE);
    /* Read I2C1 SR1 register */
    SR1_Tmp = I2C_ReadRegister(I2C1, I2C_Register_SR1);
    /* Send EEPROM address for write */
    I2C_Send7bitAddress(I2C1, Address, I2C_Direction_Transmitter);
  }while(!(I2C_ReadRegister(I2C1, I2C_Register_SR1) & 0x0002));
  
  /* Clear AF flag */
  I2C_ClearFlag(I2C1, I2C_FLAG_AF);
    /* STOP condition */    
    I2C_GenerateSTOP(I2C1, ENABLE); 
}
/*
I2C控制函数
void I2C_COL(I2C_TypeDef* I2Cx,FunctionalState STATE)
I2Cx   I2C1 I2C2
STATE   ENABLE DISABLE
*/
void I2C_COL(I2C_TypeDef* I2Cx,FunctionalState STATE)
{
  I2C_Cmd(I2Cx, STATE);
}
/*
I2C1初始化函数
void I2C_Config()
初始化I2C1 
*/
void EEPROM_I2C_Config()
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  I2C_InitTypeDef  I2C_InitStructure;
	
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;	       // 开漏输出
  GPIO_Init(GPIOB, &GPIO_InitStructure);
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;	       // 开漏输出
  GPIO_Init(GPIOB, &GPIO_InitStructure);	
	
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	
  I2C_InitStructure.I2C_OwnAddress1 =  0X0A  ; 
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable ;
	
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = 400000;

  I2C_Init(I2C1, &I2C_InitStructure);
  I2C_Cmd(I2C1, ENABLE); 
}

/**************************************************************************/
/************************/
/*EEPROM_24C02专用*/
/***********************/
/**************************************************************************/

/*
在EEPROM中写函数
pBuffer          缓冲区数组
WriteAddr        写地址
NumByteToWrite   写字节数
Address          设备地址
*/
uint32_t I2C_EE_PageWrite(u8* pBuffer, u8 WriteAddr, u8 NumByteToWrite,uint16_t Address)
{
  I2CTimeout = I2CT_LONG_TIMEOUT;

  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))   
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(4);
  } 
  
  /* Send START condition */
  I2C_GenerateSTART(I2C1, ENABLE);
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))  
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(5);
  } 
  
  /* Send EEPROM address for write */
  I2C_Send7bitAddress(I2C1, Address, I2C_Direction_Transmitter);
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))  
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(6);
  } 
  
  /* Send the EEPROM's internal address to write to */    
  I2C_SendData(I2C1, WriteAddr);  

  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV8 and clear it */
  while(! I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(7);
  } 

  /* While there is data to be written */
  while(NumByteToWrite--)  
  {
    /* Send the current byte */
    I2C_SendData(I2C1, *pBuffer); 

    /* Point to the next byte to be written */
    pBuffer++; 
  
    I2CTimeout = I2CT_FLAG_TIMEOUT;

    /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
      if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(8);
    } 
  }

  /* Send STOP condition */
  I2C_GenerateSTOP(I2C1, ENABLE);
  
  return 1;
}



/*
在EEPROM中写一个字节
uint32_t I2C_EE_ByteWrite(u8* pBuffer, u8 WriteAddr, uint16 Address)
pBuffer  缓冲区
WriteAddr 写地址
Address   设备地址
*/
uint32_t I2C_EE_ByteWrite(u8* pBuffer, u8 WriteAddr, uint16_t Address)
{
   /* Send STRAT condition */
  I2C_GenerateSTART(I2C1, ENABLE);

  I2CTimeout = I2CT_FLAG_TIMEOUT;  
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))  
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(0);
  } 
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Send EEPROM address for write */
  I2C_Send7bitAddress(I2C1, Address, I2C_Direction_Transmitter);
  
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(1);
  }  
  /* Send the EEPROM's internal address to write to */
  I2C_SendData(I2C1, WriteAddr);
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(2);
  } 
  
  /* Send the byte to be written */
  I2C_SendData(I2C1, *pBuffer); 
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;  
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(3);
  } 
  
  /* Send STOP condition */
  I2C_GenerateSTOP(I2C1, ENABLE);
  
  return 1;
}

/*
写字节到 EEPROM
void I2C_EE_BufferWrite(u8* pBuffer, u8 WriteAddr, u16 NumByteToWrite)
pBuffer:                 缓冲区指针
WriteAddr:               写地址 
NumByteToWrite           写的字节数
Address                  设备地址
*/
void I2C_EE_BufferWrite(u8* pBuffer, u8 WriteAddr, u16 NumByteToWrite,uint16_t Address)
{
  u8 NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0;

  Addr = WriteAddr % I2C_PageSize;
  count = I2C_PageSize - Addr;
  NumOfPage =  NumByteToWrite / I2C_PageSize;
  NumOfSingle = NumByteToWrite % I2C_PageSize;
 
  /* If WriteAddr is I2C_PageSize aligned  */
  if(Addr == 0) 
  {
    /* If NumByteToWrite < I2C_PageSize */
    if(NumOfPage == 0) 
    {
      I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle, Address);
      I2C_EE_WaitEepromStandbyState(Address);
    }
    /* If NumByteToWrite > I2C_PageSize */
    else  
    {
      while(NumOfPage--)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, I2C_PageSize, Address); 
    	I2C_EE_WaitEepromStandbyState(Address);
        WriteAddr +=  I2C_PageSize;
        pBuffer += I2C_PageSize;
      }

      if(NumOfSingle!=0)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle, Address);
        I2C_EE_WaitEepromStandbyState(Address);
      }
    }
  }
  /* If WriteAddr is not I2C_PageSize aligned  */
  else 
  {
    /* If NumByteToWrite < I2C_PageSize */
    if(NumOfPage== 0) 
    {
      I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle, Address);
      I2C_EE_WaitEepromStandbyState(Address);
    }
    /* If NumByteToWrite > I2C_PageSize */
    else
    {
      NumByteToWrite -= count;
      NumOfPage =  NumByteToWrite / I2C_PageSize;
      NumOfSingle = NumByteToWrite % I2C_PageSize;	
      
      if(count != 0)
      {  
        I2C_EE_PageWrite(pBuffer, WriteAddr, count, Address);
        I2C_EE_WaitEepromStandbyState(Address);
        WriteAddr += count;
        pBuffer += count;
      } 
      
      while(NumOfPage--)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, I2C_PageSize, Address);
        I2C_EE_WaitEepromStandbyState(Address);
        WriteAddr +=  I2C_PageSize;
        pBuffer += I2C_PageSize;  
      }
      if(NumOfSingle != 0)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle, Address); 
        I2C_EE_WaitEepromStandbyState(Address);
      }
    }
  }  
}

/*
I2C从设备读取
uint32_t I2C_EE_BufferRead(u8* pBuffer, u8 ReadAddr, u16 NumByteToRead,uint16_t Address)
pBuffer         缓冲区指针
ReadAddr        读地址 
NumByteToRead   读字节数
Address         设备地址
*/
uint32_t I2C_EE_BufferRead(u8* pBuffer, u8 ReadAddr, u16 NumByteToRead, uint16_t Address)
{  
  
  I2CTimeout = I2CT_LONG_TIMEOUT;
  
  //*((u8 *)0x4001080c) |=0x80; 
  while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(9);
   }
  
  /* Send START condition */
  I2C_GenerateSTART(I2C1, ENABLE);
  //*((u8 *)0x4001080c) &=~0x80;
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(10);
   }
  
  /* Send EEPROM address for write */
  I2C_Send7bitAddress(I2C1, Address, I2C_Direction_Transmitter);

  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(11);
   }
    
  /* Clear EV6 by setting again the PE bit */
  I2C_Cmd(I2C1, ENABLE);

  /* Send the EEPROM's internal address to write to */
  I2C_SendData(I2C1, ReadAddr);  

   
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(12);
   }
    
  /* Send STRAT condition a second time */  
  I2C_GenerateSTART(I2C1, ENABLE);
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(13);
   }
    
  /* Send EEPROM address for read */
  I2C_Send7bitAddress(I2C1, Address, I2C_Direction_Receiver);
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(14);
   }
  
  /* While there is data to be read */
  while(NumByteToRead)  
  {
    if(NumByteToRead == 1)
    {
      /* Disable Acknowledgement */
      I2C_AcknowledgeConfig(I2C1, DISABLE);
      
      /* Send STOP Condition */
      I2C_GenerateSTOP(I2C1, ENABLE);
    }

    /* Test on EV7 and clear it */    
    I2CTimeout = I2CT_LONG_TIMEOUT;
    
		while(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED)==0)  
		{
			if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(3);
		} 
    {      
      /* Read a byte from the EEPROM */
      *pBuffer = I2C_ReceiveData(I2C1);

      /* Point to the next location where the byte read will be saved */
      pBuffer++; 
      
      /* Decrement the read bytes counter */
      NumByteToRead--;        
    }   
  }

  /* Enable Acknowledgement to be ready for another reception */
  I2C_AcknowledgeConfig(I2C1, ENABLE);
  
    return 1;
}

/***********************************************************************/
/*****************/
/*标准I2C接口*/
/****************/
/**********************************************************************/



/*
标准I2C接口初始化    I2C2
void Std_I2C_Config();
*/
void Std_I2C_Config()
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  I2C_InitTypeDef  I2C_InitStructure;
	
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;	       // 开漏输出
  GPIO_Init(GPIOB, &GPIO_InitStructure);
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;	       // 开漏输出
  GPIO_Init(GPIOB, &GPIO_InitStructure);	
	
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	
  I2C_InitStructure.I2C_OwnAddress1 =  0X0A  ; 
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable ;
	
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = 400000;

  I2C_Init(I2C1, &I2C_InitStructure);
  I2C_Cmd(I2C1, ENABLE); 
}
/*

标准I2C写数据函数
void Std_I2C_Write(uint16_t Address,u8 WriteAddr,u8 NumByte,u8* pBuffer)
Address    设备地址
WriteAddr  写地址
NumByte    写的字节数
pBuffer    缓冲区指针

*/
uint32_t Std_I2C_Write(uint16_t Address,u8 WriteAddr,u8 NumByte,u8* pBuffer)
{
  I2CTimeout = I2CT_LONG_TIMEOUT;

  while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))   
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(4);
  } 
  
  I2C_GenerateSTART(I2C2,ENABLE);
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))  
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(5);
  } 
  
  I2C_Send7bitAddress(I2C2,Address,I2C_Direction_Transmitter);
   I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))  
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(6);
  } 
  
  I2C_SendData(I2C2, WriteAddr);  
 I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV8 and clear it */
  while(! I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(7);
  } 
  
  while(NumByte--)
  {
    I2C_SendData(I2C2, *pBuffer); 
    pBuffer++;
    I2CTimeout = I2CT_FLAG_TIMEOUT;

    /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
      if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(8);
    } 
  }
  I2C_GenerateSTOP(I2C2, ENABLE);
  return 1;
}
/*
标准I2C读数据函数
void Std_I2C_Read(uint16_t Address,u8 ReadAddr,u8 NumByte,u8* pBuffer)
Address  设备地址
ReadAddr  读地址
NumByte   读的字节数
pBuffer   缓冲区指针
*/
uint32_t Std_I2C_Read(uint16_t Address,u8 ReadAddr,u8 NumByte,u8* pBuffer)
{
  I2CTimeout = I2CT_LONG_TIMEOUT;
  while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(9);
   }
  I2C_GenerateSTART(I2C2, ENABLE);
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(10);
   }
  I2C_Send7bitAddress(I2C2, Address, I2C_Direction_Transmitter);
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(11);
   }
    
  I2C_Cmd(I2C2, ENABLE);
  I2C_SendData(I2C2, ReadAddr);
   I2C_SendData(I2C2, ReadAddr);  

   
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(12);
   }
  I2C_GenerateSTART(I2C2, ENABLE);
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(13);
   }
  I2C_Send7bitAddress(I2C2, Address, I2C_Direction_Receiver);
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(14);
   }
  while(NumByte)  
  {
    if(NumByte == 1)
    {
      I2C_AcknowledgeConfig(I2C2, DISABLE);
      I2C_GenerateSTOP(I2C2, ENABLE);
    }
    I2CTimeout = I2CT_LONG_TIMEOUT;
    
		while(I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)==0)  
		{
			if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(3);
		} 
  *pBuffer = I2C_ReceiveData(I2C2);
  pBuffer++;
  NumByte--; 
  }
  return 1;
}
