/*
 *	@(#)Gameport.c 1.6 02/09/30 18:41:07
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

#include <stdlib.h>
#include <string.h>
#include "serial.h"
#include "gameport.h"

/*
 * Class:     com_sun_j3d_input_Gameport
 * Method:    getEvents
 * Signature: (JI[D[D[I)I
 */
JNIEXPORT jint JNICALL Java_com_sun_j3d_input_Gameport_getEvents
    (JNIEnv *jenv, jobject jobj, jlong jin, jint jdev,
     jdoubleArray jmat1, jdoubleArray jmat2, jintArray jbuts) {

    double cmat1[2], cmat2[2] ;
    int cbuts[4] = {0, 0, 0, 0} ;

    nu_serial_ctx_type *ctx ;
    gameport_unit *unit ;
    ctx = (nu_serial_ctx_type *)jin ;
    unit = (gameport_unit *)ctx->peripheral_assignments[jdev] ;

    if (! gameport_obtain_current_raw_events(unit))
	return 0 ;  /* no new event yet */

    if (unit->raw_event.buttons & BUTTON0_MASK)
	cbuts[0] = 1 ;
    if (unit->raw_event.buttons & BUTTON1_MASK)
	cbuts[1] = 1 ;
    if (unit->raw_event.buttons & BUTTON2_MASK)
	cbuts[2] = 1 ;
    if (unit->raw_event.buttons & BUTTON3_MASK)
	cbuts[3] = 1 ;

    cmat1[0] = (double)unit->raw_event.x1 ;
    cmat1[1] = (double)unit->raw_event.y1 ;
    cmat2[0] = (double)unit->raw_event.x2 ;
    cmat2[1] = (double)unit->raw_event.y2 ;

    (*jenv)->SetIntArrayRegion(jenv, jbuts, 0, 4, cbuts) ;
    (*jenv)->SetDoubleArrayRegion(jenv, jmat1, 0, 2, cmat1) ;
    (*jenv)->SetDoubleArrayRegion(jenv, jmat2, 0, 2, cmat2) ;

    return 1 ;
}
