/************************ (C) COPYRIGHT Megahuntmicro *************************
 * @file                : mhscpu_qspi.c
 * @author              : Megahuntmicro
 * @version             : V1.0.0
 * @date                : 21-October-2014
 * @brief               : This file provides all the QSPI firmware functions
 *****************************************************************************/

 /* Includes ----------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "mhscpu.h"
#include "mhscpu_cache.h"
#include "mhscpu_qspi.h"
#include "mhscpu_dma.h"

#if (ENABLE_CACHE_AES)
#include "mh_aes.h"
#endif

#define QSPI_DEVICE_PARA_FLASH_READY_Mask	(BIT(3))


typedef enum
{
    MH_OK = 0x00,
    MH_ERROR = 0x01,
    MH_BUSY = 0x02,
    MH_TIMEOUT = 0x03
}MH_StatusTypeDef;

typedef struct
{
    uint8_t Instruction;       
    QSPI_BusModeTypeDef BusMode;
    QSPI_CmdFormatTypeDef CmdFormat;
    uint32_t Address;
	
    uint32_t WrData;
    uint32_t RdData;
	
}MH_CommandTypeDef;


//��������жϱ�־��λ������Ǿ��Ǳ�������ʱ
//(PP_CMD(1B) + PP_ADDR(3B) + PP_MAX_BYTES(256B) + Cache_line(32B) + Cache_CMD(1B)+Cache_ADDR(3B)) * 8(bits) * 8(CPU��Ƶ�����QSPIƵ�ʵ�8��) 
//(1+ 3 + 256 + 32 + 1 + 3) * 8 * 8
#define MH_QSPI_TIMEOUT_DEFAULT_CNT	(19000) //18944

#define MH_QSPI_ACCESS_REQ_ENABLE	(0x00000001U)
#define MH_QSPI_FLASH_READY_ENABLE	(0x0000006BU)


#define IS_PARAM_NOTNULL(PARAM)				((PARAM) != NULL)

#define IS_QSPI_ADDR(ADDR)					((((int32_t)(ADDR) ) >= (uint32_t)(0x00000000)) &&\
											(((int32_t)(ADDR) ) <= (uint32_t)(0x00FFFFFF)))

#define IS_QSPI_ADDR_ADD_SZ(ADDR, SZ)		((((int32_t)((ADDR) + (SZ))) >= (uint32_t)(0x00000000)) && \
											(((int32_t)((ADDR) + (SZ))) <= (uint32_t)(0x01000000)))

//ʵ�����Ƶ����Ƶ����ʱ��Ϊ42ms
static void delay_40ms(void)
{
	int i, j;
	for(i = 7; i > 0; i --)
		for(j = SYSCTRL->HCLK_1MS_VAL; j > 0; j --);
}

static MH_StatusTypeDef MH_QSPI_Command(MH_CommandTypeDef *cmd, int32_t timeout)
{
	int32_t i;
	MH_StatusTypeDef status = MH_ERROR;
	
	assert_param(IS_PARAM_NOTNULL(cmd));	
	
	MHSCPU_MODIFY_REG32(&(QSPI->REG_WDATA), (QUADSPI_REG_WDATA), (cmd->WrData));
	MHSCPU_MODIFY_REG32(&(QSPI->ADDRES), (QUADSPI_ADDRESS_ADR), (cmd->Address << 8));
	MHSCPU_MODIFY_REG32(&(QSPI->FCU_CMD), 
						(QUADSPI_FCU_CMD_CODE | QUADSPI_FCU_CMD_BUS_MODE | QUADSPI_FCU_CMD_CMD_FORMAT | QUADSPI_FCU_CMD_ACCESS_REQ),
						(((uint32_t)(cmd->Instruction << 24)) |((uint32_t)( cmd->BusMode<< 8)) |((uint32_t)( cmd->CmdFormat << 4))| (MH_QSPI_ACCESS_REQ_ENABLE)));
	
	//Wait For CMD done
	for (i = 0; i < timeout; i += 4)
	{
		if (QSPI->INT_RAWSTATUS & QUADSPI_INT_RAWSTATUS_DONE_IR)
		{
			QSPI->INT_CLEAR = QUADSPI_INT_CLEAR_DONE;
			status = MH_OK;
			break;
		}
	}

	MHSCPU_WRITE_REG32(&(cmd->RdData), QSPI->REG_RDATA);
	return status;
}

static QSPI_StatusTypeDef QSPI_WriteEnable(QSPI_BusModeTypeDef bus_mode)
{
	MH_CommandTypeDef sCommand;

	sCommand.Instruction = WRITE_ENABLE_CMD;       
	sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8;

	if (QSPI_BUSMODE_444 == bus_mode)
	{
		sCommand.BusMode = QSPI_BUSMODE_444;
	}
	else
	{
		sCommand.BusMode = QSPI_BUSMODE_111;
	}
	
	if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
	{
		return QSPI_STATUS_ERROR;
	}
	
	return QSPI_STATUS_OK;
}

//PP,QPP,Sector Erase,Block Erase, Chip Erase, Write Status Reg, Erase Security Reg
static QSPI_StatusTypeDef QSPI_IsBusy(QSPI_BusModeTypeDef bus_mode)
{
	MH_CommandTypeDef sCommand;

	sCommand.Instruction = READ_STATUS_REG1_CMD;       
	sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8_RREG8;

	if (QSPI_BUSMODE_444 == bus_mode)
	{
		sCommand.BusMode = QSPI_BUSMODE_444;
	}
	else
	{
		sCommand.BusMode = QSPI_BUSMODE_111;
	}
	
	if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
	{
		return QSPI_STATUS_ERROR;
	}
		
	if (sCommand.RdData & BIT0)
	{
		return QSPI_STATUS_ERROR;
	}
	return QSPI_STATUS_OK;
}

#define MAX_RD_DATA_LEN		0x10
#define MAX_WR_DATA_LEN		0x04

uint8_t QSPI_Read(QSPI_CommandTypeDef *cmdParam, uint8_t* buf, uint32_t addr, uint32_t sz)
{
	uint32_t read_times = 0, i = 0, j = 0, rxCount = 0;
	uint8_t end_len = 0;
	MH_CommandTypeDef sCommand;

	while (CACHE->CACHE_CS & CACHE_IS_BUSY);	
	
	addr &= (uint32_t)(0x00FFFFFF);
	assert_param(IS_QSPI_ADDR(addr));
	assert_param(IS_QSPI_ADDR_ADD_SZ(addr, sz));

	if (cmdParam == NULL)
	{
		sCommand.Instruction = READ_CMD;
		sCommand.BusMode = QSPI_BUSMODE_111;
		sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8_ADDR24_RDAT;
	}
	else
	{
		sCommand.Instruction = cmdParam->Instruction;       
		sCommand.BusMode = cmdParam->BusMode;
		sCommand.CmdFormat = cmdParam->CmdFormat;
	}
	
	read_times = sz / MAX_RD_DATA_LEN;
	end_len = sz % MAX_RD_DATA_LEN;

	for (i = 0; i < read_times; i ++)
	{
		QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_RFFH;
		QSPI->BYTE_NUM = MAX_RD_DATA_LEN;
		
		sCommand.Address = addr;

		if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
		{
			return QSPI_STATUS_ERROR;
		}

		rxCount = (QSPI->FIFO_CNTL & 0x7);
		for (j = 0; j < rxCount; j++)
		{
			*(uint32_t *)(buf) = QSPI->RD_FIFO;
			buf += 4;
		}
		
		addr += MAX_RD_DATA_LEN;
	}

	QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_RFFH;
	sCommand.Address = addr;
	if (end_len > 0)
	{
		QSPI->BYTE_NUM = end_len;
		
		if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
		{
			return QSPI_STATUS_ERROR;
		}

		rxCount = (QSPI->FIFO_CNTL & 0x7);
		for (j = 0; j < rxCount; j++)
		{
			*(uint32_t *)(buf) = QSPI->RD_FIFO;
			buf += 4;
		}
	}
	
	return QSPI_STATUS_OK;
}


static uint8_t QSPI_ProgramPage_Ex(QSPI_CommandTypeDef *cmdParam, uint32_t adr, uint32_t sz, uint8_t *buf)
{
	uint32_t i,j;
	uint32_t end_addr, current_size, current_addr, current_dat,loop_addr;
	MH_CommandTypeDef sCommand;

	adr &= (uint32_t)0x00FFFFFF;
	assert_param(IS_QSPI_ADDR(adr));
	assert_param(IS_QSPI_ADDR_ADD_SZ(adr, sz));
	
	current_addr = 0;

	while (current_addr <= adr)
	{
       	current_addr += X25Q_PAGE_SIZE;
	}
	current_size = current_addr - adr;

	/* Check if the size of the data is less than the remaining place in the page */
	if (current_size > sz)
	{
		current_size = sz;
	}

    /* Initialize the adress variables */
	current_addr = adr;
	loop_addr = adr;
	end_addr = adr + sz;

	QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_TFFH;
	QSPI->BYTE_NUM = MAX_WR_DATA_LEN << 16;
	sCommand.Address = current_addr;

	if (cmdParam == NULL)
	{
		sCommand.Instruction = QUAD_INPUT_PAGE_PROG_CMD;       
		sCommand.BusMode = QSPI_BUSMODE_114;
		sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8_ADDR24_PDAT;
	}
	else
	{
		sCommand.Instruction = cmdParam->Instruction;   
		sCommand.BusMode = cmdParam->BusMode;
		sCommand.CmdFormat = cmdParam->CmdFormat;
	}
	
	do
	{
		QSPI->BYTE_NUM = MAX_WR_DATA_LEN << 16;
		
		for (i = 0; i < (current_size/MAX_WR_DATA_LEN); i++)
		{
			QSPI->WR_FIFO = (*(buf + 3) << 24) | (*(buf + 2) << 16) | (*(buf + 1) << 8) | (*(buf + 0));
	
	        if (QSPI_WriteEnable(sCommand.BusMode) != QSPI_STATUS_OK)
	        {
				return QSPI_STATUS_ERROR;
	        }
	        
	        if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != QSPI_STATUS_OK)
	        {
	        	return QSPI_STATUS_ERROR;
	        }
			
	        while (QSPI_IsBusy(sCommand.BusMode));
				
	        QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_TFFH;
			buf += 4;

			current_addr += MAX_WR_DATA_LEN;
			sCommand.Address = current_addr;
        }
		
		if (current_size % MAX_WR_DATA_LEN > 0)
		{
			current_dat = 0;
			QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_TFFH;
			
			for (j = (current_size % MAX_WR_DATA_LEN); j > 0; j--)
			{
				current_dat <<= 8;
				current_dat |= *(buf + j - 1);
			}
			
			QSPI->WR_FIFO = current_dat;
			QSPI->BYTE_NUM = ((current_size % MAX_WR_DATA_LEN)) << 16;
		    
			if (QSPI_WriteEnable(sCommand.BusMode) != QSPI_STATUS_OK)
	       	{
				return QSPI_STATUS_ERROR;
			}
	        
			MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT);
			while(QSPI_IsBusy(sCommand.BusMode));
			
			buf += (current_size % MAX_WR_DATA_LEN);
			sCommand.Address = (current_addr + current_size % MAX_WR_DATA_LEN);
		}
		
       	loop_addr += current_size;
		current_addr = loop_addr;
		current_size = ((loop_addr + X25Q_PAGE_SIZE) > end_addr) ? (end_addr - loop_addr) : X25Q_PAGE_SIZE;
	}while (loop_addr < end_addr);
	
	return QSPI_STATUS_OK;
}


