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

#ifndef  SLAVE_H_INCLUDED
#define  SLAVE_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include "config.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DEBUG_OUTPUT 1

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/*
 * AppData
 */
typedef struct {
	// DEBUG
	uint8 u8DebugLevel;

	// ネットワーク時の抽象アドレス 0:親機 1~:子機, 0xFF:通信しない
	uint8 u8AppLogicalId;

    // MAC
	uint8 u8channel;
	uint16 u16addr;

    // シーケンス番号
	uint32 u32Seq;

    // 自身の状態 0 or 1
	bool_t mode;

	// 情報LEDを点滅させる(残り)回数
	uint32 remainCntInform;

	// アラートLEDを点滅させる(残り)回数
	uint32 remainCntAlert;

} tsAppData;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#define SWITCH_SHIFT  12 // シフトスイッチ
#define LED_RED        4 // LED赤
#define LED_GLEEN     19 // LED緑
#define LED_YELLOW    18 // LED黄 INFORM用途
#define UART_BAUD 115200 // シリアルのボーレート

#if defined __cplusplus
}
#endif

#endif  /* SLAVE_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
