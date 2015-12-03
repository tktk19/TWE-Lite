/****************************************************************************
 * (C) Skyarchnetworks, Inc. - 2015 - 2016 all rights reserved.
 *
 * Condition to use:
 *   - The full or part of source code is limited to use for TWE (TOCOS
 *     Wireless Engine) as compiled and flash programmed.
 *   - The full or part of source code is prohibited to distribute without
 *     permission from TOCOS.
 *
 * 利用条件:
 *   - 本ソースコードは、別途ソースコードライセンス記述が無い限り東京コスモス電機が著作権を
 *     保有しています。
 *   - 本ソースコードは、無保証・無サポートです。本ソースコードや生成物を用いたいかなる損害
 *     についても東京コスモス電機は保証致しません。不具合等の報告は歓迎いたします。
 *   - 本ソースコードは、東京コスモス電機が販売する TWE シリーズ上で実行する前提で公開
 *     しています。他のマイコン等への移植・流用は一部であっても出来ません。
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <string.h>

#include <jendefs.h>
#include <AppHardwareApi.h>

#include "utils.h"

#include "SavacanSwitch.h"

#include "common.h"
#include "config.h"

#include "Version.h"

// DEBUG options

#include "serial.h"
#include "fprintf.h"
#include "sprintf.h"

/****************************************************************************/
/***        ToCoNet Definitions                                           ***/
/****************************************************************************/
// Select Modules (define befor include "ToCoNet.h")
//#define ToCoNet_USE_MOD_NBSCAN // Neighbour scan module
//#define ToCoNet_USE_MOD_NBSCAN_SLAVE

// includes
#include "ToCoNet.h"
#include "ToCoNet_mod_prototype.h"

#include "app_event.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg);

static void vInitHardware(int f_warm_start);

void vSerialInit(uint32 u32Baud, tsUartOpt *pUartOpt);
static void vHandleSerialInput(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Version/build information. This is not used in the application unless we
   are in serial debug mode. However the 'used' attribute ensures it is
   present in all binary files, allowing easy identifaction... */

/* Local data used by the tag during operation */
static tsAppData sAppData;

PUBLIC tsFILE sSerStream;
tsSerialPortSetup sSerPort;

// Wakeup port
const uint32 u32DioPortWakeUp = 1UL << 7; // UART Rx Port

/****************************************************************************
 *
 * NAME: AppColdStart
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
void cbAppColdStart(bool_t bAfterAhiInit)
{
	//static uint8 u8WkState;
	if (!bAfterAhiInit) {
		// before AHI init, very first of code.

		// Register modules
		ToCoNet_REG_MOD_ALL();

	} else {
		// disable brown out detect
		vAHI_BrownOutConfigure(0,//0:2.0V 1:2.3V
				FALSE,
				FALSE,
				FALSE,
				FALSE);

		// clear application context
		memset (&sAppData, 0x00, sizeof(sAppData));
		sAppData.u8channel = CHANNEL;
		sAppData.mode = FALSE;

		// ToCoNet configuration
		sToCoNet_AppContext.u32AppId = APP_ID;
		sToCoNet_AppContext.u8Channel = CHANNEL;

		sToCoNet_AppContext.bRxOnIdle = TRUE;

		// others
		SPRINTF_vInit128();

		// Register
		ToCoNet_Event_Register_State_Machine(vProcessEvCore);

		// Others
		vInitHardware(FALSE);

		// MAC start
		ToCoNet_vMacStart();
	}
}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
static bool_t bWakeupByButton;

void cbAppWarmStart(bool_t bAfterAhiInit)
{
	if (!bAfterAhiInit) {
		// before AHI init, very first of code.
		//  to check interrupt source, etc.
		bWakeupByButton = FALSE;

		if(u8AHI_WakeTimerFiredStatus()) {
			// wake up timer
		} else
		if(u32AHI_DioWakeStatus() & u32DioPortWakeUp) {
			// woke up from DIO events
			bWakeupByButton = TRUE;
		} else {
			bWakeupByButton = FALSE;
		}
	} else {
		// Initialize hardware
		vInitHardware(TRUE);

		// MAC start
		ToCoNet_vMacStart();
	}
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vMain
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
void cbToCoNet_vMain(void)
{
	static unsigned char stsPrev = 0xFF;
	bool_t sts = bPortRead(SWITCH_SHIFT);

	// スイッチが押される度に自身のモードを切り替え
	if (sts != stsPrev) {
		stsPrev = sts;
		// スイッチの押し終わりを検出
		if (bPortRead(SWITCH_SHIFT)) {
			sAppData.mode = !sAppData.mode;
			vPortSet_TrueAsLo(LED_RED, sAppData.mode);
			vPortSet_TrueAsLo(LED_GLEEN, !sAppData.mode);
		}
		DBGOUT(1, LB "SWITCH CHANGED -> %d", bPortRead(sts));
	}

	/* handle uart input */
	vHandleSerialInput();
}

