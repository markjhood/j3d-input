/*
 *	@(#)SerialDevice.c 1.11 02/10/04 20:06:03
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
#include "gameport.h"

/*
 * User level serial driver code for VRlib and Java 3D.
 *
 * Derived 8/25/98 from initial version updated 9/5/92 by Michael F. Deering.
 * Revised 2000 - 2002 for dev use by subsequent Java 3D releases and
 * revised for public com.sun.j3d.input package October 2002 by Mark Hood.
 * No longer requires Solaris Realtime Serial Buffer (RSB) package.
 */

/*
 * Class:     com_sun_j3d_input_SerialDevice
 * Method:    initSerial
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_sun_j3d_input_SerialDevice_initSerial
    (JNIEnv *jenv, jclass jobj) {

    nu_serial_ctx_type *ctx ;
    ctx = (nu_serial_ctx_type *)malloc(sizeof(nu_serial_ctx_type)) ;
    if (ctx == 0) {
	fprintf(stderr, "Error:  cannot malloc serial context\n") ;
	return 0 ;
    }

    memset(ctx, 0, sizeof(nu_serial_ctx_type)) ;
    redbarron_install_peripheral_driver(ctx) ;
    gameport_install_peripheral_driver(ctx) ;

    return (jlong)ctx ;
}

/*
 * Class:     com_sun_j3d_input_SerialDevice
 * Method:    newDevice
 * Signature: (JLjava/lang/String;)I
 * 
 * Create a new instance of the named peripheral device.
 *
 * If no driver with this name is found, it prints an error message and
 * returns failure.
 *
 * If the maximum number of assigned drivers is exceeded, it prints an
 * error message and returns failure.
 *
 * Otherwise the information is recorded in the peripheral_assignments
 * data structure. Note that ALL input string information is copied.
 */
JNIEXPORT jint JNICALL Java_com_sun_j3d_input_SerialDevice_newDevice
    (JNIEnv *jenv, jclass jobj, jlong jin, jstring deviceName) {

    int i, n ;
    nu_serial_ctx_type *ctx ;
    serial_device_substruct *unit ;
    char *peripheral_driver_name ;

    ctx = (nu_serial_ctx_type *)jin ;
    if (ctx == 0) return NULL ;

    peripheral_driver_name =
	(char *) (*jenv)->GetStringUTFChars(jenv, deviceName, NULL) ;

    for (i = 0 ; i < ctx->number_peripheral_drivers ; i++) {
	if (strcmp(peripheral_driver_name,
	    ctx->peripheral_drivers[i].peripheral_driver_name) == 0) {

	    for (n = 0 ; n < MAX_NUMBER_PERIPHERAL_ASSIGNMENTS ; n++) {
		if (ctx->peripheral_assignments[n] == 0)
		    break ;
	    }

	    if (n == MAX_NUMBER_PERIPHERAL_ASSIGNMENTS) {
		fprintf(stderr, "Exceeded max peripheral assignments (%d)\n",
			MAX_NUMBER_PERIPHERAL_ASSIGNMENTS);
		(*jenv)->ReleaseStringUTFChars(jenv, deviceName, NULL) ;
		return -1 ;
	    }

	    unit = ctx->peripheral_drivers[i].create_instance() ;
	    unit->fd = -1 ;
	    unit->peripheral_driver_index = i ;
	    unit->state = SERIAL_DEVICE_STATE_CREATED ;

	    ctx->peripheral_assignments[n] = unit ;

	    (*jenv)->ReleaseStringUTFChars(jenv, deviceName, NULL) ;
	    return n ;  /* success */
	}
    }

    (*jenv)->ReleaseStringUTFChars(jenv, deviceName, NULL) ;
    fprintf(stderr, "Input Peripheral Driver \"%s\" not known\n",
	    peripheral_driver_name) ;

    return -1 ;  /* couldn't find named driver */
}


/*
 * Class:     com_sun_j3d_input_SerialDevice
 * Method:    deviceAttribute
 * Signature: (JIID)I
 */
JNIEXPORT jint JNICALL 
Java_com_sun_j3d_input_SerialDevice_deviceAttribute__JIID
    (JNIEnv *jenv, jobject jobj, jlong jin, jint devin,
     jint attributeNumber, jdouble val) {

    nu_serial_ctx_type *ctx ;
    serial_device_substruct *unit ;
    int pdi, deviceIndex ;

    ctx = (nu_serial_ctx_type *)jin ;
    if (!ctx) return 0 ;

    deviceIndex = (int)devin ;
    if (deviceIndex >= MAX_NUMBER_PERIPHERAL_ASSIGNMENTS) return 0 ;

    unit = ctx->peripheral_assignments[deviceIndex] ;
    if (!unit) return 0 ;

    if (attributeNumber == USE_RSB) {
	unit->use_rsb = (int)val ;
	return 1 ;
    }

    pdi = unit->peripheral_driver_index ;
    return ctx->peripheral_drivers[pdi].device_attribute_double
	(unit, (int)attributeNumber, (double)val) ;
}

/*
 * Class:     com_sun_j3d_input_SerialDevice
 * Method:    deviceAttribute
 * Signature: (JIILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL 
Java_com_sun_j3d_input_SerialDevice_deviceAttribute__JIILjava_lang_String_2
    (JNIEnv *jenv, jobject jobj, jlong jin, jint devin,
     jint attributeNumber, jstring val) {

    nu_serial_ctx_type *ctx ;
    serial_device_substruct *unit ;
    int pdi, rval ;
    char *str ;
    int deviceIndex ;

    str = (char *) (*jenv)->GetStringUTFChars(jenv, val, NULL) ;
    if (!str) return 0 ;

    ctx = (nu_serial_ctx_type *)jin ;
    if (!ctx) return 0 ;

    deviceIndex = (int)devin ;
    if (deviceIndex >= MAX_NUMBER_PERIPHERAL_ASSIGNMENTS) return 0 ;

    unit = ctx->peripheral_assignments[deviceIndex] ;
    if (!unit) return 0 ;

    if (attributeNumber == SERIAL_PORT_FILE_NAME) {
	unit->port_name = strdup(str) ;
	return 1;
    }

    pdi = unit->peripheral_driver_index ;
    rval = ctx->peripheral_drivers[pdi].device_attribute_string
	(unit, (int)attributeNumber, str) ;

    (*jenv)->ReleaseStringUTFChars(jenv, val, NULL) ;
    return rval ;
}

/*
 * Class:     com_sun_j3d_input_SerialDevice
 * Method:    openProbe
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_sun_j3d_input_SerialDevice_openProbe
    (JNIEnv *jenv, jobject jobj, jlong jin) {

    return serial_ports_init_open_probe((nu_serial_ctx_type *)jin) ;
}


/*
 * Class:     com_sun_j3d_input_SerialDevice
 * Method:    close
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_com_sun_j3d_input_SerialDevice_close
    (JNIEnv *jenv, jobject jobj, jlong jin, jint jdev) {

    int pdi, deviceIndex ;
    nu_serial_ctx_type *ctx ;
    serial_device_substruct *unit ;

    deviceIndex = (int)jdev ;
    if (deviceIndex >= MAX_NUMBER_PERIPHERAL_ASSIGNMENTS) return ;

    ctx = (nu_serial_ctx_type *)jin ;
    unit = ctx->peripheral_assignments[deviceIndex] ;
    if (!unit) return ;

    /* call instance-specific close_device() */
    pdi = unit->peripheral_driver_index ;
    ctx->peripheral_drivers[pdi].close_device(unit) ;

    /* close the serial port */
    serial_close(ctx, deviceIndex) ;
}
