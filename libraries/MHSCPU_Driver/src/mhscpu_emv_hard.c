
//#include "EMV_unit_test.h"
//#include <arm_math.h>
#include <stdio.h>
#include "mhscpu_sci.h"
#include "emv_core.h"
#include "emv_queue.h"
#include "emv_hard.h"
//#include "test_emv.h"

#define FREQ_Mhz(f)           (f * 1000000UL)
#define DBG_PRINT(format, args...)\
        printf(format, ##args)

//card insert
#define SCICARDIN             (1 << 0)
//card pull out
#define SCICARDOUT            (1 << 1)
//card power up
#define SCICARDUP             (1 << 2)
//card power down
#define SCICARDDN             (1 << 3)
//SCI found tx error
#define SCITXERR              (1 << 4)
//TS(first ATR char) timeout
#define SCIATRSTOUT           (1 << 5)
//recv ATR timeout(more than 19200 etus)
#define SCIATRDTOUT           (1 << 6)
//Block timeout
#define SCIBLKTOUT            (1 << 7)
//char timeout
#define SCICHTOUT             (1 << 8)
//FIFO read timeout(Data in FIFO and has not been read for a period of time).
#define SCIRTOUT              (1 << 9)
//rx FIFO's count is more than x
#define SCIRXTIDE             (1 << 10)
//tx FIFO's count is less than x 
#define SCITXTIDE             (1 << 11)



//0: checking error.
#define CR0_RXNAK             (1 << 5)
//0:Even 1:Odd
#define CR0_RXPARITY          (1 << 4)
//0:Do not check error.
#define CR0_TXNAK             (1 << 3)
//0:Even 1:Odd
#define CR0_TXPARITY          (1 << 2)
//0:Low = 0(Direct use) 1:Low = 1(Inverse use)
#define CR0_ORDER             (1 << 1)
//0:Direct 1:Inverse
#define CR0_SENSE             (1 << 0)

//0: use debounce. 1: Bypass debounce.
#define CR1_EXDBNCE           (1 << 5)
//0:Disable 1:Enable
#define CR1_BGTEN             (1 << 4)
//0:SCICLK configured as buffer ouput.
//1:SCICLK configured as pulled down(open drain).
#define CR1_CLKZ1             (1 << 3)
//0:Recv 1:Send.
#define CR1_MODE_SEND         (1 << 2)
//0:Disable. 1:Enable.
#define CR1_BLKEN             (1 << 1)
//ATR duration time. 
//0:Disable. 1:Enable.
#define CR1_ATRDEN            (1 << 0)

//Set 1 to initiates a warm reset.
#define CR2_WRESET            (1 << 2)
//Set 1 to deactivates the card.
#define CR2_FINISH            (1 << 1)
//Set 1 to starts the activation of the card.
#define CR2_STARTUP           (1 << 0)


#define SCI0_CG_BIT            (BIT(14))
//#define SCI1_CG_BIT            (BIT(15))
#define SCI2_CG_BIT            (BIT(16))

#define SLOT_COUNT          (EMV_TERMINAL_NUM)
#define SLOT_NUM_MAX        (SLOT_COUNT - 1)


#define FIFO_DEP            (8)

/* user card core structure */
volatile struct emv_core  *user_dev = NULL;
/* the parameters save region */
volatile struct emv_core  *soft_dev = NULL;


//#define BIT(n)            (1UL << (n))

#define EMV_LEVEL_1_AUTH        (1)

#if(EMV_LEVEL_1_AUTH)
#define RX_FIFO_THR             (0)
#else
#define RX_FIFO_THR             (4)
#endif

#define TX_FIFO_THR             (4)


#if 1
typedef enum
{
    EMV_SCI_INACTIVE,
    EMV_SCI_ACTIVE,
    //EMV_SCI_ATR_TS,
    EMV_SCI_ATR_RECV,
    EMV_SCI_DATA_TRANS,
    EMV_UNKNOWN_STAT,

} EMV_STAT;
#else
typedef enum
{ 
    EMV_SCI_IDLE = 0,           //SCIx has not been configured.
    EMV_POWER_UP,               //Power up when card present.
    EMV_POWER_DOWN,             //inactive card and power down SCIx.
    EMV_RECV_TS,                //Prepare for receiving TS(ATR's first char)
    EMV_RECV_ATR,                //Receiving other characters in ATR
    EMV_TRANS_DATA,
    EMV_RECV_DATA,
    EMV_UNKNOWN_STAT,
} EMV_STAT;
#endif

typedef struct 
{
    volatile EMV_STAT stat;

    int32_t    SCIx_IdealClk;
    int32_t    SCIx_RealClk;
    uint32_t   SCIx_RefClock;       //KHz
    uint16_t   SCIx_ClkReg;
    uint16_t   SCIx_ValueReg;
    uint16_t   SCIx_BaudReg;
    uint16_t   SCIx_StableReg;
    uint16_t   SCIx_DeactiveReg;
    uint16_t   SCIx_Direction;

    volatile uint32_t SCI_IntStatus;

} SCI_EMV_PARAM;


//static  SCI_TypeDef *  const aSCI_BaseTable[3] = {(SCI_TypeDef *)SCI0_BASE, (SCI_TypeDef *)SCI1_BASE, (SCI_TypeDef *)SCI2_BASE};
//static  const uint8_t        aSCI_CG_Table[3] = {(uint8_t)SCI0_CG_BIT, (uint8_t)SCI1_CG_BIT, (uint8_t)SCI2_CG_BIT};

static  SCI_TypeDef *  const aSCI_BaseTable[3] = {(SCI_TypeDef *)SCI0_BASE, NULL, (SCI_TypeDef *)SCI2_BASE};
static  const uint8_t        aSCI_CG_Table[3] = {(uint8_t)SCI0_CG_BIT, NULL, (uint8_t)SCI2_CG_BIT};

/*
volatile struct emv_queue  scs_queue[3];    
static volatile SCI_EMV_PARAM aEMV_Param[3];    
struct emv_core emv_devs[ EMV_TERMINAL_NUM ];
*/

/* datas buffer */
volatile struct emv_queue  scs_queue[3];    
static volatile SCI_EMV_PARAM aEMV_Param[3];    
struct emv_core emv_devs[ EMV_TERMINAL_NUM ];

//#define SET_ERR(n, err)   {scs_queue[n].sf = err;scs_queue[n].pf = scs_queue[n].ip;}


#define SCI_DEBUG

#ifndef SCI_DEBUG
#define MY_STATIC_INLINE    __STATIC_INLINE
#else
#define MY_STATIC_INLINE
#endif

static void SCIx_IRQHandler(uint8_t slot);

//extern uint32_t get_HCLK_1Ms(void);
//extern uint32_t get_PCLK_1Ms(void);

__STATIC_INLINE void SET_ERR(uint8_t n, int32_t err)
{
    //Already set the error flag and didn't be read.
    if (0 != scs_queue[n].sf && scs_queue[n].pf > scs_queue[n].op)
    {
        return;
    }
    scs_queue[n].sf = err;
    scs_queue[n].pf = scs_queue[n].ip;
}

__STATIC_INLINE void FLUSH_QUEUE(uint8_t slot)
{
    if (!(slot < SLOT_COUNT))
    {
        return;
    }

    scs_queue[slot].sf = 0;
    scs_queue[slot].ip = 0;
    scs_queue[slot].op = 0;
    scs_queue[slot].pf = INVALID_INDEX;
}

int isvalid_channel( int slot )
{
    if( slot < SLOT_COUNT )
    {
        return 1;
    }
    
    return 0;
}


uint32_t emv_sci_reset(uint8_t slot)
{
    if (slot > SLOT_NUM_MAX)
    {
        return (1);
    }

    /**********************Here need SCI module reset************************/
    SYSCTRL->SOFT_RST1 = aSCI_CG_Table[slot];

    /**********************Here need SCI module reset************************/

    //ClkReg value is calculated in SCI_ConfigEMV().
    aSCI_BaseTable[slot]->SCI_CLKICC = aEMV_Param[slot].SCIx_ClkReg;
    while (aEMV_Param[slot].SCIx_ClkReg != aSCI_BaseTable[slot]->SCI_CLKICC)
    {
        aSCI_BaseTable[slot]->SCI_CLKICC = aEMV_Param[slot].SCIx_ClkReg;
    }
    //100ns deactive time(reference clock).
    aSCI_BaseTable[slot]->SCI_STABLE = aEMV_Param[slot].SCIx_StableReg;
    //100ns for each stage of deactive squence.
    aSCI_BaseTable[slot]->SCI_DTIME = aEMV_Param[slot].SCIx_DeactiveReg;
    
    return 0;
}

/**
 * select slot
 */
int select_slot( int slot )
{
    SYSCTRL->CG_CTRL1 |= aSCI_CG_Table[slot];
    //emv_sci_reset(slot);

    return ( slot & 0x03 );
}

//static int alarm_counter = 0;
//int emv_alarm_count()
//{
//    return alarm_counter;
//}

void check_dev(uint8_t SCI_Bitmap)
{
    uint8_t i;
    float32_t deviation;

    for (i = 0; i < SLOT_COUNT; i++)
    {
        if (SCI_Bitmap & (1 << i))
        {
            if (0 != aEMV_Param[i].SCIx_IdealClk)
            {
                deviation = ((int32_t)aEMV_Param[i].SCIx_RealClk - (int32_t)aEMV_Param[i].SCIx_IdealClk) * 100.0f / (float32_t)aEMV_Param[i].SCIx_IdealClk;
                //dprintf("slot %d deviation percent %+1.2f%%!\n", i, deviation);
                if (deviation > 4.0f || deviation < -4.0f)
                {
                    while (1);
                }
            }
            else
            {
                //dprintf("slot %d invalid!\n", i);;
                while (1);
            }
            
        }
    }
}

#define CLK_DIV         (1)

int32_t SCI_ConfigEMV(uint8_t SCI_Bitmap, uint32_t SCIx_Clk)
{
    uint8_t i;
    uint32_t SCIx_RefClock;
    uint32_t SCIx_ClkReg;
    int32_t tmp, SICx_RealClock;

    if (0 == SCI_Bitmap || SCI_Bitmap > 7 || SCIx_Clk < FREQ_Mhz(1) || SCIx_Clk > FREQ_Mhz(5))
    {
        return -1;
    }
//#define PCLK_KHZ        (*(uint32_t *)(0x40000024))
    //SCIx reference clock is same as system PCLK(Khz).
    SCIx_RefClock = SYSCTRL->PCLK_1MS_VAL;
    SCIx_Clk *=CLK_DIV;

    //clk = refClk / (2 *( CLkReg + 1)) MHz.
    //tmp = clk * (CLkReg + 1) = refClk * 1000 / 2 Hz
    tmp = SCIx_RefClock * 500UL;
    //The real value in Reg SCICLKICC, round up to SCIx_Clk / 2.
    SCIx_ClkReg = (tmp - (SCIx_Clk >> 1)) / SCIx_Clk;
    //realClock is the real ICC clock(Hz).
    SICx_RealClock = tmp / (SCIx_ClkReg + 1);

    for (i = 0; i < SLOT_COUNT; i++)
    {
        if (SCI_Bitmap & (1 << i))
        {
            aEMV_Param[i].SCIx_IdealClk = SCIx_Clk;
            aEMV_Param[i].SCIx_RealClk = SICx_RealClock;
            aEMV_Param[i].SCIx_RefClock = SCIx_RefClock;
            aEMV_Param[i].SCIx_ClkReg = SCIx_ClkReg;
            //Stable time is 10ms(ref clock), use 16-bit counter feed 8-bit counter.
            //aEMV_Param[i].SCIx_StableReg = (10 * SCIx_RefClock) >> 16;
            //Use NCN8025 stable time is 0ms(ref clock), NCN has already dedounced.
            aEMV_Param[i].SCIx_StableReg = 0;
            //Deacticve time is 100ns(ref clock) for each satage of deactive card.(Total three)
            //aEMV_Param[i].SCIx_DeactiveReg = SCIx_RefClock / 10;
            //Use NCN 8025 Deacticve time should be 0(ref clock).It means shut down immediately.
            aEMV_Param[i].SCIx_DeactiveReg = 1000;
            aEMV_Param[i].SCI_IntStatus = 0;
            SYSCTRL->CG_CTRL1 |= aSCI_CG_Table[i];
            emv_sci_reset(i);
        }
    }
    
    //output ICC clock deviation percent.
//    {
//        float32_t deviation = (SICx_RealClock - (int32_t)SCIx_Clk) * 100.0f / (float32_t)SCIx_Clk;
//        
//        dprintf("deviation percent %+1.2f%%\n", deviation);
//    }
    //check_dev(SCI_Bitmap);
    
    return 0;
}

int32_t SCI_MatchBaudReg(SCI_EMV_PARAM * pSCIx_Param, uint16_t EMV_F, uint16_t EMV_D)
{
    uint16_t tmp, i, mod;
    uint32_t totalClk;

    if (0 == EMV_F || 0 == EMV_D)
    {
        return SCI_EMV_F_D_ERR;
    }

    mod = 0xFF;
    //(1+SCIBAUD)*SCIVALUE = 2F*(SCICLKICC+1)/D
    totalClk = (CLK_DIV * 2 * EMV_F * (pSCIx_Param->SCIx_ClkReg + 1) + (EMV_D >> 1)) / EMV_D;
    for(i = 5; i < 0x100; i++)
    {
        if (((totalClk % i) < mod) && (totalClk / i < 0xFFFF))
        {
            tmp = i;
            if (0 == (mod = totalClk % i))
            {
                break;
            }
        }
    }

    pSCIx_Param->SCIx_ValueReg = tmp;
    pSCIx_Param->SCIx_BaudReg = totalClk / tmp - 1;

    return 0;
}

int emv_disturb_interruption(struct emv_core *pdev, int enable)
{
/*
    if( pdev->terminal_ch < 3)
    {
        if (enable)
        {
            aSCI_BaseTable[pdev->terminal_ch]->SCI_IER = SCICARDIN | SCICARDOUT;
        }
        else
        {
            aSCI_BaseTable[pdev->terminal_ch]->SCI_IER = 0;
        }
    }
*/
    return 0;
}


int emv_hard_init()
{
    /*
    uint8_t i;

    for (i = 0; i < SLOT_COUNT; i++)
    { 
        SCI_ConfigEMV(1 << i, 3000000); 
    }
    */

    return 0;
}

/**
 * Configuarate software parameters.
 */
int emv_hard_config(struct emv_core *pdev)
{
    int32_t s32Tmp;
    SCI_EMV_PARAM volatile *pSCI_Param;
    //SCI_TypeDef * pSCI_Base;
    
    if (pdev->terminal_ch > SLOT_NUM_MAX || NULL == pdev)
    {
        return (-1);
    }

    //Get slot param
    pSCI_Param = &aEMV_Param[pdev->terminal_ch];
    //pSCI_Base = aSCI_BaseTable[pdev->terminal_ch];

    if (0 > (s32Tmp = SCI_MatchBaudReg((SCI_EMV_PARAM *)pSCI_Param, pdev->terminal_fi, pdev->terminal_di)))
    {
        return s32Tmp;
    }

    pdev->queue->sf = 0;
    pdev->queue->pf = INVALID_INDEX;
    
    return (0);

/*
    if (0 > (tmp32 = SCI_MatchBaudReg(pSCI_Param, pdev->terminal_fi, pdev->terminal_di)))
    {
        return tmp32;
    }

    pSCI_Base->SCI_BAUD = pSCI_Param->SCIx_BaudReg;
    pSCI_Base->SCI_VALUE = pSCI_Param->SCIx_ValueReg;
    //TX threshold is 4, 8 is full.
    pSCI_Base->SCI_TIDE = 0x04;
    //Clear tx and rx FIFO
    pSCI_Base->SCI_RXCOUNT = 0x01;
    pSCI_Base->SCI_TXCOUNT = 0x01;
    //RX timeout is 48 etus.
    //tmp32 = pdev->terminal_fi * 48 / pdev->terminal_di;
    tmp32 = pdev->terminal_fi * 14 / pdev->terminal_di;
    pSCI_Base->SCI_RXTIME = tmp32 > 0xFFFF ? 0xFFFF : tmp32;
    
    return 0;
*/
}

/**
 * Detect card whether is in socket or not?
 */
int emv_hard_detect( struct emv_core *pdev )
{
    if (pdev->terminal_ch > SLOT_NUM_MAX)
    {
        return 0;
    }

    /*
    if (0 == (SYS_CTRL->CG_Ctrl & aSCI_BaseTable[pdev->terminal_ch]))
    {
        //Unlock and enable SCIx
        SYS_CTRL->CG_Lock &= ~aSCI_BaseTable[pdev->terminal_ch];
        SYS_CTRL->CG_Ctrl |= aSCI_BaseTable[pdev->terminal_ch];
        //reset SCIx hardware.
        emv_sci_reset(pdev->terminal_ch);
        //Get card present flag.
        SCIx_CardPresent = (aSCI_BaseTable[pdev->terminal_ch]->SCI_ISTAT >> 9) & 1;
        //Disable and lock SCIx
        SYS_CTRL->CG_Ctrl &= ~aSCI_BaseTable[pdev->terminal_ch];
        SYS_CTRL->CG_Lock |= aSCI_BaseTable[pdev->terminal_ch];
    }
    else
    {
        SCIx_CardPresent = (aSCI_BaseTable[pdev->terminal_ch]->SCI_ISTAT >> 9) & 1;
    }
    */

    pdev->terminal_exist = 0;
    if (SCICARDIN == (aSCI_BaseTable[pdev->terminal_ch]->SCI_IIR & SCICARDIN))
    {
        pdev->terminal_exist = 1;
    }

    return (pdev->terminal_exist);
}


//deka use 
//#define ATR_S_TIME (42150)
//BCTC use
#define ATR_S_TIME (43000)
#define ATR_CH_ETU (10080 - 10)


/**
 * cold reset
 */
int emv_hard_cold_reset( struct emv_core *pdev )
{
    int32_t s32Tmp;
    SCI_TypeDef * pSCI_Base;
    volatile SCI_EMV_PARAM * pSCI_Param;
    
    if (!(pdev->terminal_ch < SLOT_COUNT))
    {
        return -1;
    }

    pSCI_Base = aSCI_BaseTable[pdev->terminal_ch];
    //Disable all SCIx interrupts.
    pSCI_Base->SCI_IER = 0;

    pdev->terminal_state = EMV_COLD_RESET;
    /* restor default parameters */
    pdev->terminal_fi = pdev->terminal_implict_fi;
    pdev->terminal_di = pdev->terminal_implict_di;
    pdev->terminal_conv  = 0;    
    
    emv_queue_flush(pdev);
    if (emv_hard_config(pdev))
    {
        SET_ERR(pdev->terminal_ch, ICC_ERR_PARAM);
        return (-1);
    }
#if EMV_LEVEL_1_AUTH
    pSCI_Base->SCI_RXTIME = 0;
#else
    s32Tmp = 1;
    if (0 != pdev->terminal_di)
    {
        //RX timeout is 16 etus.
        s32Tmp = pdev->terminal_fi * 16 / pdev->terminal_di;
        if (s32Tmp > 0xFFFF)
        {
            s32Tmp = 0xFFFF;
        }
        else if (0 == s32Tmp)
        {
            s32Tmp = 1;
        }
    }
    pSCI_Base->SCI_RXTIME = s32Tmp;
#endif    
    //Set communication baudrate.
    pSCI_Param = &aEMV_Param[pdev->terminal_ch];
    pSCI_Base->SCI_BAUD = pSCI_Param->SCIx_BaudReg;
    pSCI_Base->SCI_VALUE = pSCI_Param->SCIx_ValueReg;
    //active time, RST holding low time 40000~45000(ICC clock).
    pSCI_Base->SCI_ATIME = ATR_S_TIME * CLK_DIV;
    //TS waiting time(ICC clock).
    //pSCI_Base->SCI_ATRSTIME = 42372 * CLK_DIV;
    pSCI_Base->SCI_ATRSTIME = ATR_S_TIME * CLK_DIV;
    pSCI_Base->SCI_DTIME = pSCI_Param->SCIx_DeactiveReg;
    //ATR total time(etu).
    pSCI_Base->SCI_ATRDTIME = 19200 + 960;
    //char to char time(etu).
    pSCI_Base->SCI_CHTIME = ATR_CH_ETU;
    //pSCI_Base->SCI_CHTIME = 9600 - 12;
    //don't force IO low, when parity error occuered.
    pSCI_Base->SCI_CR0 = CR0_RXNAK;
    //pSCI_Base->SCI_CR0 = CR0_RXNAK | CR0_RXPARITY;
    //recv mode, ATR duration timeout enable.
    pSCI_Base->SCI_CR1 = CR1_BLKEN | CR1_ATRDEN;
    //RX FIFO is disabled, TX FIFO interrupt is 4.When start to recv ATR(TS), rx FIFO must be 0.
    pSCI_Base->SCI_TIDE = TX_FIFO_THR;
    //Clear tx and rx FIFO
    pSCI_Base->SCI_RXCOUNT = 0x01;
    pSCI_Base->SCI_TXCOUNT = 0x01;
    //Disable send/recv retry in ATR.
    pSCI_Base->SCI_RETRY = 0;
    //Clear interrupts except in/out on/off(power) status. 
    pSCI_Base->SCI_IIR = 0xFF0;
    
    s32Tmp = ICC_ERR_POWER;
    //if card is already power on, we can't active cold reset.
    if (SCICARDDN == (aSCI_BaseTable[pdev->terminal_ch]->SCI_IIR & SCICARDDN))
    {
        s32Tmp = ICC_ERR_NOCARD;
        //card in slot and interrupt flag is set.
        if (SCICARDIN == (aSCI_BaseTable[pdev->terminal_ch]->SCI_IIR & SCICARDIN))
        {
#if (0 == EMV_UNIT_TEST)
            //active card.
            pSCI_Base->SCI_CR2 = CR2_STARTUP;
            aEMV_Param[pdev->terminal_ch].stat = EMV_SCI_ACTIVE;
            pSCI_Base->SCI_IER = SCICARDOUT | SCIATRSTOUT | SCIRTOUT | SCICHTOUT | SCIRXTIDE | SCITXERR;

            //alarm_counter++;
            while(EMV_SCI_ACTIVE == aEMV_Param[pdev->terminal_ch].stat);
#else
            pdev->queue->ip = sizeof(ATR);
            //copy len + 1 as timeout position.
            memcpy(pdev->queue->qbuf, ATR, pdev->queue->ip);
            SET_ERR(pdev->terminal_ch, ICC_ERR_ATR_CWT);
#endif
            return 0;
        }
    }
    
    //No card in slot.
    SET_ERR(pdev->terminal_ch, s32Tmp);
    return -1;
}

/**
 * warm reset
 */
int emv_hard_warm_reset( struct emv_core *pdev )
{
    int32_t s32Tmp;
    SCI_TypeDef * pSCI_Base;
    volatile SCI_EMV_PARAM * pSCI_Param;

    if (!(pdev->terminal_ch < SLOT_COUNT))
    {
        return -1;
    }

    pSCI_Base = aSCI_BaseTable[pdev->terminal_ch];
    //Disable all SCIx interrupts.
    pSCI_Base->SCI_IER = 0;

    pdev->terminal_state = EMV_WARM_RESET;
    /* restor default parameters */
    pdev->terminal_fi = pdev->terminal_implict_fi;
    pdev->terminal_di = pdev->terminal_implict_di;
    pdev->terminal_conv  = 0;
    
    emv_queue_flush(pdev);
    if (emv_hard_config( pdev ))
    {
        SET_ERR(pdev->terminal_ch, ICC_ERR_PARAM);
        return (-1);
    }
#if EMV_LEVEL_1_AUTH
    pSCI_Base->SCI_RXTIME = 0;
#else
    s32Tmp = 1;
    if (0 != pdev->terminal_di)
    {
        //RX timeout is 16 etus.
        s32Tmp = pdev->terminal_fi * 16 / pdev->terminal_di;
        if (s32Tmp > 0xFFFF)
        {
            s32Tmp = 0xFFFF;
        }
        else if (0 == s32Tmp)
        {
            s32Tmp = 1;
        }
    }
    pSCI_Base->SCI_RXTIME = s32Tmp;
#endif    
    //Set communication baudrate.
    pSCI_Param = &aEMV_Param[pdev->terminal_ch];
    pSCI_Base->SCI_BAUD = pSCI_Param->SCIx_BaudReg;
    pSCI_Base->SCI_VALUE = pSCI_Param->SCIx_ValueReg;
    //active time, RST holding low time(ICC clock).
    pSCI_Base->SCI_ATIME = ATR_S_TIME * CLK_DIV;
    //TS waiting time(ICC clock).
    //pSCI_Base->SCI_ATRSTIME = 42372 * CLK_DIV;
    pSCI_Base->SCI_ATRSTIME = ATR_S_TIME * CLK_DIV;
    pSCI_Base->SCI_DTIME = pSCI_Param->SCIx_DeactiveReg;
    //ATR total time(etu).
    pSCI_Base->SCI_ATRDTIME = 19200 + 960;
    //char to char time(etu).
    //pSCI_Base->SCI_CHTIME = 9600 - 12;
    //
    pSCI_Base->SCI_CHTIME = ATR_CH_ETU;
    
    //don't force IO low, when parity error occuered.
    pSCI_Base->SCI_CR0 = CR0_RXNAK;
    //recv mode, ATR duration timeout enable.
    pSCI_Base->SCI_CR1 = CR1_BLKEN | CR1_ATRDEN;
    //RX FIFO is disabled, TX FIFO interrupt is 4.When start to recv ATR(TS), rx FIFO must be 0.
    pSCI_Base->SCI_TIDE = TX_FIFO_THR;
    //Clear tx and rx FIFO
    pSCI_Base->SCI_RXCOUNT = 0x01;
    pSCI_Base->SCI_TXCOUNT = 0x01;
    //Disable send/recv retry in ATR.
    pSCI_Base->SCI_RETRY = 0;
    //Clear the twelve interupts.
    pSCI_Base->SCI_IIR = 0xFF0;

    s32Tmp = ICC_ERR_POWER;
    //if card is not power on yet, we can't active warm reset.
    if (SCICARDUP == (aSCI_BaseTable[pdev->terminal_ch]->SCI_IIR & SCICARDUP))
    {
        s32Tmp = ICC_ERR_NOCARD;
        //card in slot and interrupt flag is set.
        if (SCICARDIN == (aSCI_BaseTable[pdev->terminal_ch]->SCI_IIR & SCICARDIN))
        {
#if (0 == EMV_UNIT_TEST)
            //warm reset card.
            pSCI_Base->SCI_CR2 = CR2_WRESET;
            aEMV_Param[pdev->terminal_ch].stat = EMV_SCI_ACTIVE;
            pSCI_Base->SCI_IER = SCICARDOUT | SCIATRSTOUT | SCIRTOUT | SCICHTOUT | SCIRXTIDE | SCITXERR;

            //alarm_counter++;
            while( EMV_SCI_ACTIVE == aEMV_Param[pdev->terminal_ch].stat);
#else
            pdev->queue->ip = sizeof(ATR_W);
            //copy len + 1 as timeout position.
            memcpy(pdev->queue->qbuf, ATR_W, pdev->queue->ip);
            SET_ERR(pdev->terminal_ch, ICC_ERR_ATR_CWT);
#endif
            return 0;
        }
    }
    SET_ERR(pdev->terminal_ch, s32Tmp);

    return -1;
}

/**
 * power up 
 */
/*Using PL130 power up and active are relatived, so pump is only do some initialize*/ 
int emv_hard_power_pump( struct emv_core *pdev )
{
    uint32_t u32Tmp;

    if (NULL == pdev || pdev->terminal_ch > SLOT_NUM_MAX)
    {
        return -1;
    }

    u32Tmp = pdev->terminal_ch;

    //disable all interrupts.
    aSCI_BaseTable[u32Tmp]->SCI_IER = 0;
    //SCI can't power up until active a cold reset, we should keep power off until cold reset.
    aEMV_Param[pdev->terminal_ch].stat = EMV_SCI_INACTIVE;

    emv_core_init(pdev, (struct emv_queue *)&scs_queue[pdev->terminal_ch]);
    emv_queue_flush(pdev);
    
    if (SCICARDIN != (aSCI_BaseTable[u32Tmp]->SCI_IIR & SCICARDIN))
    {
        return ICC_ERR_NOCARD;
    }
    switch(pdev->terminal_vcc)
    {
        case 1800:
            u32Tmp = 0x03;
        break;
        case 3000:
        default:
            u32Tmp = 0x02;
        break;
    }
    if (0 == pdev->terminal_ch)
    {
        SYSCTRL->ISO7816_CR = (SYSCTRL->ISO7816_CR & ~(0x3 << 6)) | (u32Tmp << 6);
    }
    if (SCICARDDN != (aSCI_BaseTable[pdev->terminal_ch]->SCI_IIR & SCICARDDN))
    {
        return emv_hard_power_dump(pdev);
    }
    return 0;
}

/**
 * executing power down procedure.
 */
int emv_hard_power_dump( struct emv_core *pdev )
{
    uint32_t j;
    SCI_TypeDef * pSCI_Base;
    SCI_EMV_PARAM volatile * pSCI_Param;

    if (NULL == pdev || pdev->terminal_ch > SLOT_NUM_MAX)
    {
        return (-1);
    }

    pSCI_Param = &aEMV_Param[pdev->terminal_ch];
    pSCI_Base = aSCI_BaseTable[pdev->terminal_ch];

    //Disable all SCIx interrupts.
    pSCI_Base->SCI_IER = 0;

    pSCI_Param->stat = EMV_SCI_INACTIVE;
    pSCI_Base->SCI_CR2 = CR2_FINISH;
    
    emv_queue_flush(pdev);
    pdev->terminal_state = EMV_IDLE;
    pdev->terminal_open  = 0;

    while (SCICARDDN != (aSCI_BaseTable[pdev->terminal_ch]->SCI_IIR & SCICARDDN));
    for (j = 0; j < SYSCTRL->PCLK_1MS_VAL / 10; j++);

    SET_ERR(pdev->terminal_ch, ICC_ERR_POWER);
    return 0;
}

//uint32_t isr_flag;
static void SCIx_IRQHandler(uint8_t slot)
{
    uint16_t u16Recv;
    int32_t s32ErrNo = ICC_ERR_OCCUPY;
    SCI_TypeDef * pSCI_Base;
    SCI_EMV_PARAM volatile * pSCI_Param;

    //Get slot param
    pSCI_Param = &aEMV_Param[slot];
    pSCI_Base = aSCI_BaseTable[slot];

    switch (pSCI_Param->stat)
    {
    case EMV_SCI_ACTIVE:
        #define SCI_ACTIVE_ERROR        (SCIATRSTOUT | SCICARDOUT | SCICARDDN)
        //Recv TS in ATR.
        if (0 != (pSCI_Base->SCI_IIR & SCI_ACTIVE_ERROR))
        {
            if (pSCI_Base->SCI_IIR & SCIATRSTOUT)
            {
                s32ErrNo = ICC_ERR_ATR_SWT;
            }
            else if (pSCI_Base->SCI_IIR & SCICARDOUT)
            {
                s32ErrNo = ICC_ERR_NOCARD;
            }
            else if (pSCI_Base->SCI_IIR & SCICARDDN)
            {
                s32ErrNo = ICC_ERR_POWER;
            }
        }
        else if (pSCI_Base->SCI_IIR & (SCIRXTIDE | SCIRTOUT))
        {
            u16Recv = pSCI_Base->SCI_DATA;
            //Set to default direct.
            pSCI_Param->SCIx_Direction = 0;
            if (0x103 == u16Recv || 0x3B == u16Recv)
            {
                if (0x103 == u16Recv)
                {
                    pSCI_Param->SCIx_Direction = 1;
                    pSCI_Base->SCI_CR0 |= CR0_ORDER | CR0_SENSE;
                    u16Recv = 0x3F;
                }
                //Pax's code didn't deal with queue full.
                scs_queue[slot].qbuf[scs_queue[slot].ip++] = u16Recv;
                if (MAX_QBUF_SIZE == scs_queue[slot].ip)
                {
                    scs_queue[slot].ip = 0;
                }

                pSCI_Base->SCI_IER &= ~SCIATRSTOUT;
                pSCI_Base->SCI_IER |= SCIATRDTOUT | SCICHTOUT | SCITXERR;
                if (0 == pSCI_Base->SCI_RXCOUNT) 
                {
                    pSCI_Base->SCI_IIR = SCIRTOUT | SCIRXTIDE;
                }
                pSCI_Param->stat = EMV_SCI_ATR_RECV;
                return;
            }
            s32ErrNo = ICC_ERR_ATR_TS;
        }

        pSCI_Param->stat = EMV_SCI_INACTIVE;
        break;
    case EMV_SCI_ATR_RECV:
        if (pSCI_Base->SCI_IIR & SCICHTOUT)
        {
            s32ErrNo = ICC_ERR_ATR_CWT;
        }
        else if (pSCI_Base->SCI_IIR & SCIATRDTOUT)
        {
            s32ErrNo = ICC_ERR_ATR_TWT;
        }
        else if (pSCI_Base->SCI_IIR & SCICARDOUT)
        {
            s32ErrNo = ICC_ERR_NOCARD;
        }
        else if (pSCI_Base->SCI_IIR & SCITXERR)
        {
            s32ErrNo = ICC_ERR_PARS;
        }
        else if (pSCI_Base->SCI_IIR & (SCIRXTIDE | SCIRTOUT))
        {
            while (pSCI_Base->SCI_RXCOUNT)
            {
                u16Recv = pSCI_Base->SCI_DATA;
                /*ATR期间，不允许出现校验错误*/
                if (0 != (u16Recv & 0x100))
                {
                    s32ErrNo = ICC_ERR_PARS;
                    //pSCI_Base->SCI_TXCOUNT = 0x01;
                    scs_queue[slot].pf = scs_queue[slot].ip;
                    scs_queue[slot].sf = ICC_ERR_PARS;
                }
                
                scs_queue[slot].qbuf[scs_queue[slot].ip++] = u16Recv;
                if(MAX_QBUF_SIZE == scs_queue[slot].ip)
                {
                    scs_queue[slot].ip = 0;
                }
            }
            //if (ICC_ERR_PARS != s32ErrNo)
            {
                pSCI_Base->SCI_IIR = SCIRTOUT | SCIRXTIDE;
                return;
            }
        }
        
        break;
    case EMV_SCI_DATA_TRANS:
        if (CR1_MODE_SEND == (pSCI_Base->SCI_CR1 & CR1_MODE_SEND))
        {
            #define SCI_SEND_ERROR      (SCICARDOUT | SCICARDDN | SCITXERR)
            if (0 != (pSCI_Base->SCI_IIR & SCI_SEND_ERROR))
            {
                if (SCITXERR == (pSCI_Base->SCI_IIR & SCITXERR))
                {
                    //Clear tx FIFO and queue, next time will trigger tx tide interrupt.
                    pSCI_Base->SCI_TXCOUNT = 0x01;
                    //We didn't set this bit in mode T1, so this erro must occur in T0.
                    s32ErrNo = ICC_ERR_SEND;
                }
                else if (SCICARDOUT == (pSCI_Base->SCI_IIR & SCICARDOUT))
                {
                    s32ErrNo = ICC_ERR_NOCARD;
                }
            }
            else if (SCITXTIDE == (pSCI_Base->SCI_IIR & SCITXTIDE))
            {
                if ((scs_queue[slot].ip == scs_queue[slot].op) && (0 == pSCI_Base->SCI_TXCOUNT))
                {
                    //Transmit finished, switch to recv mode and enable block timeout.
                    pSCI_Base->SCI_CR1 &= ~CR1_MODE_SEND;
                    pSCI_Base->SCI_IIR = 0xFF0;
                    pSCI_Base->SCI_IER = SCICARDOUT | SCICHTOUT | SCIBLKTOUT | SCIRTOUT | SCIRXTIDE;
                    FLUSH_QUEUE(slot);
                }
                else
                {
                    while (pSCI_Base->SCI_TXCOUNT < FIFO_DEP)
                    {
                        if( scs_queue[slot].ip != scs_queue[slot].op )/* has remain datas */          
                        {
                            pSCI_Base->SCI_DATA = scs_queue[slot].qbuf[scs_queue[slot].op++];
                            if( MAX_QBUF_SIZE == scs_queue[slot].op )
                            {
                                scs_queue[slot].op = 0;   
                            }
                        }
                        else  //Last character has been put into FIFO.
                        {
                            //Set tx tide threshold value to 1.
                            pSCI_Base->SCI_TIDE = (pSCI_Base->SCI_TIDE & 0xF0) | 0x01;
                            break;
                        }
                    }
                }
                pSCI_Base->SCI_IIR = SCITXTIDE;
                return;
            }
            FLUSH_QUEUE(slot);
        }
        else
        {
            s32ErrNo = pSCI_Base->SCI_IIR;
            if (pSCI_Base->SCI_IIR & SCICHTOUT)
            {
                s32ErrNo = ICC_ERR_CWT;
            }
            else if (pSCI_Base->SCI_IIR & SCIBLKTOUT)
            {
                s32ErrNo = ICC_ERR_BWT;
            }
            else if (pSCI_Base->SCI_IIR & SCITXERR)
            {
                s32ErrNo = ICC_ERR_PARS;
            }
            else if (pSCI_Base->SCI_IIR & (SCIRXTIDE | SCIRTOUT))
            {
                while (pSCI_Base->SCI_RXCOUNT)
                {
                    u16Recv = pSCI_Base->SCI_DATA;
                    if (0 != (u16Recv & 0x100))
                    {
                        SET_ERR(slot, ICC_ERR_PARS);
                    }
                    scs_queue[slot].qbuf[scs_queue[slot].ip++] = u16Recv;
                    if( MAX_QBUF_SIZE == scs_queue[slot].ip )
                    {
                        scs_queue[slot].ip = 0;
                    }
                }
                pSCI_Base->SCI_IIR = SCIRTOUT | SCIRXTIDE;

                return;
            }
        }
        break;
    default:
        break;
    }
    pSCI_Base->SCI_IER = 0;
    pSCI_Base->SCI_IIR = 0xFF0;
    SET_ERR(slot, s32ErrNo);
    pSCI_Base->SCI_CR1 &= ~CR1_MODE_SEND;
}


void SCI0_IRQHandler(void)
{
    SCIx_IRQHandler(0);
}

void SCI2_IRQHandler(void)
{
    SCIx_IRQHandler(2);
}


#if (0 != EMV_UNIT_TEST)
void outputQueue(char *pStr, uint32_t u32Slot)
{
    uint32_t j = 0,  i = scs_queue[u32Slot].op;
    
    DBG_PRINT(pStr);
    while (scs_queue[u32Slot].ip != i)
    {
        DBG_PRINT("%02X ", scs_queue[u32Slot].qbuf[i++]);
        if( MAX_QBUF_SIZE == i )
        {
            i = 0;   
        }
        /*
        if (0 == (++j % 32))
        {
            DBG_PRINT("\n");
        }
        */
    }
    DBG_PRINT("End!\n");
}
#endif


/*
#define CMD1    "\xA4"
#define CMD2    "\x61\x15"
#define CMD3    "\xC0\x00\x82\x00\x00\x10\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x90\x00"
#define CMD4    "\x82"
#define CMD5    "\x62\x83"

static EMV_RECV_CMD gcmdRecv_1738[] = 
{
    {sizeof(CMD1) - 1, CMD1},
    {sizeof(CMD2) - 1, CMD2},
    {sizeof(CMD3) - 1, CMD3},
    {sizeof(CMD4) - 1, CMD4},
    {sizeof(CMD5) - 1, CMD5}
};


#define ATR    "\x3B\xE0\x00\x00\x81\x31\x20\x01\x71"
#define CMD1    "\x00\xE1\x01\xFE\x1E"
#define CMD2    "\x00\x00\x07\xB2\x01\x04\x00\x90\x00\x20\x11"
#define CMD3    "\x00\x00\x07\x00\xB2\x01\x04\x00\x90\x00\x8A"
#define CMD4    "\x00\x91\x00\x91"
#define CMD5    "\x00\x00\x07\x00\xB2\x01\x04\x00\x90\x00\x20"

static EMV_RECV_CMD gcmdRecv[] = 
{
    {sizeof(CMD1), CMD1},
    {sizeof(CMD2), CMD2},
    {sizeof(CMD3), CMD3},
    {sizeof(CMD4), CMD4},
    {sizeof(CMD5), CMD5}
};
*/


/**
 * start hard to transmit
 */
int emv_hard_xmit(struct emv_core *pdev)
{
    uint32_t u32Tmp;
    SCI_TypeDef * pSCI_Base;
    SCI_EMV_PARAM volatile * pSCI_Param;

    if (!(pdev->terminal_ch < SLOT_COUNT))
    {
        return -1;
    }

    pSCI_Param = &aEMV_Param[pdev->terminal_ch];
    pSCI_Base = aSCI_BaseTable[pdev->terminal_ch];

    //outputQueue(pdev->terminal_ch);
    //Disable all interrupts.
    pSCI_Base->SCI_IER = 0;
    if (emv_hard_config(pdev))
    {
        SET_ERR(pdev->terminal_ch, ICC_ERR_PARAM);
        return (-1);
    }

    //Configuarate baudrate.
    pSCI_Base->SCI_BAUD = pSCI_Param->SCIx_BaudReg;
    pSCI_Base->SCI_VALUE = pSCI_Param->SCIx_ValueReg;

    //TX threshold is 4, 8 is full.
    pSCI_Base->SCI_TIDE = (RX_FIFO_THR << 4) | TX_FIFO_THR;
    //Clear tx and rx FIFO
    pSCI_Base->SCI_RXCOUNT = 0x01;
    pSCI_Base->SCI_TXCOUNT = 0x01;
    //RX timeout is 18 etus, in emv test this must be 0.
#if EMV_LEVEL_1_AUTH
    pSCI_Base->SCI_RXTIME = 0;
#else
    u32Tmp = pdev->terminal_fi * 18 / pdev->terminal_di;
    pSCI_Base->SCI_RXTIME = u32Tmp > 0xFFFF ? 0xFFFF : u32Tmp;
#endif    
    //pSCI_Base->SCI_CHGUARD = pdev->terminal_igt;
    //pSCI_Base->SCI_BLKGUARD = pdev->terminal_cgt;

    pSCI_Param->stat = EMV_SCI_DATA_TRANS;

    pSCI_Base->SCI_BLKGUARD = pdev->terminal_igt;
    //Keep direction bit, clear others.
    pSCI_Base->SCI_CR0 &= CR0_SENSE | CR0_ORDER;
    if (1 == pdev->terminal_ptype)
    {
        pSCI_Base->SCI_CHTIME = pdev->terminal_cwt - 10;
        //u32BlkTime = pdev->terminal_bwt - 10;
        pSCI_Base->SCI_BLKTIME = pdev->terminal_bwt - 10;
        pSCI_Base->SCI_CHGUARD = pdev->terminal_cgt - 11;
    }
    else
    {
        pdev->terminal_ptype = 0;
        pSCI_Base->SCI_CHTIME = pdev->terminal_wwt;
        pSCI_Base->SCI_BLKTIME = pdev->terminal_wwt - 12;
        pSCI_Base->SCI_CHGUARD = pdev->terminal_cgt - 12;
        //pSCI_Base->SCI_BLKGUARD = pdev->terminal_igt - 12;
        //dprintf("WWT 0x%08x!\n", pdev->terminal_wwt);
        //0:2 TX retry, 3:5 RX retry.
        pSCI_Base->SCI_RETRY = (5<<3) | 4;
        //pSCI_Base->SCI_RETRY = 4;
        //Enable retransmiting when detect IO low.
        pSCI_Base->SCI_CR0 |= CR0_TXNAK | CR0_RXNAK;
    }
#if (0 == EMV_UNIT_TEST)
    pSCI_Base->SCI_CR1 &= ~CR1_ATRDEN;

    //Block guard time enable, send mode.
    pSCI_Base->SCI_CR1 |= CR1_BGTEN | CR1_MODE_SEND;
    pSCI_Base->SCI_IER = SCICARDOUT | SCITXTIDE | SCITXERR;

    u32Tmp = SCICHTOUT | SCIBLKTOUT;

    while (CR1_MODE_SEND == (pSCI_Base->SCI_CR1 & CR1_MODE_SEND))
    {
        if (0 != (pSCI_Base->SCI_IIR & u32Tmp))
        {
            pSCI_Base->SCI_CR1 &= ~CR1_MODE_SEND;
            SET_ERR(pdev->terminal_ch, ICC_ERR_SEND);
            return -2;
        }
    }
#else
    outputQueue("Send:", 0);
    emv_queue_flush(pdev);
    pdev->queue->ip = gcmdRecv[gu8Step].u32Size;
    //copy len + 1 as timeout position.
    memcpy(pdev->queue->qbuf, gcmdRecv[gu8Step].pu8Str, pdev->queue->ip);
    SET_ERR(pdev->terminal_ch, ICC_ERR_CWT);
    outputQueue("Recv:", 0);
    gu8Step++;
#endif        

    return 0;
}