/****************************************************************************
 *
 * NAME: cbToCoNet_vNwkEvent
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
void cbToCoNet_vNwkEvent(teEvent eEvent, uint32 u32arg) {
	switch(eEvent) {
	default:
		break;
	}
}

/****************************************************************************
 *
 * NAME: cbvMcRxHandler
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
void cbToCoNet_vRxEvent(tsRxDataApp *pRx) {
	int i;
	static uint32 u32SrcAddrPrev = 0;
	static uint16 u16seqPrev = 0xFFFF;
	//uint8 *p = pRx->auData;

	// print coming payload
	DBGOUT(1, LB"[PKT Ad:%04x,Ln:%03d,Seq:%03d,Lq:%03d,Tms:%05d \"",
			pRx->u32SrcAddr,
			pRx->u8Len+4, // Actual payload byte: the network layer uses additional 4 bytes.
			pRx->u8Seq,
			pRx->u8Lqi,
			pRx->u32Tick & 0xFFFF);
	for (i = 0; i < pRx->u8Len; i++) {
		if (i < 32) {
			sSerStream.bPutChar(sSerStream.u8Device,
					(pRx->auData[i] >= 0x20 && pRx->auData[i] <= 0x7f) ? pRx->auData[i] : '.');
		} else {
			V_PRINT("..");
			break;
		}
	}
	V_PRINT("C\"]");

	// 前回と同一の送信元＋シーケンス番号のパケットなら受け流す
	if (pRx->u32SrcAddr == u32SrcAddrPrev && pRx->u8Seq == u16seqPrev) {
		return;
	}

	u32SrcAddrPrev = pRx->u32SrcAddr;
	u16seqPrev = pRx->u8Seq;


	//TODO pRx->auData変換がうまくいかない
	// ペイロードを切り出してデバッグ出力
	/*
	char rcvCode[2];
	int len = (pRx->u8Len < sizeof(rcvCode)) ? pRx->u8Len : sizeof(rcvCode)-1;
	memcpy(rcvCode, pRx->auData, len);
	rcvCode[len] = '\0';

	DBGOUT(1, LB "%d", (int)rcvCode);

	switch ((int)rcvCode) {
	case VAL_INFORMATION_NORM :
		// informLEDを点滅させる
		sAppData.timestampInformFrom = u32TickCount_ms;
	break;

	case VAL_EMERG :
		// alertLEDを点滅させる
		sAppData.timestampAlertFrom = u32TickCount_ms;
	break;
	}
	*/

	// informLEDを点滅させる
	sAppData.timestampInformFrom = u32TickCount_ms;

	// UARTに出力
	DBGOUT(1, LB "Message from %08x" LB, pRx->u32SrcAddr);
}

/****************************************************************************
 *
 * NAME: cbvMcEvTxHandler
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
void cbToCoNet_vTxEvent(uint8 u8CbId, uint8 bStatus) {
	return;
}

/****************************************************************************
 *
 * NAME: cbToCoNet_vHwEvent
 *
 * DESCRIPTION:
 * Process any hardware events.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u32DeviceId
 *                  u32ItemBitmap
 *
 * RETURNS:
 * None.
 *
 * NOTES:
 * None.
 ****************************************************************************/
void cbToCoNet_vHwEvent(uint32 u32DeviceId, uint32 u32ItemBitmap)
{
    switch (u32DeviceId) {
    case E_AHI_DEVICE_TICK_TIMER:

		// InformLED BLINK
    	if (sAppData.timestampInformFrom != 0 && u32TickCount_ms - sAppData.timestampInformFrom < BLINK_TIME_INFORM) {
    		vPortSet_TrueAsLo(LED_YELLOW, u32TickCount_ms & BLINK_SPEED_INFORM);
    	} else {
    		sAppData.timestampInformFrom = 0;
    		vPortSetHi(LED_YELLOW);
    	}

    	// AlertLED BLINK
    	if (sAppData.timestampAlertFrom != 0 && u32TickCount_ms - sAppData.timestampAlertFrom < BLINK_TIME_ALERT) {
    		vPortSet_TrueAsLo(LED_RED, u32TickCount_ms & BLINK_SPEED_ALERT);
    	} else {
    		sAppData.timestampAlertFrom = 0;
    		// モードと共有しているため sAppData.modeの値に応じて変更
    		if (sAppData.mode) {
    			vPortSetLo(LED_RED);
    		} else {
        		vPortSetHi(LED_RED);
    		}
    	}

    	break;

    default:
    	break;
    }
}