#define MAX_RD_DMA_DATA_LEN		0x100
#define MAX_WR_DMA_DATA_LEN		0x100

static QSPI_StatusTypeDef QSPI_DMA_Configuration(DMA_TypeDef *DMA_Channelx, DMA_InitTypeDef *DMA_InitStruct)
{
	DMA_InitStruct->DMA_DIR = DMA_DIR_Memory_To_Peripheral;
	DMA_InitStruct->DMA_Peripheral = (uint32_t)(QSPI);
	DMA_InitStruct->DMA_PeripheralBaseAddr = (uint32_t)&(QSPI->WR_FIFO);
	DMA_InitStruct->DMA_PeripheralInc = DMA_Inc_Nochange;
	DMA_InitStruct->DMA_PeripheralDataSize = DMA_DataSize_Word;
	DMA_InitStruct->DMA_PeripheralBurstSize = DMA_BurstSize_4;
	
	DMA_InitStruct->DMA_MemoryInc = DMA_Inc_Increment;
	DMA_InitStruct->DMA_MemoryDataSize = DMA_DataSize_Word;
	DMA_InitStruct->DMA_MemoryBurstSize = DMA_BurstSize_4;
	DMA_InitStruct->DMA_BlockSize = MAX_WR_DMA_DATA_LEN >> 2;
	DMA_InitStruct->DMA_PeripheralHandShake = DMA_PeripheralHandShake_Hardware;

	if (DMA_Channelx == DMA_Channel_0)
	{
		SYSCTRL->DMA_CHAN = (SYSCTRL->DMA_CHAN & ~0x1F) | 0x1A;
	}
	else if (DMA_Channelx == DMA_Channel_1)
	{
		SYSCTRL->DMA_CHAN = (SYSCTRL->DMA_CHAN & ~(0x1F << 8)) | (0x1A << 8);
	}
	else if (DMA_Channelx == DMA_Channel_2)
	{
		SYSCTRL->DMA_CHAN = (SYSCTRL->DMA_CHAN & ~(0x1F << 16)) | (0x1A << 16);
	}
	else if (DMA_Channelx == DMA_Channel_3)
	{
		SYSCTRL->DMA_CHAN = (SYSCTRL->DMA_CHAN & ~(0x1F << 24)) | (0x1A << 24);
	}
	else if (DMA_Channelx == DMA_Channel_4)
	{
		SYSCTRL->DMA_CHAN1 = (SYSCTRL->DMA_CHAN1 & ~0x1F) | 0x1A;
	}		
	else if (DMA_Channelx == DMA_Channel_5)
	{
		SYSCTRL->DMA_CHAN1 = (SYSCTRL->DMA_CHAN1 & ~(0x1F << 8)) | (0x1A << 8);
	}
	else if (DMA_Channelx == DMA_Channel_6)
	{
		SYSCTRL->DMA_CHAN1 = (SYSCTRL->DMA_CHAN1 & ~(0x1F << 16)) | (0x1A << 16);
	}
	else if (DMA_Channelx == DMA_Channel_7)
	{
		SYSCTRL->DMA_CHAN1 = (SYSCTRL->DMA_CHAN1 & ~(0x1F << 24)) | (0x1A << 24);
	}		
	else
	{
		return	QSPI_STATUS_NOT_SUPPORTED;
	}
	
	return QSPI_STATUS_OK;
}

