/*
 * odt_modules.h
 *
 *  Created on: Nov 7, 2024
 *      Author: katarinac
 */

#ifndef __ODT_MODULES_H
#define __ODT_MODULES_H

#include "../haos/haos.h"

extern HAOS_Mif_t Module_mif;
extern HAOS_Mif_t FIR_mif;
extern HAOS_Mif_t MultitapDelay_mif;
extern HAOS_Mif_t upMixer_mif;
HAOS_Odt_t odt =
{
		//{&upMixer_mif, 0x15},
		//{&TestModule_mif, 0x30},
		//{&FIR_mif, 0x40},
		//{&MultitapDelay_mif, 0x50},
		{&Module_mif, 0x70},
		{0, 0} // null entry terminates the table of modules
};

#endif /* __ODT_MODULES_H */
