/****************************************************************************
 * (C) Skyarchnetworks, Inc. - 2015 - 2016 all rights reserved.
 *
 * Condition to use: (refer to detailed conditions in Japanese)
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
 *
 ****************************************************************************/

#ifndef COMMON_H_
#define COMMON_H_

/*
 * 出力マクロ
 */
#define V_PRINT(...) vfPrintf(&sSerStream,__VA_ARGS__) //!< VERBOSE モード時の printf 出力
#define V_PUTCHAR(c) sSerStream.bPutChar(sSerStream.u8Device, c)  //!< VERBOSE モード時の putchar 出力
#ifdef DEBUG_OUTPUT
#define DBGOUT(lv, ...) if(sAppData.u8DebugLevel >= lv) vfPrintf(&sSerStream, __VA_ARGS__) //!< デバッグ出力
#else
#define DBGOUT(lv, ...) //!< デバッグ出力
#endif

#endif /* COMMON_H_ */