#if(ENABLE_CACHE_AES)

#define DEBUG_AES 	0
uint32_t mh_frand(void *rand, uint32_t bytes)
{	
	uint32_t i;
	
	for(i = 0; i < bytes; i++)
	{
		if( i % 2 )
			((uint8_t*)rand)[i] = 0x11;//(MH_TRNG->RNG_PN) & 0xFF;
		else
			((uint8_t*)rand)[i] = 0x22;//((MH_TRNG->RNG_PN) >> 3) & 0xFF;
	}
	return i;
}

uint32_t mh_rand_p(void *rand, uint32_t bytes, void *para)
{
	return mh_frand(rand, bytes);
}
static uint8_t buf_aes_enc(uint8_t *cp_buf, uint32_t sz, uint8_t *sup_buf)
{
	uint8_t is_sup = FALSE;
	mh_pack_mode_def modes[2] = { ECB, CBC};
	mh_rng_callback f_rng = mh_rand_p;
	uint32_t i,k,cnt_buf;
	int j;
	
	uint8_t key128[] = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00};
	uint8_t iv[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

	uint8_t cipher[32];
	uint8_t plain[32];
											 
	memset(cipher, 0, sizeof(cipher));
	memset(plain, 0, sizeof(plain));
	
#if(DEBUG_AES)	
	printf("Plain:\r\n");
	for(i = 0; i < sz; i ++)
	{
		printf("%02X ",cp_buf[i]);
	}
	printf("\r\n");
#endif

	for(i = 0; i < sz/32; i ++)
	{
		//memcpy(plain, cp_buf + i * 32, 32);
		k = 0;
		for(j = 15; j >= 0; j --)
		{
			plain[k] = cp_buf[ i * 32 + j];
			plain[k + 16] = cp_buf[i * 32 + 16 + j];
			k += 1;
		}
		
		mh_aes_enc(modes[1], cipher, 32, plain, 32, key128, MH_AES_128, iv, f_rng, NULL);
		
		k = 0;
		for(j = 15; j >= 0; j --)
		{
			cp_buf[i * 32 + j] = cipher[k];
			cp_buf[i * 32 + 16 + j] = cipher[k + 16];
			k += 1;
		}
		//memcpy(cp_buf + i * 32, cipher, 32);
	}
	cnt_buf = i;
	
	if(sz % 32)
	{
		uint8_t  tmp;
		
		is_sup = TRUE;
		memset(plain, 0xFF, 32);
		memcpy(plain, cp_buf + cnt_buf * 32, sz%32);
		
		for(j = 0; j < 8; j ++)
		{
			tmp = plain[j];
			plain[j] = plain[15 - j];
			plain[15 - j] = tmp;

			tmp = plain[j + 16];
			plain[j + 16] = plain[31 - j];
			plain[31 - j] = tmp;

		}
		mh_aes_enc(modes[1], cipher, 32, plain, 32, key128, MH_AES_128, iv, f_rng, NULL);
	
		k = 0;
		for(j = 15; j >= 0; j --)
		{
			sup_buf[j] = cipher[k];
			sup_buf[16 + j] = cipher[k + 16];
			k += 1;
		}
	}
#if(DEBUG_AES)
	printf("cipher: sz = %#x\r\n", sz);
	for(i = 0; i < (sz / 32) * 32; i ++)
	{
		printf("%02X ",cp_buf[i]);
	}
	for(i = 0; i < 32; i ++)
	{
		printf("%02X ",sup_buf[i]);
	}
	printf("\r\n");
	
	printf("AES ECB key128 test\n");
#endif

	return is_sup;
}
#endif