/****************************************************************************
 *
 * NAME: cbToCoNet_u8HwInt
 *
 * DESCRIPTION:
 *   called during an interrupt
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u32DeviceId
 *                  u32ItemBitmap
 *
 * RETURNS:
 *                  FALSE -  interrupt is not handled, escalated to further
 *                           event call (cbToCoNet_vHwEvent).
 *                  TRUE  -  interrupt is handled, no further call.
 *
 * NOTES:
 *   Do not put a big job here.
 ****************************************************************************/
uint8 cbToCoNet_u8HwInt(uint32 u32DeviceId, uint32 u32ItemBitmap) {
	return FALSE;
}

/****************************************************************************
 *
 * NAME: vInitHardware
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
static void vInitHardware(int f_warm_start)
{
	// Serial Initialize
#if 0
	// UART の細かい設定テスト
	tsUartOpt sUartOpt;
	memset(&sUartOpt, 0, sizeof(tsUartOpt));
	sUartOpt.bHwFlowEnabled = FALSE;
	sUartOpt.bParityEnabled = E_AHI_UART_PARITY_ENABLE;
	sUartOpt.u8ParityType = E_AHI_UART_EVEN_PARITY;
	sUartOpt.u8StopBit = E_AHI_UART_2_STOP_BITS;
	sUartOpt.u8WordLen = 7;

	vSerialInit(UART_BAUD, &sUartOpt);
#else
	vSerialInit(UART_BAUD, NULL);
#endif

	ToCoNet_vDebugInit(&sSerStream);
	ToCoNet_vDebugLevel(0);

	/// IOs
	// Input
	vPortAsInput(SWITCH_SHIFT);

	// Output
	vPortAsOutput(LED_RED);
	vPortAsOutput(LED_YELLOW);
	vPortAsOutput(LED_GLEEN);

	vPortSetHi(LED_RED);
	vPortSetHi(LED_YELLOW);
	vPortSetLo(LED_GLEEN);

}

/****************************************************************************
 *
 * NAME: vInitHardware
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
void vSerialInit(uint32 u32Baud, tsUartOpt *pUartOpt) {
	/* Create the debug port transmit and receive queues */
	static uint8 au8SerialTxBuffer[96];
	static uint8 au8SerialRxBuffer[32];

	/* Initialise the serial port to be used for debug output */
	sSerPort.pu8SerialRxQueueBuffer = au8SerialRxBuffer;
	sSerPort.pu8SerialTxQueueBuffer = au8SerialTxBuffer;
	sSerPort.u32BaudRate = u32Baud;
	sSerPort.u16AHI_UART_RTS_LOW = 0xffff;
	sSerPort.u16AHI_UART_RTS_HIGH = 0xffff;
	sSerPort.u16SerialRxQueueSize = sizeof(au8SerialRxBuffer);
	sSerPort.u16SerialTxQueueSize = sizeof(au8SerialTxBuffer);
	sSerPort.u8SerialPort = UART_PORT_SLAVE;
	sSerPort.u8RX_FIFO_LEVEL = E_AHI_UART_FIFO_LEVEL_1;
	SERIAL_vInitEx(&sSerPort, pUartOpt);

	sSerStream.bPutChar = SERIAL_bTxChar;
	sSerStream.u8Device = UART_PORT_SLAVE;
}

/****************************************************************************
 *
 * NAME: vHandleSerialInput
 *
 * DESCRIPTION:
 *
 * PARAMETERS:      Name            RW  Usage
 *
 * RETURNS:
 *
 * NOTES:
 ****************************************************************************/
