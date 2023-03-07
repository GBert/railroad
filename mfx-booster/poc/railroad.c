/* Various Model Railroad formats
 *
 * Maerklin Motorola timing
 * ------------------------
 * https://www.heise.de/ct/Redaktion/cm/buch/digit_1.html
 * https://www.h0-modellbahner.de/digital-oder-system-mainmenu-82.html
 * https://www.maerklin.de/fileadmin/media/service/technische_informationen/Codiertabelle2.pdf
 * 
 * switch
 * https://www.digital-bahn.de/info_tech/motorola.htm
 *
 * 13us 91us
 * 26us 182us
 *

 * DCC timing
 * ----------
 * 58us 58us
 * 116us 116us  (used)
 * Railcom starts 29us (middle 1 bit) , cut off 4*116us -> 464us
 *

 * mfx timing
 * ----------
 * http://www.skrauss.de/modellbahn/Schienenformat.pdf
 * big endian
 *
 * sync
 *
 * _--_--__-__  or
 * -__-__--_-- 
 *  ^ 
 *  |
 *
 * 50us 50us
 * 100us 100us
 *

 * Selectrix timing
 * ----------------
 * 10us 50us   off plus 
 * 10us 50us   off minus
 */

/* output format for PIO (32bit) 
 * ---------------------
 * 4bit | 12 bit       | 4bit | 12 bit
 * xxxx | yyyyyyyyyyyy | xxxx | yyyyyyyyyyyy
 * pins | time [us]    | pins | delay [us]

 * pins ->
 * left enable | left power | right enable | right power
 * 
 * example mfx 13us high (left) 26us low (right) with power on left and right
 *
 * b 1101 | 0000 0000 1101 | 0111 | 0000 0010 0110
 * x    
 * 
 * 12 bit time will be decreased by delay in PIO programm
 */

#define BIT(x)          (1UL<<x)
/* TODO */
#define BITMASK(x)	((1UL << (x)) - 1UL)