uint8_t QSPI_SoftWareReset(QSPI_BusModeTypeDef BusMode)
{
	MH_CommandTypeDef sCommand;
	
	sCommand.Instruction = RESET_ENABLE_CMD;      
	sCommand.BusMode =BusMode;
	sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8;

	if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != QSPI_STATUS_OK)
	{
		return QSPI_STATUS_ERROR;
	}

	sCommand.Instruction = RESET_MEMORY_CMD;      
	sCommand.BusMode =BusMode;
	sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8;

	if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != QSPI_STATUS_OK)
	{
		return QSPI_STATUS_ERROR;
	}

	//��λ�������ʱ30ms����
	delay_40ms();
	
	return QSPI_STATUS_OK;
}

uint8_t QSPI_SingleCommand(QSPI_CommandTypeDef *cmdParam)
{
	MH_CommandTypeDef sCommand;

	sCommand.Instruction = cmdParam->Instruction;      
	sCommand.BusMode = cmdParam->BusMode;
	sCommand.CmdFormat = cmdParam->CmdFormat;

	if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != QSPI_STATUS_OK)
	{
		return QSPI_STATUS_ERROR;
	}
	
	return QSPI_STATUS_OK;
}

uint8_t QSPI_DeepPowerDown(QSPI_CommandTypeDef *cmdParam)
{
	MH_CommandTypeDef sCommand;
	
	while (CACHE->CACHE_CS & CACHE_IS_BUSY);	
	
	if (NULL == cmdParam)
	{
		sCommand.Instruction = DEEP_POWER_DOWN;      
		sCommand.BusMode = QSPI_BUSMODE_111;
		sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8;
	}
	else
	{
		sCommand.Instruction = cmdParam->Instruction; 
		sCommand.BusMode 	= cmdParam->BusMode;
		sCommand.CmdFormat = cmdParam->CmdFormat;
	}
	if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
	{
		return QSPI_STATUS_ERROR;
	}
	return QSPI_STATUS_OK;
}

uint8_t QSPI_ReleaseDeepPowerDown(QSPI_CommandTypeDef *cmdParam)
{
	MH_CommandTypeDef sCommand;
	uint32_t clock_delay; 
	
	while (CACHE->CACHE_CS & CACHE_IS_BUSY);	
	
	if (NULL == cmdParam)
	{
		sCommand.Instruction 	= RELEASE_FROM_DEEP_POWER_DOWN;      
		sCommand.BusMode = QSPI_BUSMODE_111;
		sCommand.CmdFormat 	= QSPI_CMDFORMAT_CMD8;
	}
	else
	{
		sCommand.Instruction = cmdParam->Instruction; 
		sCommand.BusMode 	= cmdParam->BusMode;
		sCommand.CmdFormat = cmdParam->CmdFormat;
	}
	if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
	{
		return QSPI_STATUS_ERROR;
	}

	//��ֹ�����ڼ�CACHE����FLASH��������
	//��ʱ30us
	for(clock_delay =0; clock_delay < (SYSCTRL->HCLK_1MS_VAL* 150 /1000); clock_delay += 4);

	return QSPI_STATUS_OK;
}

uint32_t QSPI_ReadID(QSPI_CommandTypeDef *cmdParam)
{
	MH_CommandTypeDef sCommand;

	while (CACHE->CACHE_CS & CACHE_IS_BUSY);	
	
	if (cmdParam == NULL)
	{
		sCommand.Instruction = READ_JEDEC_ID_CMD;    
		sCommand.BusMode = QSPI_BUSMODE_111;
		sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8_RREG24;
	}
	else
	{
		sCommand.Instruction = cmdParam->Instruction;   
		sCommand.BusMode = cmdParam->BusMode;
		sCommand.CmdFormat = cmdParam->CmdFormat;
	}

	if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != QSPI_STATUS_OK)
	{
		return QSPI_STATUS_ERROR;
	}
	
	return (sCommand.RdData & 0x00FFFFFF);
}

