/*
 *      @(#)serial.h 1.16 02/10/04 20:05:19
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

#include <stdio.h>
#include <sys/time.h>
#include "com_sun_j3d_input_SerialDevice.h"

#ifdef DEBUG
#define DPRINT(args) printf args
#else
#define DPRINT(args)
#endif

typedef enum {
    USE_RSB =
    com_sun_j3d_input_SerialDevice_USE_RSB,
    SERIAL_PORT_FILE_NAME =
    com_sun_j3d_input_SerialDevice_SERIAL_PORT_FILE_NAME
} serial_attributes ;

typedef enum {
    SERIAL_DEVICE_STATE_NULL,
    SERIAL_DEVICE_STATE_CREATED,
    SERIAL_DEVICE_STATE_OPENED,
    SERIAL_DEVICE_STATE_CLOSED
} serial_device_states ;

/*
 * Struct of memory ttys for special shared memory RSB ztty driver (qzs & no
 * std zs).  Also used when RSB is not available; in that case, read(2) is
 * called to read character data from the tty port and write into this
 * struct to emulate the RSB.  This is done for backward compatibility with
 * the existing code base.
 */
typedef struct  mtty {
    unsigned char abuf[2044] ;
    unsigned char bbuf[2044] ;
    int           a_off ;
    int           b_off ;
} mtty ;

#define SERIAL_DEVICE_COMMON_FIELDS                            \
    int peripheral_driver_index ;                              \
    serial_device_states state ;                               \
    char *port_name ;                                          \
    int baud ;                                                 \
    int needs_rts_and_dtr ;                                    \
    int fd ;                                                   \
    int use_rsb ;   /* set by RealtimeSerialBuffer property */ \
    mtty *ztty ;    /* ptr to ztty, may be shared memory */    \
    int ztty_buf ;  /* 0 means use abuf, 1 means use bbuf */   \
    int ztty_last   /* previous position in the ztty buffer */ \

/*
 * This struct is extended by the peripheral driver implementations.
 */
typedef struct serial_device_substruct {
    SERIAL_DEVICE_COMMON_FIELDS ;
} serial_device_substruct ;

#define MAX_NUMBER_PERIPHERAL_DRIVERS 16
#define MAX_NUMBER_PERIPHERAL_ASSIGNMENTS 32

typedef struct peripheral_driver {
    /* string identifying this peripheral driver */
    char *peripheral_driver_name ;

    /* driver-specific data area for all driver's devices */
    void *driver_ctx ;

    /* array of references to devices that can be initialized in parallel */
    serial_device_substruct *affinities[MAX_NUMBER_PERIPHERAL_ASSIGNMENTS] ;
    int affinity_count ;

    /* pointers to functions implementing the driver */
    void (*reset_device)() ;
    int  (*probe_device)() ;
    void (*reset_device_array)() ;      /* null if affinities not handled */
    int  (*probe_device_array)() ;      /* null if affinities not handled */
    void (*close_device)() ;
    int  (*device_attribute_double)() ;
    int  (*device_attribute_string)() ;
    serial_device_substruct *(*create_instance)() ;

} peripheral_driver;


typedef struct nu_serial_ctx_type {
    int number_peripheral_drivers ;

    peripheral_driver
        peripheral_drivers[MAX_NUMBER_PERIPHERAL_DRIVERS] ;

    serial_device_substruct *
        peripheral_assignments[MAX_NUMBER_PERIPHERAL_ASSIGNMENTS] ;

} nu_serial_ctx_type ;

typedef double matrix_d3d[4][4];  /* 3D 64-bit floating-point matrix */


/*
 * Extern prototypes.
 */
int
serial_ports_init_open_probe(nu_serial_ctx_type *ctx) ;

void
serial_read(serial_device_substruct *unit) ;

void
serial_command(serial_device_substruct *units[], int count,
               int pre_sleep, int post_sleep,
               unsigned char *command);

void
serial_close(nu_serial_ctx_type *ctx, int deviceIndex) ;
