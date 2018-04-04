/*
 *	@(#)TrackdInputDevice.c 1.8 02/08/15 19:04:54
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

#ifdef SOLARIS
#include <dlfcn.h>
/*
 * The following symbols are defined by libtrackdAPI.so.  Since this code may
 * be a part of a native library that is loaded even when the
 * TrackdInputDevice implementation is not used and libtrackdAPI.so is not
 * available, they must be defined as weak symbols to prevent fatal errors
 * from the runtime linker.
 */
#pragma weak trackdInitTrackerReader
#pragma weak trackdInitControllerReader
#pragma weak trackdGetNumberOfButtons
#pragma weak trackdGetNumberOfSensors
#pragma weak trackdGetNumberOfValuators
#pragma weak trackdGetButton
#pragma weak trackdGetValuator
#pragma weak trackdGetMatrix
#endif

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#endif

#include "trackdAPI.h"
#include "com_sun_j3d_input_TrackdInputDevice.h"

#define TRACKER_INIT_ERR    0x1
#define CONTROLLER_INIT_ERR 0x2
#define LIB_INIT_ERR        0x4
#define FEET_TO_METERS      0.3048f

static void *tracker ;
static void *controller ;

/*
 * Class:     com_sun_j3d_input_TrackdInputDevice
 * Method:    initTrackd
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_sun_j3d_input_TrackdInputDevice_initTrackd
    (JNIEnv *env, jobject obj, jint trackerShmKey, jint controllerShmKey) {

    jint status = 0 ;

#ifdef SOLARIS
    /*
     * The native code link is performed with the -z lazyload option. Check to
     * see if trackdInitTrackerReader is defined.  Since it is a weak symbol
     * this native method would fail silently if libtrackdAPI.so was not
     * available.  Return an error status instead.
     */
    char *sym = "trackdInitTrackerReader" ;
    if (! dlsym(RTLD_DEFAULT, sym)) {
	char *err ;
	fprintf(stderr, "dlsym() returned null for %s\n", sym) ;
	if ((err = dlerror()) != 0)
	    fprintf(stderr, "%s\n", err) ;
	else
	    fprintf(stderr, "no error message available\n") ;

	return LIB_INIT_ERR ;
    }
#endif

#ifdef WIN32
    /*
     * The native code link is performed with the /DELAYLOAD option.  See if
     * trackdAPI_MT.dll can be loaded here at runtime.  If not, return an
     * error status instead of an access violation.
     */
    char *libname = "trackdAPI_MT" ;
    HMODULE trackdlib = LoadLibrary(libname) ;

    if (trackdlib == NULL) {
	LPVOID lpMsgBuf ;
	DWORD err = GetLastError() ;
	fprintf(stderr, "LoadLibrary() returned null\n") ;
	fprintf(stderr, "library name: %s\n", libname) ;
	fprintf(stderr, "GetLastError() returned 0x%x (%d)\n", err, err) ;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		      FORMAT_MESSAGE_FROM_SYSTEM | 
		      FORMAT_MESSAGE_IGNORE_INSERTS,
		      NULL, err,
		      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		      (LPTSTR) &lpMsgBuf, 0, NULL) ;
	fprintf(stderr, (LPCTSTR)lpMsgBuf) ;
	LocalFree(lpMsgBuf) ;

	return LIB_INIT_ERR ;    
    }
#endif

    tracker = (void *)trackdInitTrackerReader(trackerShmKey) ;
    controller = (void *)trackdInitControllerReader(controllerShmKey) ;

    if (tracker == NULL)
	status |= TRACKER_INIT_ERR ;

    if (controller == NULL)
	status |= CONTROLLER_INIT_ERR ;

    return status ;
}

/*
 * Class:     com_sun_j3d_input_TrackdInputDevice
 * Method:    getButtonCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_sun_j3d_input_TrackdInputDevice_getButtonCount
    (JNIEnv *env, jobject obj) {

    return trackdGetNumberOfButtons(controller) ;
}


/*
 * Class:     com_sun_j3d_input_TrackdInputDevice
 * Method:    getTrackerCount
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_sun_j3d_input_TrackdInputDevice_getTrackerCount
    (JNIEnv *env, jobject obj) {

    return trackdGetNumberOfSensors(tracker) ;
}

/*
 * Class:     com_sun_j3d_input_TrackdInputDevice
 * Method:    getValuatorCount
 * Signature: ()I
 */
JNIEXPORT jint 
JNICALL Java_com_sun_j3d_input_TrackdInputDevice_getValuatorCount
    (JNIEnv *env, jobject obj) {

    return trackdGetNumberOfValuators(controller) ;
}


/*
 * Class:     com_sun_j3d_input_TrackdInputDevice
 * Method:    getButton
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_sun_j3d_input_TrackdInputDevice_getButton
    (JNIEnv *env, jobject obj, jint buttonIndex) {

    return trackdGetButton(controller, buttonIndex) ;
}

/*
 * Class:     com_sun_j3d_input_TrackdInputDevice
 * Method:    getValuator
 * Signature: (I)F
 */
JNIEXPORT jfloat JNICALL Java_com_sun_j3d_input_TrackdInputDevice_getValuator
    (JNIEnv *env, jobject obj, jint valuatorIndex) {

    return (jfloat)trackdGetValuator(controller, valuatorIndex) ;
}

/*
 * Class:     com_sun_j3d_input_TrackdInputDevice
 * Method:    getMatrix
 * Signature: ([FI)V
 */
JNIEXPORT void JNICALL Java_com_sun_j3d_input_TrackdInputDevice_getMatrix
    (JNIEnv *env, jobject obj, jfloatArray jMatrix, jint trackerIndex) {

    float m[4][4] ;
    jfloat *result ;
    trackdGetMatrix(tracker, trackerIndex, m) ;
    result = (jfloat *)(*env)->GetPrimitiveArrayCritical(env, jMatrix, 0) ;

    /*
     * Copy and transpose the matrix returned by Trackd.  The translation
     * components are scaled from feet to meters.  The projection components
     * should always be {0, 0, 0, 1}.
     */
    result[0]  = m[0][0] ;
    result[1]  = m[1][0] ;
    result[2]  = m[2][0] ;
    result[3]  = m[3][0] * FEET_TO_METERS ;
    result[4]  = m[0][1] ;
    result[5]  = m[1][1] ;
    result[6]  = m[2][1] ;
    result[7]  = m[3][1] * FEET_TO_METERS ;
    result[8]  = m[0][2] ;
    result[9]  = m[1][2] ;
    result[10] = m[2][2] ;
    result[11] = m[3][2] * FEET_TO_METERS ;

    (*env)->ReleasePrimitiveArrayCritical(env, jMatrix, result, 0) ;
}