uint16_t QSPI_StatusReg(QSPI_CommandTypeDef *cmdParam)
{
	MH_CommandTypeDef sCommand;

	assert_param(IS_PARAM_NOTNULL(cmdParam));
	
	while (CACHE->CACHE_CS & CACHE_IS_BUSY);	
	
	sCommand.Instruction = cmdParam->Instruction;
	sCommand.BusMode = cmdParam->BusMode;
	sCommand.CmdFormat = cmdParam->CmdFormat;
	
	if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != QSPI_STATUS_OK)
	{
		return QSPI_STATUS_ERROR;
	}
	
	return (sCommand.RdData & 0x0000FFFF);
}

uint8_t QSPI_WriteParam(QSPI_CommandTypeDef *cmdParam, uint16_t wrData)
{
	MH_CommandTypeDef sCommand;
	
	assert_param(IS_PARAM_NOTNULL(cmdParam));

	while (CACHE->CACHE_CS & CACHE_IS_BUSY);	
	
	sCommand.Instruction = cmdParam->Instruction;
	sCommand.BusMode = cmdParam->BusMode;
	sCommand.CmdFormat = cmdParam->CmdFormat;
	sCommand.WrData = wrData;
	
	if (QSPI_WriteEnable(sCommand.BusMode) != QSPI_STATUS_OK)
	{
		return QSPI_STATUS_ERROR;
	}

	if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
	{
		return QSPI_STATUS_ERROR;
	}
	while (QSPI_IsBusy(sCommand.BusMode));
	
	return QSPI_STATUS_OK;
}

uint8_t QSPI_EraseSector(QSPI_CommandTypeDef *cmdParam, uint32_t SectorAddress)
{
	MH_CommandTypeDef sCommand;
	SectorAddress &= (uint32_t)(0x00FFFFFF);
	
	assert_param(IS_QSPI_ADDR(SectorAddress));	
	
	while (CACHE->CACHE_CS & CACHE_IS_BUSY);	
	
	if(cmdParam == NULL)
	{
		sCommand.Instruction = SECTOR_ERASE_CMD;       
		sCommand.BusMode = QSPI_BUSMODE_111;
		sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8_ADDR24;
	}
	else
	{
		sCommand.Instruction = cmdParam->Instruction;       
		sCommand.BusMode = cmdParam->BusMode;
		sCommand.CmdFormat 	= cmdParam->CmdFormat;
	}

	sCommand.Address = SectorAddress;
	
	if (QSPI_WriteEnable(sCommand.BusMode) != QSPI_STATUS_OK)
	{
		return QSPI_STATUS_ERROR;
	}

	if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
	{
		return QSPI_STATUS_ERROR;
	}
	
	while(QSPI_IsBusy(sCommand.BusMode));

	return QSPI_STATUS_OK;
}

uint8_t QSPI_EraseChip (QSPI_CommandTypeDef *cmdParam)
{
	MH_CommandTypeDef sCommand;
	
	while (CACHE->CACHE_CS & CACHE_IS_BUSY);	
	
	if(cmdParam == NULL)
	{
		sCommand.Instruction = CHIP_ERASE_CMD;       
		sCommand.BusMode = QSPI_BUSMODE_111;
		sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8;
	}
	else
	{
		sCommand.Instruction 	= cmdParam->Instruction;       
		sCommand.BusMode = cmdParam->BusMode;
		sCommand.CmdFormat 	= cmdParam->CmdFormat;
	}

	while(QSPI_IsBusy(sCommand.BusMode));
	
	if (QSPI_WriteEnable(sCommand.BusMode) != QSPI_STATUS_OK)
	{
		return QSPI_STATUS_ERROR;
	}
	
	if((MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT)) != MH_OK)
	{
		return QSPI_STATUS_ERROR;
	}

	while(QSPI_IsBusy(sCommand.BusMode));
	
	return QSPI_STATUS_OK;
}

