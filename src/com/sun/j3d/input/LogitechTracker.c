/*
 *	@(#)LogitechTracker.c 1.8 02/10/04 20:04:18
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
#include "redbarron.h"

/*
 * Class:     com_sun_j3d_input_LogitechTracker
 * Method:    getEvents
 * Signature: (JI[D[I)I
 */
JNIEXPORT jint JNICALL Java_com_sun_j3d_input_LogitechTracker_getEvents
    (JNIEnv *jenv, jobject jobj, jlong jin, jint jdev, jdoubleArray jmat,
     jintArray jbuts) {

    nu_serial_ctx_type *ctx ;
    redbarron_unit *unit ;
    track_ctx *t_ctx ;
    matrix_d3d track_to_dig ;
    long i, pdi, temp ;
    int cbuts[4] ;

    ctx = (nu_serial_ctx_type *)jin ;
    unit = (redbarron_unit *)ctx->peripheral_assignments[jdev] ;
    pdi = unit->peripheral_driver_index ;
    t_ctx = (track_ctx *)ctx->peripheral_drivers[pdi].driver_ctx ;
    
    redbarron_obtain_current_raw_events(t_ctx, unit) ;
    i = comp_redbarron_raws(t_ctx, unit, track_to_dig) ;

    if (i != 0)
	(*jenv)->SetDoubleArrayRegion(jenv, jmat, 0, 16, &track_to_dig[0][0]) ;

    if (i != 0 && (temp = redbarron_debounce_buttons(unit)) >= 0) {
	cbuts[0] = (temp&1) == 0 ? 0 : 1 ;
	cbuts[1] = (temp&2) == 0 ? 0 : 1 ;
	cbuts[2] = (temp&4) == 0 ? 0 : 1 ;
	cbuts[3] = (temp&8) == 0 ? 0 : 1 ;
	(*jenv)->SetIntArrayRegion(jenv, jbuts, 0, 4, cbuts) ;
    }

    return i ;
}

