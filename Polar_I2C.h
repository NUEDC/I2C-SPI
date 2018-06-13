#ifndef __POLAR_I2C_H__
#define __POLAR_12C_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include <stdio.h>
#define uint unsigned int
#define uchar unsigned char

   
/* 
 * AT24C02 2kb = 2048bit = 2048/8 B = 256 B
 * 32 pages of 8 bytes each
 *
 * Device Address
 * 1 0 1 0 A2 A1 A0 R/W
 * 1 0 1 0 0  0  0  0 = 0XA0
 * 1 0 1 0 0  0  0  1 = 0XA1 
 */   
 
   
#define EEPROM_Block0_ADDRESS 0xA0   /* E2 = 0 */
#define EEPROM_Block1_ADDRESS 0xA2   /* E2 = 0 */
#define EEPROM_Block2_ADDRESS 0xA4   /* E2 = 0 */
#define EEPROM_Block3_ADDRESS 0xA6   /* E2 = 0 */ 

#define  EEP_Firstpage       0x00
#define  EEP_Secondpage      0x01
#define  EEP_Thirdpage       0x02
#define  EEP_Fourthpage      0x03
   
   
/*信息输出*/
#define EEPROM_DEBUG_ON         0

#define EEPROM_INFO(fmt,arg...)           printf("<<-EEPROM-INFO->> "fmt"\n",##arg)
#define EEPROM_ERROR(fmt,arg...)          printf("<<-EEPROM-ERROR->> "fmt"\n",##arg)
#define EEPROM_DEBUG(fmt,arg...)          do{\
                                          if(EEPROM_DEBUG_ON)\
                                          printf("<<-EEPROM-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                          }while(0)   
   
extern void EEPROM_I2C_Config(); //I2C初始化函数

/******EEPROM 24C02 专用******/
extern void I2C_EE_WaitEepromStandbyState(uint16_t Address) ;
extern void I2C_COL(I2C_TypeDef* I2Cx,FunctionalState STATE);
extern void EEPROM_I2C_Config();
extern uint32_t I2C_EE_PageWrite(u8* pBuffer, u8 WriteAddr, u8 NumByteToWrite,uint16_t Address);
extern uint32_t I2C_EE_ByteWrite(u8* pBuffer, u8 WriteAddr, uint16_t Address);
extern void I2C_EE_BufferWrite(u8* pBuffer, u8 WriteAddr, u16 NumByteToWrite,uint16_t Address);
extern uint32_t I2C_EE_BufferRead(u8* pBuffer, u8 ReadAddr, u16 NumByteToRead, uint16_t Address);

/********Std I2C********/
extern void Std_I2C_Config();
extern uint32_t Std_I2C_Read(uint16_t Address,u8 ReadAddr,u8 NumByte,u8* pBuffer);

#endif
	