uint8_t QSPI_ProgramPage(QSPI_CommandTypeDef *cmdParam, DMA_TypeDef *DMA_Channelx, uint32_t adr, uint32_t sz, uint8_t *buf)
{
	uint32_t i;
	uint32_t end_addr, current_size, current_addr = 0,loop_addr;
	DMA_InitTypeDef DMA_InitStruct;
	MH_CommandTypeDef sCommand;
	
	while (CACHE->CACHE_CS & CACHE_IS_BUSY);	
	
	if (DMA_Channelx == NULL)
	{
		return QSPI_ProgramPage_Ex(cmdParam, adr, sz, buf);
	}
	
	adr &= (uint32_t)(0x00FFFFFF);
	assert_param(IS_QSPI_ADDR(adr));
	assert_param(IS_QSPI_ADDR_ADD_SZ(adr, sz));
	
	while (current_addr <= adr)
	{
	   current_addr += X25Q_PAGE_SIZE;
	}
	current_size = current_addr - adr;

	/* Check if the size of the data is less than the remaining place in the page */
	if (current_size > sz)
	{
	   current_size = sz;
	}

	/* Initialize the adress variables */
	current_addr = adr;
	loop_addr = adr;
	end_addr = adr + sz;

	if (cmdParam == NULL)
	{
		sCommand.Instruction	= QUAD_INPUT_PAGE_PROG_CMD;       
		sCommand.BusMode    =QSPI_BUSMODE_114;
		sCommand.CmdFormat  = QSPI_CMDFORMAT_CMD8_ADDR24_PDAT;
	}
	else
	{
		sCommand.Instruction	= cmdParam->Instruction;     
		sCommand.BusMode     	= cmdParam->BusMode;
		sCommand.CmdFormat  = cmdParam->CmdFormat;
	}
	
	if (QSPI_DMA_Configuration(DMA_Channelx, &DMA_InitStruct) != QSPI_STATUS_OK)
	{
		return QSPI_STATUS_ERROR;
	}
	
	do
	{
		for (i = 0; i < (current_size/MAX_WR_DMA_DATA_LEN); i++)
	    {
			sCommand.Address = current_addr;
			QSPI->BYTE_NUM = MAX_WR_DMA_DATA_LEN << 16;
			QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_TFFH;

			DMA_InitStruct.DMA_BlockSize = (MAX_WR_DMA_DATA_LEN / 4);			
			DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)buf;
			DMA_Init(DMA_Channelx, &DMA_InitStruct);

			//Enable DMA
			DMA_ChannelCmd(DMA_Channelx, ENABLE);
			QSPI->DMA_CNTL |= QUADSPI_DMA_CNTL_TX_EN;

			if (QSPI_WriteEnable(sCommand.BusMode) != QSPI_STATUS_OK)
	        {
				return QSPI_STATUS_ERROR;
	      	}
	        	
			if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
			{
				return QSPI_STATUS_ERROR;
			}
			
	        while(QSPI_IsBusy(sCommand.BusMode));

			QSPI->DMA_CNTL &= ~QUADSPI_DMA_CNTL_TX_EN;
			DMA_ChannelCmd(DMA_Channelx, DISABLE);
			
			buf += MAX_WR_DMA_DATA_LEN;
			current_addr += MAX_WR_DMA_DATA_LEN;
			sCommand.Address = current_addr;
			    
		}
		if (current_size % MAX_WR_DMA_DATA_LEN > 0)
		{	
			QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_TFFH;
			sCommand.Address = current_addr;
			QSPI->BYTE_NUM = (current_size % MAX_WR_DMA_DATA_LEN) << 16;
			
			DMA_InitStruct.DMA_BlockSize = ((current_size + 3) % MAX_WR_DMA_DATA_LEN) >> 2;//DMA_MemoryDataSize must be DMA_DataSize_Word
			DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)buf;
			DMA_Init(DMA_Channelx, &DMA_InitStruct);

			DMA_ChannelCmd(DMA_Channelx, ENABLE);
			QSPI->DMA_CNTL |= QUADSPI_DMA_CNTL_TX_EN;
			
			if (QSPI_WriteEnable(sCommand.BusMode) != QSPI_STATUS_OK)
			{
				return QSPI_STATUS_ERROR;
			}
		        
			if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
			{
				return QSPI_STATUS_ERROR;
			}
			
			while(QSPI_IsBusy(sCommand.BusMode));

			QSPI->DMA_CNTL &= ~QUADSPI_DMA_CNTL_TX_EN;
			DMA_ChannelCmd(DMA_Channelx, DISABLE);
			buf += current_size % MAX_WR_DMA_DATA_LEN;
		}  

       	loop_addr += current_size;
       	current_addr = loop_addr;
 
		current_size = ((loop_addr + X25Q_PAGE_SIZE) > end_addr) ? (end_addr - loop_addr) : X25Q_PAGE_SIZE;
	} while (loop_addr < end_addr);
	
	//disable DMA
	DMA_ChannelCmd(DMA_Channelx,DISABLE);
	return QSPI_STATUS_OK;
}

