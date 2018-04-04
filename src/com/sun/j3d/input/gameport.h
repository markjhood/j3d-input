/*
 *	@(#)gameport.h 1.4 02/09/24 02:26:35
 *
 * Copyright (c) 1996-2002 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistribution in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * Neither the name of Sun Microsystems, Inc. or the names of
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * This software is provided "AS IS," without a warranty of any
 * kind. ALL EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND
 * WARRANTIES, INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT, ARE HEREBY
 * EXCLUDED. SUN AND ITS LICENSORS SHALL NOT BE LIABLE FOR ANY DAMAGES
 * SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THE SOFTWARE OR ITS DERIVATIVES. IN NO EVENT WILL SUN
 * OR ITS LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA, OR
 * FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR
 * PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY OF
 * LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE SOFTWARE,
 * EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 *
 * You acknowledge that Software is not designed,licensed or intended
 * for use in the design, construction, operation or maintenance of
 * any nuclear facility.
 */

#include "com_sun_j3d_input_Gameport.h"

#define BUTTON0_MASK 0x10
#define BUTTON1_MASK 0x20
#define BUTTON2_MASK 0x40
#define BUTTON3_MASK 0x80

/*
 *  Event record of raw Gameport event in byte form.
 */
typedef struct  gameport_raw {
    unsigned char sync_byte ;        /* should always be 0 */
    unsigned char buttons ;          /* only upper 4 bits; lower 4 are all 0 */
    unsigned char x1, y1, x2, y2 ;   /* X & Y for sticks 1 & 2 */
} gameport_raw;


/*
 *  Coordinating structure for information about a tracker unit.
 */
typedef struct gameport_unit {
    SERIAL_DEVICE_COMMON_FIELDS ;    /* common fields from serial.h */
    unsigned char current_event[6] ;
    gameport_raw raw_event ;
} gameport_unit;


/*
 * Extern prototypes.
 */
int
gameport_install_peripheral_driver(nu_serial_ctx_type *ctx) ;

int
gameport_obtain_current_raw_events(gameport_unit *unit) ;