static void vHandleSerialInput(void)
{
    // handle UART command
	while (!SERIAL_bRxQueueEmpty(sSerPort.u8SerialPort)) {
		int16 i16Char;

		i16Char = SERIAL_i16RxChar(sSerPort.u8SerialPort);

		V_PRINT(LB "# [%c] --> ", i16Char);
	    SERIAL_vFlush(sSerStream.u8Device);

		switch(i16Char) {

		case 'd':
			_C {
				sAppData.u8DebugLevel++;
				if (sAppData.u8DebugLevel > 5)
					sAppData.u8DebugLevel = 0;

				V_PRINT("set App debug level to %d." LB, sAppData.u8DebugLevel);
			}
			break;

		case 'p':
			// 出力調整のテスト
			_C {
				static uint8 u8pow = 3; // (MIN)0..3(MAX)

				u8pow = (u8pow + 1) % 4;
				V_PRINT("set power to %d.", u8pow);

				sToCoNet_AppContext.u8TxPower = u8pow;
				ToCoNet_vRfConfig();
			}
			break;

		case 'i': case 'e': // Information / Emergency を送信
			_C {
				// transmit Ack back
				tsTxDataApp tsTx;
				memset(&tsTx, 0, sizeof(tsTxDataApp));

				sAppData.u32Seq++;

				tsTx.u32SrcAddr = ToCoNet_u32GetSerial(); // 自身のアドレス
				tsTx.u32DstAddr = 0xFFFF; // ブロードキャスト

				tsTx.bAckReq = FALSE;
				tsTx.u8Retry = 0x82; // ブロードキャストで都合３回送る
				tsTx.u8CbId  = sAppData.u32Seq & 0xFF;
				tsTx.u8Seq   = sAppData.u32Seq & 0xFF;
				tsTx.u8Cmd   = TOCONET_PACKET_CMD_APP_DATA;

				// SPRINTF でメッセージを作成
				SPRINTF_vRewind();
				if (i16Char == 'i') {
					vfPrintf(SPRINTF_Stream, "%d", VAL_INFORMATION_NORM);
				} else if (i16Char == 'e') {
					vfPrintf(SPRINTF_Stream, "%d", VAL_EMERG);
				}
				memcpy(tsTx.auData, SPRINTF_pu8GetBuff(), SPRINTF_u16Length());
				tsTx.u8Len = SPRINTF_u16Length();

				// 送信
				ToCoNet_bMacTxReq(&tsTx);

				// UARTに出力
				if (i16Char == 'i') {
					DBGOUT(1, LB "INFORMATION REQUEST SENT CODE = 10");
				} else if (i16Char == 'e') {
					DBGOUT(1, LB "EMERGENCY REQUEST SENT CODE = 99");
				}
			}
			break;

		default:
			break;
		}

		DBGOUT(1, LB);
	    SERIAL_vFlush(sSerStream.u8Device);
	}
}

/****************************************************************************
 *
 * NAME: vProcessEvent
 *
 * DESCRIPTION:
 *
 * RETURNS:
 *
 ****************************************************************************/
static void vProcessEvCore(tsEvent *pEv, teEvent eEvent, uint32 u32evarg) {
	if (eEvent == E_EVENT_START_UP) {
		// 起動


	}
}


/** @ingroup MASTER
 * IO 情報を送信します。
 *
 * - IO状態の変化、および１秒置きの定期送時に呼び出されます。
 *
 * - Packet 構造
 *   - BE_DWORD: 送信元のシリアル番号
 *   - OCTET: 宛先論理ID
 *   - BE_WORD: 送信タイムスタンプ (64fps カウンタの値の下１６ビット, 約1000秒で１周する)
 *   - OCTET: 中継フラグ(中継したパケットは１、最初の送信なら０を格納）
 *   - BE_WORD: 電圧
 *   - OCTET: 温度 (int8型)  ※ TODO: 値が不正確。ADC 値から温度への変換式がメーカより開示されないため。
 *   - OCTET: ボタン (LSB から順に SW1 ... SW4, 1=Lo), 0x80ビットは通常送信の識別用フラグ
 *   - OCTET: ボタン変化 (LSB から順に SW1 ... SW4, 1=変化)
 *
 * @returns -1:ERROR, 0..255 CBID
 */
static int16 i16TransmitIoData(bool_t bQuick, bool_t bRegular) {
	int16 i16Ret = -1;
	tsTxDataApp sTx;
	memset(&sTx, 0, sizeof(sTx));

	uint8 *q = sTx.auData;

	// ペイロードを構成
	S_OCTET(sAppData.mode); // 自身のモードを親機に送信

	{
		/* MAC モードでは細かい指定が可能 */
		sTx.bAckReq     = FALSE;
		sTx.u32SrcAddr  = sToCoNet_AppContext.u16ShortAddress;
		sTx.u16RetryDur = bQuick ? 0 : 4;  // 再送間隔
		sTx.u16DelayMax = bQuick ? 0 : 16; // 衝突を抑制するため送信タイミングにブレを作る(最大16ms)

		// 送信API
		if (ToCoNet_bMacTxReq(&sTx)) {
			i16Ret = sTx.u8CbId;
		}
	}

	return i16Ret;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