#if(ENABLE_CACHE_AES)
uint8_t QSPI_ProgramPage_ByAES(QSPI_CommandTypeDef *cmdParam,  DMA_TypeDef *DMA_Channelx, uint32_t adr, uint32_t sz, uint8_t *buf)
{
	uint32_t i;
	uint32_t end_addr, current_size, current_addr = 0,loop_addr;
	DMA_InitTypeDef DMA_InitStruct;
	uint8_t mplain[32];
	MH_CommandTypeDef sCommand;

	while (CACHE->CACHE_CS & CACHE_IS_BUSY);		
	
	adr &= (uint32_t)(0x00FFFFFF);
	
	assert_param(IS_QSPI_ADDR(adr));
	assert_param(IS_QSPI_ADDR_ADD_SZ(adr, sz));	
	
	if (DMA_Channelx == NULL)
	{
		DMA_Channelx = DMA_Channel_0;
	}
	
	while (current_addr <= adr)
	{
	   current_addr += X25Q_PAGE_SIZE;
	}
	current_size = current_addr - adr;

	/* Check if the size of the data is less than the remaining place in the page */
	if (current_size > sz)
	{
	   current_size = sz;
	}

	/* Initialize the adress variables */
	current_addr = adr;
	loop_addr = adr;
	end_addr = adr + sz;

	buf_aes_enc((uint8_t*)(buf), sz, (uint8_t *)mplain);
	if (QSPI_DMA_Configuration(DMA_Channelx, &DMA_InitStruct) != QSPI_STATUS_OK)
	{
		return QSPI_STATUS_ERROR;
	}

	if (cmdParam == NULL)
	{
		sCommand.Instruction = QUAD_INPUT_PAGE_PROG_CMD;       
		sCommand.BusMode =QSPI_BUSMODE_114;
		sCommand.CmdFormat = QSPI_CMDFORMAT_CMD8_ADDR24_PDAT;
	}
	else
	{
		sCommand.Instruction	= cmdParam->Instruction;     
		sCommand.BusMode     	= cmdParam->BusMode;
		sCommand.CmdFormat  = cmdParam->CmdFormat;
	}
	
	do
	{
		for (i = 0; i < (current_size/MAX_WR_DMA_DATA_LEN); i++)
	    {
			sCommand.Address = current_addr;
			QSPI->BYTE_NUM = MAX_WR_DMA_DATA_LEN << 16;
			QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_TFFH;

			DMA_InitStruct.DMA_BlockSize = (MAX_WR_DMA_DATA_LEN / 4);
			DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)buf;
			DMA_Init(DMA_Channelx, &DMA_InitStruct);

			//Enable DMA
			DMA_ChannelCmd(DMA_Channelx,ENABLE);
			QSPI->DMA_CNTL |= QUADSPI_DMA_CNTL_TX_EN;

			if (QSPI_WriteEnable(sCommand.BusMode) != QSPI_STATUS_OK)
	        {
				return QSPI_STATUS_ERROR;
	        }
	        	
			if (MH_QSPI_Command(&sCommand, MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
			{
				return QSPI_STATUS_ERROR;
			}
	        
			while(QSPI_IsBusy(sCommand.BusMode));

			QSPI->DMA_CNTL &= ~QUADSPI_DMA_CNTL_TX_EN;
			DMA_ChannelCmd(DMA_Channelx, DISABLE);

			buf += MAX_WR_DMA_DATA_LEN;
			current_addr += MAX_WR_DMA_DATA_LEN;
			sCommand.Address = current_addr;	    
		}
		
		if (current_size % MAX_WR_DMA_DATA_LEN > 0)
		{	
			uint8_t cnt_aes_div, cnt_aes_rem;
			cnt_aes_rem = current_size % 32;
			cnt_aes_div = current_size /32;
			
			if (cnt_aes_div)
			{
				QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_TFFH;
				sCommand.Address= current_addr;
				QSPI->BYTE_NUM = ((cnt_aes_div * 32)) << 16;
					
				DMA_InitStruct.DMA_BlockSize = (cnt_aes_div * 32);
				DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)buf;
				DMA_Init(DMA_Channelx, &DMA_InitStruct);

				DMA_ChannelCmd(DMA_Channelx,ENABLE);
				QSPI->DMA_CNTL |= QUADSPI_DMA_CNTL_TX_EN;
				
				if (QSPI_WriteEnable(sCommand.BusMode) != QSPI_STATUS_OK)
				{
					return QSPI_STATUS_ERROR;
				}
			        
				if (MH_QSPI_Command(&sCommand,MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
				{
					return QSPI_STATUS_ERROR;
				}
				while(QSPI_IsBusy(sCommand.BusMode));

				QSPI->DMA_CNTL &= ~QUADSPI_DMA_CNTL_TX_EN;
				DMA_ChannelCmd(DMA_Channelx, DISABLE);
				
				buf += cnt_aes_div * 32;
				current_addr += (cnt_aes_div * 32);
				QSPI->ADDRES = current_addr << 8;
			}
			if (cnt_aes_rem)
			{
				QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_TFFH;
				sCommand.Address= current_addr;
				QSPI->BYTE_NUM =  32 << 16;
					
				DMA_InitStruct.DMA_BlockSize = 32;
				DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)mplain;
				DMA_Init(DMA_Channelx, &DMA_InitStruct);

				DMA_ChannelCmd(DMA_Channelx,ENABLE);

				QSPI->DMA_CNTL |= QUADSPI_DMA_CNTL_TX_EN;
				if (QSPI_WriteEnable(sCommand.BusMode) != QSPI_STATUS_OK)
				{
					return QSPI_STATUS_ERROR;
				}
			        
				if (MH_QSPI_Command(&sCommand,MH_QSPI_TIMEOUT_DEFAULT_CNT) != MH_OK)
				{
					return QSPI_STATUS_ERROR;
				}
				while(QSPI_IsBusy(sCommand.BusMode));

				QSPI->DMA_CNTL &= ~QUADSPI_DMA_CNTL_TX_EN;
				DMA_ChannelCmd(DMA_Channelx, DISABLE);				
			}	
		}  

       	loop_addr += current_size;
       	current_addr = loop_addr;
 
		current_size = ((loop_addr + X25Q_PAGE_SIZE) > end_addr) ? (end_addr - loop_addr) : X25Q_PAGE_SIZE;
	} while (loop_addr < end_addr);
	
	//disable DMA
	DMA_ChannelCmd(DMA_Channelx, DISABLE);
	return QSPI_STATUS_OK;
}
#endif

void QSPI_Init(QSPI_InitTypeDef *mhqspi)
{
	if (mhqspi == NULL)
	{
		QSPI->DEVICE_PARA = ((QSPI->DEVICE_PARA & ~0xFF) | 0x6B) | QUADSPI_DEVICE_PARA_SAMPLE_PHA;
	}
	else
	{
		QSPI->CACHE_INTF_CMD = (uint32_t)((mhqspi->Cache_Cmd_ReleaseDeepInstruction << 24)
										 | (mhqspi->Cache_Cmd_DeepInstruction<<16)
										 | ((mhqspi->Cache_Cmd_ReadBusMode & 0x03) << 12)
									     | ((mhqspi->Cache_Cmd_ReadFormat & 0x0F) << 8) | (mhqspi->Cache_Cmd_ReadInstruction));
		
		QSPI->DEVICE_PARA  = (uint32_t)((QSPI->DEVICE_PARA & ~0xFFFF)
									   | (((mhqspi->SampleDly & 0x01) << QSPI_DEVICE_PARA_SAMPLE_DLY_Pos)
								       | ((mhqspi->SamplePha & 0x01) << QSPI_DEVICE_PARA_SAMPLE_PHA_Pos)
								       | ((mhqspi->ProToCol & 0x03) << QSPI_DEVICE_PARA_PROTOCOL_Pos)
								       | ((mhqspi->DummyCycles & 0x0F) << QSPI_DEVICE_PARA_DUMMY_CYCLE_Pos)
								       | ((mhqspi->FreqSel & 0x03)) 
									   | QSPI_DEVICE_PARA_FLASH_READY_Mask));
	}
}

/**
  * @brief  Sets the QSPI latency value.
  * @param  u32UsClk: specifies the QSPI Latency value.
  * @retval None
  */
void QSPI_SetLatency(uint32_t u32UsClk)
{
	SYSCTRL_ClocksTypeDef clocks;    
    
	if (0 == u32UsClk)
	{
        SYSCTRL_GetClocksFreq(&clocks);  
        QSPI->DEVICE_PARA = (QSPI->DEVICE_PARA & 0xFFFF) | ((clocks.CPU_Frequency*2/1000000) << 16);        
	}
	else
	{
        QSPI->DEVICE_PARA = (QSPI->DEVICE_PARA & 0xFFFF) | (u32UsClk << 16);
	}
}

/**
  * @brief  Flash Erase Sector.
  * @param  sectorAddress: The sector address to be erased
  * @retval FLASH Status:  The returned value can be: QSPI_STATUS_ERROR, QSPI_STATUS_OK
  */
uint8_t FLASH_EraseSector(uint32_t sectorAddress)
{
    uint8_t ret;
    
	__disable_irq();
	__disable_fault_irq();	

    ret = ROM_QSPI_EraseSector(NULL, sectorAddress);
    
	__enable_fault_irq();
	__enable_irq();

    return ret;
}

/**
  * @brief  Flash Program Interface.
  * @param  addr:          specifies the address to be programmed.
  * @param  size:          specifies the size to be programmed.
  * @param  buffer:        pointer to the data to be programmed, need word aligned
  * @retval FLASH Status:  The returned value can be: QSPI_STATUS_ERROR, QSPI_STATUS_OK
  */
uint8_t FLASH_ProgramPage(uint32_t addr, uint32_t size, uint8_t *buffer)
{
    uint8_t ret;
	QSPI_CommandTypeDef cmdType;
    
    cmdType.Instruction = QUAD_INPUT_PAGE_PROG_CMD;
    cmdType.BusMode = QSPI_BUSMODE_114;	      
    cmdType.CmdFormat = QSPI_CMDFORMAT_CMD8_ADDR24_PDAT;     
    
	__disable_irq();
	__disable_fault_irq();	

    ret = ROM_QSPI_ProgramPage(&cmdType, DMA_Channel_1, addr, size, buffer);
    
	__enable_fault_irq();
	__enable_irq();

    return ret;   
}

/**
  * @brief  Enable or Disable QSPI's Interrupt.
  * @param  QSPI_IT: specify the Interrupt
  *         This parameter can be one of the following values:
  *      @arg QSPI_IT_TX_FIFO_DATA
  *      @arg QSPI_IT_RX_FIFO_DATA
  *      @arg QSPI_IT_TX_FIFO_OF
  *      @arg QSPI_IT_TX_FIFO_UF
  *      @arg QSPI_IT_RX_FIFO_OF
  *      @arg QSPI_IT_RX_FIFO_UF
  *      @arg QSPI_IT_DONE_INT
  * @param  NewState: new state of Interrupt
  *         This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
void QSPI_ITConfig(uint32_t QSPI_IT, FunctionalState NewState)
{
	if (NewState != DISABLE)
	{
		QSPI->INT_UMASK |= QSPI_IT;
	}
	else
	{
		QSPI->INT_MASK |= QSPI_IT;
	}
}

/**
  * @brief  Clear the QSPI's interrupt bits
  * @param  QSPI_IT: specify the Interrupt
  *         This parameter can be one of the following values:
  *      @arg QSPI_IT_TX_FIFO_DATA
  *      @arg QSPI_IT_RX_FIFO_DATA
  *      @arg QSPI_IT_TX_FIFO_OF
  *      @arg QSPI_IT_TX_FIFO_UF
  *      @arg QSPI_IT_RX_FIFO_OF
  *      @arg QSPI_IT_RX_FIFO_UF
  *      @arg QSPI_IT_DONE_INT
  * @retval None
  */
void QSPI_ClearITPendingBit(uint32_t QSPI_IT)
{
	QSPI->INT_CLEAR |= QSPI_IT;
}

ITStatus QSPI_GetITStatus(uint32_t QSPI_IT)
{
	if ((QSPI->INT_STATUS & QSPI_IT) != RESET)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

ITStatus QSPI_GetITRawStatus(uint32_t QSPI_IT)
{
	if ((QSPI->INT_RAWSTATUS & QSPI_IT) != RESET)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}


void QSPI_TxFIFOFlush(void)
{
	QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_TFFH;
}

FlagStatus QSPI_TxFIFOEmpty(void)
{
	if (QSPI->FIFO_CNTL & QUADSPI_FIFO_CNTL_TFE)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

FlagStatus QSPI_TxFIFOFull(void)
{
	if (QSPI->FIFO_CNTL & QUADSPI_FIFO_CNTL_TFFL)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

uint8_t QSPI_TxFIFOLevel(void)
{
	return ((QSPI->FIFO_CNTL>>16) & 0xF);
}

void QSPI_RxFIFOFlush(void)
{
	QSPI->FIFO_CNTL |= QUADSPI_FIFO_CNTL_RFFH;
}


FlagStatus QSPI_RxFIFOEmpty(void)
{
	if (QSPI->FIFO_CNTL & QUADSPI_FIFO_CNTL_RFE)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

FlagStatus QSPI_RxFIFOFull(void)
{
	if(QSPI->FIFO_CNTL & QUADSPI_FIFO_CNTL_RFFL)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

uint8_t QSPI_RxFIFOLevel(void)
{
	return ((QSPI->FIFO_CNTL) & 0xF);
}

uint32_t QSPI_ReadRxFIFO(void)
{
	return QSPI->RD_FIFO;
}

void QSPI_WriteTxFIFO(uint32_t data)
{
	QSPI->WR_FIFO = data;
}


/**************************      (C) COPYRIGHT Megahunt    *****END OF FILE****/
