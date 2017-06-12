#include "modbus.h"

uint8_t MbRcCount, MbTrCount, cTempUART;  //счетчик принятых/переданных данных дданных
bool MbStartRec = false;// false/true начало/прием посылки
unsigned char cNumRcByte; //передает в обработчик кол-во принятых байт
unsigned char cNumTrByte;
unsigned char MbRcBuf[MaxLenghtRecBuf]; //буфер принимаемых данных
unsigned char MbTrBuf[MaxLenghtTrBuf]; //буфер передаваемых данных

uint16_t MbDataRegs[MaxDataLen] = {0,1,2,1500,0,0,6,7,8,0x0A};
uint16_t CRC16;

//--------------ErrorMessage--------------------//
//формирование ответа об ошибке
uint8_t ErrorMessage(uint8_t Error)
{
  char TempI;
  MbTrBuf[1]=MbRcBuf[1]+0x80;;//команда с ошибкой
  MbTrBuf[2]=Error;
  TempI = GetCRC16(MbTrBuf,3);//подсчет КС посылки
  MbTrBuf[3] = LO(TempI);
  MbTrBuf[4] = HI(TempI);
  return 5;
}//end ErrorMessage()


//--------------ModBus--------------------//
//обработка запроса в соответствие с кол-вом принятых байт NumByte
uint8_t ModBus(uint8_t NumByte) 
{
  uint16_t TempI;

  //обработка посылки
  if (MbRcBuf[0]!=RS485Addr) return 0x00; //адрес устройства  //ответ не нужен

  CRC16 = GetCRC16(MbRcBuf,NumByte-2);//подсчет CRC в принятой посылке без последних
																		//двух байт состоящих из CRC
	
  TempI = (uint16_t) (MbRcBuf[NumByte-1]<<8) + MbRcBuf[NumByte-2];//в TempI пишем КС прин.посылки

  if (CRC16 != TempI) return 0x00;  //контрольная сумма не совпала//ответ не нужен
  
  MbTrBuf[0] = RS485Addr;//адрес устройства
  
  //код команды
  switch(MbRcBuf[1]){

    case 0x03:{//чтение регистров

		//В данном применении считываем с 0 адреса и MaxDataLen регистров
			
      TempI = (uint16_t) (MbRcBuf[2]<<8) + MbRcBuf[3];
      if (TempI != 0){ //проверка номера первого регистра, д.б. 0
        return ErrorMessage(0x02); //данный адрес не может быть обработан
      }
			
      TempI=(uint16_t) (MbRcBuf[4]<<8) + MbRcBuf[5];
      if (TempI != MaxDataLen){//проверка кол-ва запрашиваемых регистров, есть только MaxDataLen регистров
         return ErrorMessage(0x02); //данный адрес не может быть обработан
      }

      MbTrBuf[1] = 0x03;//команда
      MbTrBuf[2] = MaxDataLen * 2;//кол-во байт данных (*2 число регистров)
			for (uint8_t i = 0; i < MaxDataLen; i++) {
				MbTrBuf[3 + i*2] = HI(MbDataRegs[i]);
				MbTrBuf[4 + i*2] = LO(MbDataRegs[i]);
			}
			TempI = GetCRC16(MbTrBuf, 3 + MaxDataLen * 2);//подсчет КС посылки
      MbTrBuf[3 + MaxDataLen * 2] = LO(TempI);
      MbTrBuf[4 + MaxDataLen * 2] = HI(TempI);
      return (5 + MaxDataLen * 2);
    }

    case 0x06:{//запись в единичный регистр
      TempI = (uint16_t) (MbRcBuf[2]<<8) + MbRcBuf[3];
      if (TempI > MaxDataLen){ //проBерка номера регистра, есть только 1 регистр
         return ErrorMessage(0x02); //данный адрес не может быть обработан
      }
      TempI = (uint16_t) (MbRcBuf[4]<<8) + MbRcBuf[5];
      if (TempI > 0xFFFF){  //проверка числа для записи
         return ErrorMessage(0x03); //недопустимые данные в запросе
      }
			
			//MbDataRegs[(uint8_t) (MbRcBuf[2]<<8) + MbRcBuf[3]] = (uint16_t) (MbRcBuf[4]<<8) + MbRcBuf[5];
			MbDataRegs[MbRcBuf[3]] = (uint16_t) (MbRcBuf[4]<<8) + MbRcBuf[5];

      MbTrBuf[1]=MbRcBuf[1];//команда
      MbTrBuf[2]=MbRcBuf[2];//адрес
      MbTrBuf[3]=MbRcBuf[3];//
      MbTrBuf[4]=HI(MbDataRegs[MbRcBuf[3]]);//данные
      MbTrBuf[5]=LO(MbDataRegs[MbRcBuf[3]]);//
      MbTrBuf[6]=MbRcBuf[6];//КС
      MbTrBuf[7]=MbRcBuf[7];//
			
      return 8;
    }
    default:{
       return ErrorMessage(0x01); //недопустимая команда
    }
  }
}//end ModBus()


//массивы для быстрого расчета кода CRC-16
const unsigned char srCRCHi[256]={ // переписать во флеш!
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
         0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
         0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
         0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
         0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
         0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
         0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
         0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
         0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
         0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
         0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
         0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
         0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
         0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
         0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};
const unsigned char srCRCLo[256]={// переписать во флеш!
         0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
         0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
         0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
         0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
         0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
         0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
         0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
         0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
         0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
         0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
         0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
         0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
         0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
         0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
         0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
         0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
         0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
         0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
         0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
         0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
         0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
         0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
         0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
         0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
         0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
         0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

//функция вычисляет код CRC-16
//на входе указатель на начало буфера
//и количество байт сообщения (без принятого кода CRC-16)
uint16_t GetCRC16(unsigned char *buf, char bufsize)
{
  char CRC_Low = 0xFF;
  char CRC_High = 0xFF;
  unsigned char k;
  unsigned char carry;
  for (k=0; k<bufsize; k++)
   {
    carry = CRC_Low ^ buf[k];
    CRC_Low = CRC_High ^ srCRCHi[carry];
    CRC_High = srCRCLo[carry];
   };
  //return (CRC_High);
  return((CRC_High<<8)|CRC_Low);
}//end 