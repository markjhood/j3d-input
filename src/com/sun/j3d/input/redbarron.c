/*
 *	@(#)redbarron.c 1.26 02/10/17 16:00:44
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

#include <ieeefp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/termios.h>
#include <sys/filio.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <stropts.h>
#include "serial.h"
#include "redbarron.h"

/*
 * Low level driver code for Logitech Red Barron 6DOF tracker for Java3d.
 *
 * Derived 8/25/98 from initial version updated 9/5/92 by Michael F. Deering.
 * Revised 2000 - 2002 for dev use by subsequent Java 3D releases and
 * revised for public com.sun.j3d.input package October 2002 by Mark Hood.
 */

/*
 *  Logitech 6D mouse (red barron) command codes.
 */
static unsigned char RB_GO_INTO_6D[] = "*>" ;
static unsigned char RB_DIGITIZE[] = "*d" ;
static unsigned char RB_HEADTRACK_MODE[] = "*H" ;
static unsigned char RB_LEAVE_HEADTRACK_MODE[] = "*h" ;
static unsigned char RB_STREAMING[] = "*S" ;
static unsigned char RB_DEMAND_REPORTING[] = "*D" ;
static unsigned char RB_RESET[] = "*R" ;
static unsigned char RB_GLOBAL_EULER[] = "*G" ;
static unsigned char RB_CUR_OP_INFO[] = "*m" ;
static unsigned char RB_ENABLE_TRANSMITTER_OUTPUT[] = "*A" ;
static unsigned char RB_DISABLE_TRANSMITTER_OUTPUT[] = "*O" ;
static unsigned char RB_RAW_MODE[] = { 0x2a, 0xf2, 0 } ;
static unsigned char RB_TACTILE[] = { '!', 'T', 0x1, 0x1, 0x7 } ;
static unsigned char RB_SET_EVENT_FILTER_LENGTH[] = 
    { '*', '$', 0x2, 0x7, 1,  0 } ;
static unsigned char RB_SET_SLAVE_XMITER_TYPE[] = 
    { 0x2A, 0x24, 0x2, 0x1, 1,  0 } ;


/*
 *  This routine mast be called once at the beginning of running to
 *  initialize the time management state.
 *
 *  Specifically, the pworld time within the package is defined to
 *  be the real-time since this init call was made. The problem with
 *  using the real real time is that the floating point numbers may
 *  be too big for certain accuracy, and also makes debugging hard.
 *  eventually, some form of 256 bit origin may be used for time
 *  as well.
 *
 *  This routine sets the value of pworld time_zero to the current time
 *  as returned by gethrtime(); afterwards pworld time will be defined as
 *  what gethrtime() returns MINUS this starting value.
 */
static void
t_init_pworld_time(track_ctx *t_ctx)
{
    hrtime_t	tp;

    /*
     *  Time managment.
     *  Get the time as realtime in nanoseconds (as a 64 bit int (long long)).
     *  Convert to floating point in units of seconds.
     */
    tp = gethrtime();
    t_ctx->long_long_pworld_time_zero = tp ;
    tp = tp>>10;  /* shift to avoid overflow of double */

    t_ctx->t_pworld_time.time_zero = tp;
    /* 1 Billion nanoseconds in a sec, minus >>10 */
    t_ctx->t_pworld_time.time_zero /= 1000000000.0/1024.0;

    /* Some initial values for the pworld time and delta.  These may
       have to be adjusted someday to make other init stuff start out OK. */
    t_ctx->t_pworld_time.last_time = -0.3;
    t_ctx->t_pworld_time.time = 0.0;
    t_ctx->t_pworld_time.delta_time = 0.3;
    t_ctx->time_to_meters = 0.00005549 ;  /* typical value 0.00005549 */
}  /* end of t_init_pworld_time */



/*
 *  Update the current pworld time values.
 */
static void
t_sync_pworld_time(track_ctx *t_ctx)
{
    hrtime_t	tp;

    /*
     *  Time managment.
     *  Get the time as realtime in nanoseconds (as a 64 bit int (long long)).
     *  Convert to floating point in units of seconds.
     */
    t_ctx->t_pworld_time.last_time = t_ctx->t_pworld_time.time;

    tp = gethrtime();
    tp -= t_ctx->long_long_pworld_time_zero; /* high accuracy time zero sub */
    tp = tp>>10;  /* shift to avoid overflow of double */

    t_ctx->t_pworld_time.time = tp;
    /* 1 Billion nanoseconds in a sec, minus >>10 */
    t_ctx->t_pworld_time.time /= 1000000000.0/1024.0;
    t_ctx->t_pworld_time.delta_time =
	t_ctx->t_pworld_time.time - t_ctx->t_pworld_time.last_time;

}  /* end of t_sync_pworld_time */


/*
 *  Initialize time of flight samples, and time out count.
 */
static void
redbarron_init_samples(samples *samp) {
    int i;

    for (i = 0; i < NS; i++)
	samp->dists[i] = -1.0;

    samp->lost_in_time_count = 60;

    samp->m = samp->b = -11000000.0;
}


/*
 *  Create an instance of a logitech device structure.
 */
static void
redbarron_device_struct_init(redbarron_unit *unit, int prom_rev) {

    unit->prom_revision = prom_rev;
    unit->clock_mul = 1.0; unit->clock_off = -11000000.0;
    unit->diff_from_pworld_time = 0.0;
    unit->raw_stack.stack_index = 0;
    unit->lost_in_time_count = 60 ;
    unit->temporal_filter_length = 8 ;

    redbarron_init_samples(&unit->raw_stack.sll_to_mcal);
    redbarron_init_samples(&unit->raw_stack.sll_to_mlr);
    redbarron_init_samples(&unit->raw_stack.sll_to_mll);
    redbarron_init_samples(&unit->raw_stack.sll_to_mtop);
    redbarron_init_samples(&unit->raw_stack.slr_to_mlr);
    redbarron_init_samples(&unit->raw_stack.slr_to_mll);
    redbarron_init_samples(&unit->raw_stack.slr_to_mtop);
    redbarron_init_samples(&unit->raw_stack.stop_to_mlr);
    redbarron_init_samples(&unit->raw_stack.stop_to_mll);
    redbarron_init_samples(&unit->raw_stack.stop_to_mtop);
}

/*
 * Record the *current* integer offset of the specified ztty serial unit.
 * This is primarily used just before sending a Logitech command with
 * an expected response.
 */
static void
redbarron_obtain_current_offset(redbarron_unit *unit) {
    if (unit->ztty_buf == 0) {
	unit->ztty_last = unit->ztty->a_off ;
    } else {
	unit->ztty_last = unit->ztty->b_off ;
    }
}


/*
 *  Convert a charactor array into the approprate data fields
 *  for a Logitech ``current operational information'' inquiry.
 */
static void
redbarron_convert_cur_op_info(unsigned char *ev,
		  redbarron_cur_op_info *r) {

    int i_r_ll_to_lr_mic, i_r_ll_to_top_mic ;
    int i_t_ll_to_lr_spk, i_t_ll_to_top_spk, i_t_ll_to_cal_mic ;

    /* Init results to zero */
    memset (r, 0, sizeof(redbarron_cur_op_info));

    if ((ev[0]&0xC0) == 0x80) {
	/*
	 *  We have a valid event.
	 *  Now unpack bits from record into integer slots.
	 */
	r->firmware_version_index	= ev[ 0] & 0x3F;
	r->dimension			= ev[ 1] & 0x7F;
	r->reporting_mode		= ev[ 2] & 0x7F;
	r->reporting_data_type		= ev[ 3] & 0x7F;
	r->reporting_data_record_size	= ev[ 4] & 0x7F;
	r->tracking_mode		= ev[ 5] & 0x7F;
	r->master_slave_status		= ev[ 6] & 0x7F;
	r->audio_level			= ev[ 7] & 0x7F;
	r->receiver_type_connected	= ev[ 8] & 0x7F;
	r->transmitter_type_connected	= ev[ 9] & 0x7F;
	r->buton_disable_status		= ev[14] & 0x7F;

#ifdef SWAPBYTES
	i_r_ll_to_lr_mic  = (ev[11] << 8) | ev[10] ;
	i_r_ll_to_top_mic = (ev[13] << 8) | ev[12] ;
	i_t_ll_to_lr_spk  = (ev[16] << 8) | ev[15] ;
	i_t_ll_to_top_spk = (ev[18] << 8) | ev[17] ;
	i_t_ll_to_cal_mic = (ev[20] << 8) | ev[19] ;
#else
	i_r_ll_to_lr_mic  = (ev[10] << 8) | ev[11] ;
	i_r_ll_to_top_mic = (ev[12] << 8) | ev[13] ;
	i_t_ll_to_lr_spk  = (ev[15] << 8) | ev[16] ;
	i_t_ll_to_top_spk = (ev[17] << 8) | ev[18] ;
	i_t_ll_to_cal_mic = (ev[19] << 8) | ev[20] ;
#endif	

	r->d_r_ll_to_lr_mic  = i_r_ll_to_lr_mic  * METERS_PER_INCH/1000.0 ;
	r->d_r_ll_to_top_mic = i_r_ll_to_top_mic * METERS_PER_INCH/1000.0 ;
	r->d_t_ll_to_lr_spk  = i_t_ll_to_lr_spk  * METERS_PER_INCH/1000.0 ;
	r->d_t_ll_to_top_spk = i_t_ll_to_top_spk * METERS_PER_INCH/1000.0 ;
	r->d_t_ll_to_cal_mic = i_t_ll_to_cal_mic * METERS_PER_INCH/1000.0 ;

	r->d_r_ll_lr_to_top_mic = 0.0;	/* Extra info: baseline up */
	r->d_r_top_ams_mic = 0.0;	/* Extra info: center offset  */
	r->d_t_ll_lr_to_top_spk = 0.0;	/* Extra info: baseline up (not used)*/
    }
}


/*
 * Get the current operational info packet.  
 */
static int
redbarron_obtain_current_op_info(redbarron_unit *unit) {
    int  i, j, k ;
    unsigned char *buf, tbuf[30] ;

    /* Make sure the old index is rational! */
    i = unit->ztty_last ;
    if (i < 0 || i >= 2044) return 0 ;

    if (!unit->use_rsb)
	serial_read((serial_device_substruct *)unit) ;

    /* Get the current index and proper buf pts for the desired channel. */
    if (unit->ztty_buf == 0) {
	k = unit->ztty->a_off;
	buf = unit->ztty->abuf;
    } else {
	k = unit->ztty->b_off;
	buf = unit->ztty->bbuf;
    }
    unit->ztty_last = k ;

    /*
     *  If the packet size is not *exactly* 30 bytes,
     *  something has gone wrong, so an all zero struct is pased back
     */
    if((k<i?k+2044-i:k-i) != 30) {
	memset (&unit->cur_op_info, 0, sizeof(redbarron_cur_op_info));
	return 0;
    }

    /* OK, copy the mem buff into the temp structure */
    for (j = 0; j < 30; j++) {
	tbuf[j] = buf[i++];
	if(i >= 2044) i = 0;
    }

    /*
     *  Convert the temp structure into a more usable format,
     *  placing the result into the return structure.
     */
    redbarron_convert_cur_op_info(tbuf, &unit->cur_op_info);

    /* Return success */
    return 1;

}  /* end of redbarron_obtain_current_op_info */



/*
 *  Create and return a new instance of a redbarron device.
 */
static serial_device_substruct *
redbarron_create_instance() {

    redbarron_unit *unit = (redbarron_unit *)malloc(sizeof(redbarron_unit)) ;
    if (unit == 0) {
	fprintf(stderr, "Error:  cannot malloc logitech instance\n") ;
	return 0 ;
    }

    memset(unit, 0, sizeof(redbarron_unit)) ;
    redbarron_device_struct_init(unit, 2) ;

    unit->baud = 19200 ;
    unit->needs_rts_and_dtr = 0 ;

    return (serial_device_substruct *)unit ;
}


/*
 *  Reset an array of logitech units.
 */
static void
redbarron_reset_device_array(redbarron_unit *units[], int count) {
    serial_command((serial_device_substruct **)units, count,
		   100000, 1000000, RB_RESET);
}


/*
 *  Reset a single logitech unit.
 */
static void
redbarron_reset_device(redbarron_unit *unit) {
    redbarron_reset_device_array(&unit, 1) ;
}



/*
 *  Probe an array of logitech units.
 */
static int
redbarron_probe_device_array(redbarron_unit *units[], int count) {
    int i, master_index ;

    for (i = 0 ; i < count ; i++)
	if (units[i]) redbarron_obtain_current_offset(units[i]) ;

    /* get current operational info */
    serial_command((serial_device_substruct **)units, count,
		   100000, 500000, RB_CUR_OP_INFO) ;

    for (i = 0 ; i < count ; i++)
	if (units[i] && redbarron_obtain_current_op_info(units[i]) != 1) {
	    fprintf(stderr, "Error:  cannot read status from logitech unit ") ;
	    fprintf(stderr, "at port %s.\n", units[i]->port_name) ;
	    fprintf(stderr, "(Possibly power off?)\n") ;
	    return 0 ;
	}

    /*
     * The slave device does not know what transmitter type is connected; this
     * must be set in software (by consulting the master).  Code 0x2
     * indicates master and 0x1 indicates slave.
     */
    master_index = -1 ;
    for (i = 0 ; i < count ; i++)
	if (units[i]->cur_op_info.master_slave_status == 0x2) {
	    master_index = i ;
	    break ;
	}

    if (master_index == -1) {
	fprintf(stderr, "Error:  transmitter not connected to any open ") ;
	fprintf(stderr, "logitech unit.\n") ;
	return 0 ;
    }

    /* send transmitter type to slaves */
    if (count > 1) {
	redbarron_unit *master = units[master_index] ;
	int xmiter_type = master->cur_op_info.transmitter_type_connected ;

	/* don't send next commands to the master unit */
	units[master_index] = 0 ;

	/* set the slave transmitter type */
	if (xmiter_type < 0 || xmiter_type > 15) xmiter_type = 15 ;
	RB_SET_SLAVE_XMITER_TYPE[4] = xmiter_type ;
	serial_command((serial_device_substruct **)units, count,
		       100000, 100000, RB_SET_SLAVE_XMITER_TYPE) ;

	/* re-read status to reflect updated info */
	for (i = 0 ; i < count ; i++)
	    if (units[i]) redbarron_obtain_current_offset(units[i]) ;

	serial_command((serial_device_substruct **)units, count,
		       100000, 500000, RB_CUR_OP_INFO) ;

	for (i = 0 ; i < count ; i++)
	    if (units[i]) redbarron_obtain_current_op_info(units[i]) ;

	/* restore master unit */
	units[master_index] = master ;
    }
    
    /*
     * Now that we have read what devices and measurements that Logitech
     * thinks are connected to the boxes, see if the VR package has been
     * told to overide any of these.
     */
    for (i = 0 ; i < count ; i++) 
	if (units[i]) {
	    if (units[i]->overide_d_r_ll_to_lr_mic)
		units[i]->cur_op_info.d_r_ll_to_lr_mic =
		    units[i]->d_r_ll_to_lr_mic ;

	    if (units[i]->overide_d_r_ll_to_top_mic)
		units[i]->cur_op_info.d_r_ll_to_top_mic =
		    units[i]->d_r_ll_to_top_mic ;

	    if (units[i]->overide_d_r_ll_lr_to_top_mic)
		units[i]->cur_op_info.d_r_ll_lr_to_top_mic =
		    units[i]->d_r_ll_lr_to_top_mic ;

	    if (units[i]->overide_d_r_top_ams_mic)
		units[i]->cur_op_info.d_r_top_ams_mic =
		    units[i]->d_r_top_ams_mic ;

	    if (units[i]->overide_d_t_ll_to_lr_spk)
		units[i]->cur_op_info.d_t_ll_to_lr_spk =
		    units[i]->d_t_ll_to_lr_spk ;

	    if (units[i]->overide_d_t_ll_to_top_spk)
		units[i]->cur_op_info.d_t_ll_to_top_spk =
		    units[i]->d_t_ll_to_top_spk ;

	    if (units[i]->overide_d_t_ll_to_cal_mic)
		units[i]->cur_op_info.d_t_ll_to_cal_mic =
		    units[i]->d_t_ll_to_cal_mic ;

	    if (units[i]->overide_d_t_ll_lr_to_top_spk)
		units[i]->cur_op_info.d_t_ll_lr_to_top_spk =
		    units[i]->d_t_ll_lr_to_top_spk ;
	}

    /* put the devices into raw mode */
    serial_command((serial_device_substruct **)units, count,
		   500000, 200000, RB_RAW_MODE) ;

    /* put the devices into continuous streaming mode */
    serial_command((serial_device_substruct **)units, count,
		   100000, 100000, RB_STREAMING) ;

    return 1 ;
}


/*
 *  Probe a single logitech unit.
 */
static int
redbarron_probe_device(redbarron_unit *unit) {
    return redbarron_probe_device_array(&unit, 1) ;
}



/*
 *  Sets attributes both for device and for sensor (because one per device).
 */
static int
redbarron_device_attribute_double(
    serial_device_substruct *instance_data, int attributeNumber, double val) {
    redbarron_unit *unit;

    unit = (redbarron_unit *) instance_data;

    switch(attributeNumber) {

    case D_R_LL_TO_LR_MIC:
	unit->d_r_ll_to_lr_mic = val;
	unit->overide_d_r_ll_to_lr_mic = 1;
	break;

    case D_R_LL_TO_TOP_MIC:
	unit->d_r_ll_to_top_mic = val;
	unit->overide_d_r_ll_to_top_mic = 1;
	break;

    case D_R_LL_LR_TO_TOP_MIC:
	unit->d_r_ll_lr_to_top_mic = val;
	unit->overide_d_r_ll_lr_to_top_mic = 1;
	break;

    case D_R_TOP_AMS_MIC:
	unit->d_r_top_ams_mic = val;
	unit->overide_d_r_top_ams_mic = 1;
	break;

    case D_T_LL_TO_LR_SPK:
	unit->d_t_ll_to_lr_spk = val;
	unit->overide_d_t_ll_to_lr_spk = 1;
	break;

    case D_T_LL_TO_TOP_SPK:
	unit->d_t_ll_to_top_spk = val;
	unit->overide_d_t_ll_to_top_spk = 1;
	break;

    case D_T_LL_TO_CAL_MIC:
	unit->d_t_ll_to_cal_mic = val;
	unit->overide_d_t_ll_to_cal_mic = 1;
	break;

    case D_T_LL_LR_TO_TOP_SPK:
	unit->d_t_ll_lr_to_top_spk = val;
	unit->overide_d_t_ll_lr_to_top_spk = 1;
	break;

    default:
	return 0;
    }

    return 1;
}



/*
 *  No string attribute for Redbarron yet (serial file name handled by super)
 */
static int
redbarron_device_attribute_string(
    serial_device_substruct *instance_data, int attributeNumber, char *val) {
    redbarron_unit *unit;

    unit = (redbarron_unit *)instance_data;

    return 0;
}


/*
 *  Close a logitech unit, but first take it out of stream mode.
 *  (We don't want to wear out the RS-232 cable now, do we?)
 */
static void
redbarron_close(serial_device_substruct *unit) {
    if (unit) {
	/*
	 *  Place the logitech into demand reporting mode, i.e. no output.
	 *  Also be nice and turn off transmitter.
	 */
	serial_command(&unit, 1, 100000, 200000,
		       RB_DEMAND_REPORTING) ;
	serial_command(&unit, 1, 100000, 200000,
		       RB_DISABLE_TRANSMITTER_OUTPUT) ;
    }
}


/*
 *  Install peripheral driver data structure for the redbarron device.
 */
int
redbarron_install_peripheral_driver(nu_serial_ctx_type *ctx) {
    track_ctx *t_ctx ;
    peripheral_driver *pd;

    DPRINT(("redbarron_install_peripheral_driver\n")) ;

    if (ctx->number_peripheral_drivers >= MAX_NUMBER_PERIPHERAL_DRIVERS) {
	fprintf(stderr, "Too many peripheral drivers installed (%d)\n",
		MAX_NUMBER_PERIPHERAL_DRIVERS);
	return 0;
    }

    pd = &ctx->peripheral_drivers[ctx->number_peripheral_drivers++];
    pd->driver_ctx = (void *)malloc(sizeof(track_ctx)) ;
    if (!pd->driver_ctx) {
	fprintf(stderr, "malloc failed for RedBarron driver context\n") ;
	return 0 ;
    }

    pd->peripheral_driver_name = "RedBarron";
    pd->create_instance = redbarron_create_instance;
    pd->reset_device = redbarron_reset_device;
    pd->reset_device_array = redbarron_reset_device_array ;
    pd->probe_device = redbarron_probe_device;
    pd->probe_device_array = redbarron_probe_device_array ;
    pd->device_attribute_double = redbarron_device_attribute_double;
    pd->device_attribute_string = redbarron_device_attribute_string;
    pd->close_device = redbarron_close ;

    /* Init pworld time computation */
    t_ctx = (track_ctx *)pd->driver_ctx ;
    t_init_pworld_time(t_ctx) ;

    /* Not sure that this is right yet */
    t_ctx->t_pworld_time.time_zero = 0.0 ;

    /*
     * Prediction is supposed to be handled in Java 3D core, so don't enable
     * it here.  Java 3D currently has its prediction code disabled,
     * however, so this can be set to 1 for experimental purposes.  Caveat
     * hacker!
     */
    t_ctx->t_track_prediction_enable = 0 ;
    t_ctx->t_track_prediction_time_automatic = 1 ;
    t_ctx->t_track_prediction_max_time = 0.5 ;
    t_ctx->t_max_track_distance = 1.60 ;   /* 1.6 meter max distance */

#ifdef DEBUG
    if (t_ctx->t_track_prediction_enable)
	fprintf(stderr, "  prediction enabled\n") ;
#ifdef EXPERIMENTAL_1
    /*
     * This enables code that tries to knock out suspicious values and replace
     * them with their predictions.  t_ctx->t_track_prediction_enable should
     * also be set.  Caveat hacker!
     */
    fprintf(stderr, "  using EXPERIMENTAL_1 code\n") ;
#endif
#ifdef EXPERIMENTAL_2
    /*
     * This enables code that tries to phase in clock skews as opposed to
     * abrupt updates.  Caveat hacker!
     */
    fprintf(stderr, "  using EXPERIMENTAL_2 code\n") ;
#endif
#ifdef EXPERIMENTAL_3
    /*
     * This enables additional prediction.  t_ctx->t_track_prediction_enable
     * should also be set.  Caveat hacker!
     */
    fprintf(stderr, "  using EXPERIMENTAL_3 code\n") ;
#endif
#endif /* DEBUG */

    return 1;
}


/*
 *  Macro to validate new sample distance between a speaker and mic.
 *  Only used in routine ``redbarron_obtain_current_raw_events'' below.
 */
#define UPDATE_SAMP(pair) \
temp = b->pair; temp = temp*t_ctx->time_to_meters; \
if (temp > 1.600) r->pair.dists[bi] = -1.0; \
else if (r->pair.b < -10000000.0) { \
 temp2 = temp - r->pair.prediction; \
 r->pair.dists[bi] = temp; r->pair.lost_in_time_count = 0; \
 one_inside_reality = 1; \
} else { \
 temp2 = temp - r->ext_ref_time[bi]*r->pair.m - r->pair.b; \
 if (temp2*temp2 < OK_VEL /* || \
    ADDITIONAL THRESHOLD BASED ON VELECOTY, NOT QUITE ROBUST ENOUGH YET \
    fabs(temp2) < 0.5*fabs(r->pair.m)*t_ctx->t_pworld_time.delta_time*/) \
     { r->pair.dists[bi] = temp; r->pair.lost_in_time_count = 0; \
       one_inside_reality = 1; } \
 else if (++r->pair.lost_in_time_count > 20 || \
	  pworld_time - r->pair.good_time > 0.3) \
 { r->pair.dists[bi] = r->pair.prediction = temp; one_inside_reality = 1; } \
 else r->pair.dists[bi] = -1.0; \
}


/*
 *  First, compare the current end charactor position of the shaired memory
 *  FIFO with that last seen by this routine.  From this (by deviding by 24),
 *  determine the number of possible valid uninterprepted new events in
 *  the FIFO (none, 1-(NS-1), NS or greater).  If it has been so long that
 *  NS or more events may be present, clear out *all* the old events cached
 *  on the event stack (as NS of them), marking the entire stack invalid.
 *  
 *  Now go through the raw charactor data looking for legal event headers.
 *  If one is found, copy it into the current event buffer (mostly for
 *  historical reasons, but also to byte swap the data), then process
 *  the event.
 *
 *  Next, the Logitech time stamp is examined, and it and the Host real time
 *  clock are re-syncronized (this is not used yet in the predictor code,
 *  this is a bug!!) (or is this fixed now? 5/14/95).
 *
 *  If this is the master unit, the calibration mic info is checked, and if
 *  in a valid range, the time_to_temp conversion factor is updated.
 *
 *  Finially the 9 distances in the event are put into the event stack,
 *  after being tempeture corrected *and* checked for variance against
 *  the current prediction for that distance channel.
 *
 *  This event examining is looped untill all new charactor data has been
 *  examined.
 *
 *
 *  For debug sanity checking, it would be nice to know:
 *  
 *  For each call:
 *	How many events *might* have arrived,
 *	vs. how many valid headers were found?
 *
 *  For each valid header event:
 *	What was the time value?  What was the clock skew? (big => bogus?)
 *	If master, what happened with the tempature correction factor?
 *	For the 9 distances, what were their values, and prediction variance?
 */
int
redbarron_obtain_current_raw_events(track_ctx *t_ctx, redbarron_unit *unit) {
    register double temp, temp2;
    int  i,j, l, gg, bi, obi, bi_ct = 0, e, last_valid, got_at_least_one = 0;
    int	 one_inside_reality = 0;
    unsigned char *buf;
    redbarron_raw_flt *r;
    redbarron_raw_int *b;
    int bogus_ct = 0;
    double dt, last_valid_dt, pworld_time ;

#ifdef EXPERIMENTAL_1
    double delta1, delta2, delta3 ;
#endif

    t_sync_pworld_time(t_ctx) ;
    pworld_time = t_ctx->t_pworld_time.time;

    if (!unit->use_rsb)
	serial_read((serial_device_substruct *)unit) ;

    /*
     *  Figure out which ztty buf we should be looking in.
     *  Set e to index of last char arrived.
     */
    if (unit->ztty_buf == 0) {
	e = unit->ztty->a_off;  /* um, ptr to next or current loc?? */
	buf = unit->ztty->abuf;
    } else {
	e = unit->ztty->b_off;
	buf = unit->ztty->bbuf;
    }

    /*
     *  Check to see that at least one more event has arrived
     *  Set l to last valid index.
     */
    last_valid = l = unit->ztty_last;
    if (e - (l>e?l-2044:l) < 24) {
	return 0;
    }

    /* Cache pointer to raw stack */
    r = &unit->raw_stack;

    /*
     *  Check to see if the last pointer (l) isn't more than NS*24-1 back.
     *  Also make sure, via checking pworld_time, that we havn't wrapped.
     *  Set i to index back in buf of char that we will start looking
     *  forward from.
     */
    if (l >= 0 && (e<l?e+2044:e) < (l + NS*24) &&
	(r->last_processed_pworld_time + 1.2) > pworld_time) {
	/* Add new events since our last event */
	i = l;
	obi = bi = r->stack_index;
    } else {
	/* Get eight new events */
	i = e-24*NS;
	if (i < 0) i += 2044;
	for (j = 0; j < NS; j++) {
	    r->valid[j] = 0;
	    /* negitive distance implies invalid sample */
	    r->sll_to_mcal.dists[j] = -1.0;
	    r->sll_to_mlr.dists[j] = -1.0;
	    r->sll_to_mll.dists[j] = -1.0;
	    r->sll_to_mtop.dists[j] = -1.0;
	    r->slr_to_mlr.dists[j] = -1.0;
	    r->slr_to_mll.dists[j] = -1.0;
	    r->slr_to_mtop.dists[j] = -1.0;
	    r->stop_to_mlr.dists[j] = -1.0;
	    r->stop_to_mll.dists[j] = -1.0;
	    r->stop_to_mtop.dists[j] = -1.0;
	}
	obi = bi = 0;
    }


    /*
     *  Searching forwards from index i upto index e, find all packets.
     */
    while (i+24 < e || i > (e + 22*24)) {

      throw_it_back:
	while (i+24 < e || i > (e + 22*24)) {
	    /* Set gg to index after i */
	    gg = i==2043?0:i+1;

	    /* Does gg point to valid header byte? If so, jump */
	    if ((buf[gg] == 0xF0) && ((buf[i] & 0xF0) == 0xF0)) goto got_one;

	    /* Inc index i by 1 */
	    if (++i > 2043) i = 0;
	    bogus_ct++;
	}
/*if (unit->ztty_buf == SU)
printf("none %d %f\n", bogus_ct, pworld_time);*/
	last_valid = i;  /* skip past all the proven invalid */
	break;  /* All done if gets here, exits outer while loop. */


    got_one:
	/*
	 *  We will use unit->current_event as a tempory buffer to assemble
	 *  this new event, then converting it into floating point, and
	 *  placing it into the history stack at r->whatever[bi], e.g.
	 *  at stack loc bi.
	 */
	b = (redbarron_raw_int *) unit->current_event;

	/* Copy event from ztty buf into unit->current_event */
	for (j = 0; j < 24; j++) {
	    unit->current_event[j] = buf[i++];  /* inc index i by 1 */
	    if(i > 2043) i = 0;
	}

#ifdef SWAPBYTES
	/* Swap byte order */
	for (j = 0; j < 24; j += 2) {
	    gg = unit->current_event[j];
	    unit->current_event[j] = unit->current_event[j+1];
	    unit->current_event[j+1] = gg;
	}
#endif

	/* Remember, b is an alias for  unit->current_event as shorts. */
	r->ext_ref_time[bi] = b->ext_ref_time;
	r->ext_ref_time[bi] /= 1000.0;  /* raw units of .001 sec */
/*if (unit->ztty_buf == SU)
printf("%d  %d %d  %d  %d %f\n", b->ext_ref_time, i, e, b->sll_to_mlr, bogus_ct, pworld_time);*/

	/*
	 *  ext_ref_time happend in the past, roughfly how much can be
	 *  extimated by the number of charactors that have come in
	 *  since it did, remembering events arrive about every 20ms,
	 *  and each event is 24 charactors in length.
	 */
	dt = (e-i<0?e+2044-i:e-i)*0.02/24.0;

	/*
	 *  Both the Sun and the Logitech have accurate internal clocks.
	 *  But to be able to compair them, they need to be kept in sync.
	 *  To do this, the difference between them is checked and
	 *  update every event, if the difference is small.  Remember also
	 *  that the Logitech clock rolls over every 60 seconds.
	 */
	if (unit->clock_off < -10000000.0)  /* init me! */
	    unit->clock_off = pworld_time -
			      unit->clock_mul*(r->ext_ref_time[bi] + dt);
	temp = pworld_time -
	       (unit->clock_mul*(r->ext_ref_time[bi] + dt) + unit->clock_off);
	if ((temp - 0.1 < unit->diff_from_pworld_time &&
	     temp + 0.1 > unit->diff_from_pworld_time) ||
	    (temp - 60.0 - 0.1 < unit->diff_from_pworld_time &&
	     temp - 60.0 + 0.1 > unit->diff_from_pworld_time) ||
	     unit->lost_in_time_count > 30) {
	    if (temp - 60.0 - 0.1 < unit->diff_from_pworld_time &&
	        temp - 60.0 + 0.1 > unit->diff_from_pworld_time) {
		unit->clock_off += 60.0;
		temp -= 60.0;
	    } else {
		temp = temp; /* ?? */
	    }
	    unit->diff_from_pworld_time = temp;
	    unit->lost_in_time_count = 0;
	} else {
	    unit->lost_in_time_count += 9;
	    /* Must be bogus time value!! */
	    i -= 2; if (i < 0) i += 2044;
	    last_valid = i;
	    goto throw_it_back; /* go try again */
	    /*  WAS::  continue; */
	}

	/* Remember as last valid only if makes it past time check */
	last_valid = i;

	/*
	 *  Convert time of event in logitech clock to time in Sun clock.
	 *  Note that dt *is not* used, we want when the event happened
	 *  in the past.
	 */
	r->ext_ref_time[bi] =
		r->ext_ref_time[bi]*unit->clock_mul + unit->clock_off;

	r->valid[bi] = 1;  /* provisional validty */

	/*
	 *  Copy the button data from the single new event found into
	 *  the storage slot at stack slot bi.
	 *  Note that the raw mode has button bits reversed from usual.
	 */
	gg = b->buttons & 0xF;
	r->buttons[bi] = (gg & 0xA) | ((gg & 0x1)<<2) | ((gg & 0x4)>>2);

	/* Copy the microphone calibration distance measurment into the stack*/
	r->sll_to_mcal.dists[bi] = b->sll_to_mcal;

	/*
	 *  The raw delay time data returned from the logitech units is in
	 *  units of *counts*, where each count is 1/6144000 seconds.
	 *  Each *master* packet contains a calibration mic time delay,
	 *  which can be used to establish the local speed of sound,
	 *  which allows delay times to be converted to distances.
	 */
	if (unit->cur_op_info.master_slave_status == 0x2) {
	    temp =
		unit->cur_op_info.d_t_ll_to_cal_mic/r->sll_to_mcal.dists[bi];
	    /* Code to sanity check temp */
	    if (temp > 0.000045 && temp < 0.000065)
		t_ctx->time_to_meters = temp;
	}

	/*
	 *  Now the 9 real distance measurements (counts as shorts in b)
	 *  are converted into tempature corrected distances into the
	 *  bi lock in the r stack, iff the (normalized) distances have
	 *  (individually) a reasonable value.
	 */
	/* If ALL are outside reality, probably should discard sample */
	UPDATE_SAMP(sll_to_mlr);
	UPDATE_SAMP(sll_to_mll);
	UPDATE_SAMP(sll_to_mtop);
	UPDATE_SAMP(slr_to_mlr);
	UPDATE_SAMP(slr_to_mll);
	UPDATE_SAMP(slr_to_mtop);
	UPDATE_SAMP(stop_to_mlr);
	UPDATE_SAMP(stop_to_mll);
	UPDATE_SAMP(stop_to_mtop);

/*printf("%f %f %f\n%f %f %f\n%f %f %f\n\n",
r->sll_to_mlr.dists[bi], r->sll_to_mll.dists[bi], r->sll_to_mtop.dists[bi],
r->slr_to_mlr.dists[bi], r->slr_to_mll.dists[bi], r->slr_to_mtop.dists[bi],
r->stop_to_mlr.dists[bi], r->stop_to_mll.dists[bi], r->stop_to_mtop.dists[bi]
);*/

	if (one_inside_reality == 0) {
	    r->valid[bi] = 0;
	    continue;
	}
	got_at_least_one++;
	last_valid_dt = dt;

#ifdef EXPERIMENTAL_1
	/*
	 *  Now go through dists again, knocking out the distance from
	 *  any one speaker to a given microphone that moved much more
	 *  than the other two speaker distances to the same microphone.
	 *  Replace the distance with just the prediction.
	 */

	delta1 = fabs(r->sll_to_mlr.dists[bi] - r->sll_to_mlr.prediction);
	delta2 = fabs(r->slr_to_mlr.dists[bi] - r->slr_to_mlr.prediction);
	delta3 = fabs(r->stop_to_mlr.dists[bi] - r->stop_to_mlr.prediction);
	if (16.0*delta2 < delta1 && 16.0*delta3 < delta1)
	    r->sll_to_mlr.dists[bi] = r->sll_to_mlr.prediction;
	else if (16.0*delta1 < delta2 && 16.0*delta3 < delta2)
	    r->slr_to_mlr.dists[bi] = r->slr_to_mlr.prediction;
	if (16.0*delta2 < delta3 && 16.0*delta2 < delta3)
	    r->stop_to_mlr.dists[bi] = r->stop_to_mlr.prediction;

	delta1 = fabs(r->sll_to_mll.dists[bi] - r->sll_to_mll.prediction);
	delta2 = fabs(r->slr_to_mll.dists[bi] - r->slr_to_mll.prediction);
	delta3 = fabs(r->stop_to_mll.dists[bi] - r->stop_to_mll.prediction);
	if (16.0*delta2 < delta1 && 16.0*delta3 < delta1)
	    r->sll_to_mll.dists[bi] = r->sll_to_mll.prediction;
	else if (16.0*delta1 < delta2 && 16.0*delta3 < delta2)
	    r->slr_to_mll.dists[bi] = r->slr_to_mll.prediction;
	if (16.0*delta2 < delta3 && 16.0*delta2 < delta3)
	    r->stop_to_mll.dists[bi] = r->stop_to_mll.prediction;

	delta1 = fabs(r->sll_to_mtop.dists[bi] - r->sll_to_mtop.prediction);
	delta2 = fabs(r->slr_to_mtop.dists[bi] - r->slr_to_mtop.prediction);
	delta3 = fabs(r->stop_to_mtop.dists[bi] - r->stop_to_mtop.prediction);
	if (16.0*delta2 < delta1 && 16.0*delta3 < delta1)
	    r->sll_to_mtop.dists[bi] = r->sll_to_mtop.prediction;
	else if (16.0*delta1 < delta2 && 16.0*delta3 < delta2)
	    r->slr_to_mtop.dists[bi] = r->slr_to_mtop.prediction;
	if (16.0*delta2 < delta3 && 16.0*delta2 < delta3)
	    r->stop_to_mtop.dists[bi] = r->stop_to_mtop.prediction;
#endif /* EXPERIMENTAL_1 */

	/* Inc pointer to next valid sample stash location */
	bi = (bi >= (NS-1))?0:bi+1; bi_ct++;

	/*
	 *  Now go around again and see if there's another valid sample
	 *  in the input charactor buffer.
	 */
    }

    unit->ztty_last = last_valid;  /* update for next time */
    r->stack_index = bi;
    r->last_processed_pworld_time = pworld_time;

    /*
     *  Loop to mark as invalid any stacked event with unreasonable time code.
     */

    /*  Loop here  */

    /*
     *  If we got one, update the clock sync value(s).
     *  For now, just adjust unit->clock_off, and leave unit->clock_mul 1.0
     */
    if (got_at_least_one) {
	/* Brute force clock sync, for now. Eventually, phase rates */
#ifdef EXPERIMENTAL_2
	double avg_skew ;
	avg_skew = (t_ctx->t_pworld_time.time - r->ext_ref_time[bi==0?NS-1:bi-1])/
		   (100.0*t_ctx->t_pworld_time.delta_time);
	unit->clock_mul = (unit->clock_mul + avg_skew)/unit->clock_mul;
	if (unit->clock_mul > 1.1) unit->clock_mul = 1.1;
	else if (unit->clock_mul < 0.9) unit->clock_mul = 0.9;
	unit->clock_mul = 1.0;
#endif
	unit->clock_off +=
		(pworld_time - (r->ext_ref_time[bi==0?NS-1:bi-1] +
			      last_valid_dt*unit->clock_mul))/7.0;
    }

    /*
     *  Return weither we were sucessfull in getting new data.
     */
    return got_at_least_one;

}  /* end of redbarron_obtain_current_raw_events */


/*
 *  For an array of eight samples, form a least squares line fit to
 *  the valid members (positive dist). Then predict a future value
 *  at the delta time into the future.
 */
static double
least_sq_fit_interpolate(track_ctx *t_ctx,
	redbarron_unit *unit, double times[], samples *samp,
	int cur_t, double delta_time) {

    int i,j,k;
    double sum_t = 0, sum_tt = 0, sum_d = 0, sum_td = 0, m, b, n = 0, tmp;

    /* bail if prediction is not enabled */
    if (!t_ctx->t_track_prediction_enable)
	return samp->dists[cur_t];

    /*
     *  Loop through all samples within-in temporal filter range of cur_t,
     *  computing least squares terms.
     */
    for (i = 0; i < NS; i++) {
	if (cur_t >= i) {
	    if (cur_t - i >= unit->temporal_filter_length-1) continue;
	} else {
	    if (cur_t >= unit->temporal_filter_length-1) continue;
	    if (NS-1 - i >= unit->temporal_filter_length-1 - cur_t ) continue;
	}
	if (samp->dists[i] <= 0.0) continue;
/*if (fabs(samp->dist[i] - samp->prediction) > 7.0)*/
	sum_t  += times[i];
	sum_tt += times[i]*times[i];
	sum_d  += samp->dists[i];
	sum_td += times[i]*samp->dists[i];
	n++;
    }

    /* If no valid sample, indicate to use oldest full data. */
    if (n == 0) return samp->prediction;

    /* Finish least squares computation */
    tmp = 1.0/(n*sum_tt - sum_t*sum_t);
    m = (n*sum_td - sum_t*sum_d)*tmp;
    b = (sum_d*sum_tt - sum_td*sum_t)*tmp;

    /* Now to compute the varience */
    for (i = 0, sum_t = 0.0; i < NS; i++) {
	if (cur_t >= i) {
	    if (cur_t - i >= unit->temporal_filter_length-1) continue;
	} else {
	    if (cur_t >= unit->temporal_filter_length-1) continue;
	    if (NS-1 - i >= unit->temporal_filter_length-1 - cur_t ) continue;
	}
	if (samp->dists[i] <= 0.0) continue;
	tmp     = m*times[i] + b - samp->dists[i];
	sum_t  += tmp*tmp;
    }
    sum_t /= n;

    /* If we're on good behavior, update our variance */
    if (sum_t < OK_VEL) {
	samp->variance = sum_t; samp->good_time = times[cur_t];
	samp->m = m; samp->b = b;
    }

    /* dist = m*time + b */
    /* Really need to augment delta_time with synchronized adjusted difference
       between times[cur_t] and current pworld_time!! */
    samp->prediction = m*(times[cur_t] + delta_time) + b;
    return samp->prediction;

}  /* end of least_sq_fit_interpolate */



#ifdef EXPERIMENTAL_3
static	int	cur_stat = 255;

typedef struct	past_poss {
    double	time;
    double	x, y, z;
} past_poss;

static	past_poss past_posA[8] =
{ {-1.0,0.0,0.0,0.0}, {-1.0,0.0,0.0,0.0}, {-1.0,0.0,0.0,0.0},
  {-1.0,0.0,0.0,0.0}, {-1.0,0.0,0.0,0.0}, {-1.0,0.0,0.0,0.0},
  {-1.0,0.0,0.0,0.0}, {-1.0,0.0,0.0,0.0} };
static	int	past_pos_iA = 0;
static	past_poss past_posB[8] =
{ {-1.0,0.0,0.0,0.0}, {-1.0,0.0,0.0,0.0}, {-1.0,0.0,0.0,0.0},
  {-1.0,0.0,0.0,0.0}, {-1.0,0.0,0.0,0.0}, {-1.0,0.0,0.0,0.0},
  {-1.0,0.0,0.0,0.0}, {-1.0,0.0,0.0,0.0} };
static	int	past_pos_iB = 0;

/*
 *  For an array of eight samples of xyz, form a least squares line fit to
 *  the valid members (time>0), less than ? older than cur_t.
 *  Finial prediction should be for time cur_t.
 */
static int
least_sq_fit_avg(double times[], past_poss past_pos[],
		 matrix_d3d track_to_dig, int cur_t) {

    int i,j,k;
    double sum_t = 0, sum_tt = 0, n = 0, tmp;
    double sum_x = 0, sum_tx = 0, mx, bx;
    double sum_y = 0, sum_ty = 0, my, by;
    double sum_z = 0, sum_tz = 0, mz, bz;

    /*
     *  Loop through all samples within-in temporal filter range of cur_t,
     *  computing least squares terms.
     */
    for (i = 0; i < 8; i++) {
	if (times[i] < 0.0) continue;
	if (times[cur_t] - times[i] > 0.16) continue;
	sum_t  += times[i];
	sum_tt += times[i]*times[i];
	sum_x  += past_pos[i].x;
	sum_tx += times[i]*past_pos[i].x;
	sum_y  += past_pos[i].y;
	sum_ty += times[i]*past_pos[i].y;
	sum_z  += past_pos[i].z;
	sum_tz += times[i]*past_pos[i].z;
	n++;
    }

    /* If no valid sample, use oldest full data. */
    if (n == 0) return 0;

    /* Finish least squares computation */
    tmp = 1.0/(n*sum_tt - sum_t*sum_t);
    mx = (n*sum_tx - sum_t*sum_x)*tmp;
    bx = (sum_x*sum_tt - sum_tx*sum_t)*tmp;
    my = (n*sum_ty - sum_t*sum_y)*tmp;
    by = (sum_y*sum_tt - sum_ty*sum_t)*tmp;
    mz = (n*sum_tz - sum_t*sum_z)*tmp;
    bz = (sum_z*sum_tt - sum_tz*sum_t)*tmp;

    /* dist = m*time + b */
if (fabs(mz*times[cur_t] + bz - past_pos[cur_t].z) > 2.0 && cur_stat >= 0 && cur_stat < 255) {
    n = n;
}
    track_to_dig[0][3] = mx*times[cur_t] + bx;
    track_to_dig[1][3] = my*times[cur_t] + by;
    track_to_dig[2][3] = mz*times[cur_t] + bz;

    return 1;  /* Success */

}  /* end of least_sq_fit_avg */
#endif /* EXPERIMENTAL_3 */


/*
    Dervation of sphere formula

Definitions: the labeling of the Logitech transmitter triangle speakers,
and the labeling of the Logitech reciever triangle microphones, follow
the same conventions.  With the triangle *pointed at the observer*,
the lower left devices is at the lower left, etc.
What is importaint to note is that normally the reciever is pointer
*away* from the observer, and in this situation the lower left mic is
to the observer's right. This means that there will be a 180 degree
rotation between the nominal coordinate system of the reciever and
the transmitter.  All of the derivations below will use the
assumption that the labels are applied using the facing definition.
NEW NEW NEW: will now flip reciever coordinate frame so that it is
NEW NEW NEW: right handed (z behind) when pointed at the transmitter.

Origin of Reciever
The origin of the reciever (any device with three microphones,
such as 3D mouse, glasses, or the small reciever) is assumed to be the 3D
point halfway between the 3D points of the left and right recievers
The y orgion used to be offset up by the distance oy, now gone. This offset
was needed to match Logitech's software semantics that the y point is at a
constant distance below the top transmitter (e.g. the glasses are less
high than the mouse and small triangle). This is no longer done.

Dimensions of the Reciever
Logitech reports the dimensions of a reciever as two distances:
that between the two lower mics; and that between the lower left
to top mic. Three additional calibration parameters need to specified:
the hight `h' of the top microphone from the base line between the lower
two; the offset `oy' from the baseline to the y origin (might be derivable
from h); and the right offset `ams' that is the x offset of the top
mic from the true x midpoint between the left and right mics. Half of
distance `bd' between the lower mics is represented by `hbd'.

Dimensions of the Transmitter
Logitech reports the dimensions of a transmitter as two distances:
that between the two lower speakers; and that between the lower left
to top speaker.

Origin of Transmitter
Logitech puts the origion as 12 inches in y and 18 inches in z beyond
the center of base of their transmitter frame. But this is too arbitary
for custom frames, so the VR package has re-defined the transmitter
origin to be the 3D point halfway between the two lower speakers (the
same definition as for recievers).

Coordinates of the Microphones in reciever space
OLD:OLD:OLD:OLD:OLD:
ll mic:	(-hbd, 0, 0)
lr mic:	( hbd, 0, 0)
top mic:( ams, h, 0)

NEW:NEW:NEW:NEW:NEW:
ll mic:	( hbd, 0, 0)
lr mic:	(-hbd, 0, 0)
top mic:(-ams, h, 0)

Given the distance from a single point (one of the transmitting speakers)
to three points (the three recieving microphones), the xyz coordinate
of the transmitter orgin, *in the NEW native coordinate frame of the reciever*,
can be derived.  Let r1 be the distance from the speaker to the ll mic,
r2 to the right mic, and r3 to the top mic: (remember oy is now defunct, 0):

OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:OLD:
r1^2 = (x+hbd)^2 + (y+oy)^2 + z^2
r2^2 = (x-hbd)^2 + (y+oy)^2 + z^2
r3^2 = (x-ams)^2 + (y+oy-h)^2 + z^2

r1^2 = x^2+y^2+z^2 + hbd^2 + 2*x*hbd + oy^2 + 2*y*oy
r2^2 = x^2+y^2+z^2 + hbd^2 - 2*x*hbd + oy^2 + 2*y*oy

r2^2 = r1^2 - 4*x*hbd = r1^2 - 2*x*bd

    :: x = (r1^2 - r2^2)/(2*bd)


r3^2 = x^2+y^2+z^2 + oy^2 + 2*y*oy + ams^2 - 2*x*ams + h^2 - 2*y*h - 2*h*oy
     = (r1^2 - hdb^2 - 2*x*hbd) + ams^2 - 2*x*ams + h^2 - 2*y*h - 2*h*oy

    :: y = (r1^2 - r3^2 - hbd^2 - 2*x*hbd + ams^2 - 2*x*ams + h^2 - 2*h*oy)/2*h


    :: z = sqrt(r1^2 - (x+hbd)^2 - (y+oy)^2)

NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:NEW:
r1^2 = (x-hbd)^2 + y^2 + z^2
r2^2 = (x+hbd)^2 + y^2 + z^2
r3^2 = (x+ams)^2 + (y-h)^2 + z^2

r1^2 = x^2+y^2+z^2 + hbd^2 - 2*x*hbd
r2^2 = x^2+y^2+z^2 + hbd^2 + 2*x*hbd

r2^2 = r1^2 + 4*x*hbd = r1^2 + 2*x*bd

    :: x = (r2^2 - r1^2)/(2*bd)


r3^2 = x^2+y^2+z^2 + ams^2 + 2*x*ams + h^2 - 2*y*h
     = (r1^2 - hdb^2 + 2*x*hbd) + ams^2 + 2*x*ams + h^2 - 2*y*h

    :: y = (r1^2 - r3^2 - hbd^2 + 2*x*hbd + ams^2 + 2*x*ams + h^2)/2*h


    :: z = -sqrt(r1^2 - (x-hbd)^2 - y^2)

*/


/*
 *  For a given tracker unit, compute prediction of future xform matrix.
 *
 *  First compute corrected assumed event *end* times.
 */
int
comp_redbarron_raws(track_ctx *t_ctx,
		    redbarron_unit *unit, matrix_d3d track_to_dig) {
    redbarron_raw_flt *r;
    double	delta_time;
    double	llx, lly, llz;
    double	lrx, lry, lrz;
    double	topx, topy, topz;
    double      ix,iy,iz;
    double      jx,jy,jz;
    double      kx,ky,kz;
    double	ox, oy, oz;
    double	t1, norm, hbd, ams = 0.0, t2, t3;
    double      mll, mlr, mtop;
    double	h, bd;
    double	times[3][NS];
    double	max_track_distance = 1.5;
    int		i, j, ii, bi, foo_bar = 0, need_to_adjust_time;
#ifdef EXPERIMENTAL_3
    past_poss	*past_pos; int past_pos_i;
#endif

    /* Compute delta_time */
    if (t_ctx->t_track_prediction_time_automatic) {
	delta_time = t_ctx->t_pworld_time.delta_time + 1.0/112.9;
	/* If too long, don't overshoot, undershoot. */
	if (delta_time > t_ctx->t_track_prediction_max_time)
	    delta_time = 3.0/112.9;
    } else {
	/* User given time is *relitive* to time of call */
	delta_time = t_ctx->t_track_prediction_time;
    }
    max_track_distance = t_ctx->t_max_track_distance;

    /* Compute index into raw event stack as r->stack_index - 1 */
    r = &unit->raw_stack;
    bi = r->stack_index;
    bi = (bi <= 0)?(NS-1):bi-1;


    /*
     *  The single packet logitech delivered included 9 (10) recpetion events
     *  that occured in 3 batches, at 3 different times. First we must
     *  set the proper phased times in the time array.
     */
    /* Copy time array */
    for (i = 0; i < NS; i++) {
	times[0][i] = r->ext_ref_time[i];
	/* Times for older two speakers */
	times[1][i] = times[0][i] - 0.0066667;
	times[2][i] = times[0][i] - 0.0133333;
    }


    /*
     *  bd is the base distance between the two lower microphones on the
     *  reviever device (glasses, wand, or tracker). hbd is half this.
     *  h is the heigth from this baseline to the third microphone.
     */
    bd = unit->cur_op_info.d_r_ll_to_lr_mic;
    hbd = bd/2.0;
    h = unit->cur_op_info.d_r_ll_lr_to_top_mic;
    if (h == 0) {
	t1 = unit->cur_op_info.d_r_ll_to_top_mic;
	/* tmp correction for glasses */
	/* really wants to know glasses unit in test */
	if (unit->ztty_buf == 0) t1 += 0.0008;
	h = sqrt(t1*t1 - hbd*hbd);
    }
    ams = unit->cur_op_info.d_r_top_ams_mic;



    /*
     *  Now for the lower left source, interpolate predicted future
     *  distances to the three microphones.
     */
    mll  = least_sq_fit_interpolate(t_ctx, unit, times[2], &r->sll_to_mll,
		bi, delta_time);
    mlr  = least_sq_fit_interpolate(t_ctx, unit, times[2], &r->sll_to_mlr,
		bi, delta_time);
    mtop = least_sq_fit_interpolate(t_ctx, unit, times[2], &r->sll_to_mtop,
		bi, delta_time);

    /*
     *  Use sphere formula to find intersect of three line segments.
     *  The results is the xyz location of the lower left speaker
     *  relative to the coordinate frame of the *microphones*.
     */
    llx = (mlr*mlr - mll*mll)/(2.0*bd);
    lly = (mll*mll - mtop*mtop + 2.0*llx*hbd - hbd*hbd + h*h +
	   ams*ams + 2*llx*ams)/(2.0*h);
    llz =  mll*mll - (llx-hbd)*(llx-hbd) - lly*lly;
    if(llz > 0.0) llz = -sqrt(llz); else llz = 0.0;

/*printf("lly: %f  hdb %f  h %f\n", lly, hbd, h);*/
/*printf("lly: %f  mll %f  mtop %f  2llx*hbd %f  -hbd*hbd %f  h*h %f  :: %f\n",
lly, mll, mtop, 2.0*llx*hbd, -hbd*hbd, h*h,
(mll*mll - mtop*mtop + 2.0*llx*hbd - hbd*hbd +h*h)/(2.0*h));*/

    /*
     *  Interpolate predicted future distances from lower right spk to 3 mics.
     */
    mll  = least_sq_fit_interpolate(t_ctx, unit, times[1], &r->slr_to_mll,
		bi, delta_time);
    mlr  = least_sq_fit_interpolate(t_ctx, unit, times[1], &r->slr_to_mlr,
		bi, delta_time);
    mtop = least_sq_fit_interpolate(t_ctx, unit, times[1], &r->slr_to_mtop,
		bi, delta_time);

    /* Use sphere formula to find intersect of three line segments. */
    lrx = (mlr*mlr - mll*mll)/(2.0*bd);
    lry = (mll*mll - mtop*mtop + 2.0*lrx*hbd - hbd*hbd + h*h +
	   ams*ams + 2*lrx*ams)/(2.0*h);
    lrz =  mll*mll - (lrx-hbd)*(lrx-hbd) - lry*lry;
    if (lrz > 0.0) lrz = -sqrt(lrz); else lrz = 0.0;


    /*
     *  Interpolate predicted future distances from top speaker to 3 mics.
     */
    mll  = least_sq_fit_interpolate(t_ctx, unit, times[0], &r->stop_to_mll,
		bi, delta_time);
    mlr  = least_sq_fit_interpolate(t_ctx, unit, times[0], &r->stop_to_mlr,
		bi, delta_time);
    mtop = least_sq_fit_interpolate(t_ctx, unit, times[0], &r->stop_to_mtop,
		bi, delta_time);

    /* Use sphere formula to find intersect of three line segments. */
    topx = (mlr*mlr - mll*mll)/(2.0*bd);
    topy = (mll*mll - mtop*mtop + 2.0*topx*hbd - hbd*hbd + h*h +
	   ams*ams + 2*topx*ams)/(2.0*h);
    topz =  mll*mll - (topx-hbd)*(topx-hbd) - topy*topy;
    if(topz > 0.0) topz = -sqrt(topz); else topz = 0.0;

/*printf("topy: %f  mll %f  mtop %f  2topx*hbd %f  -hbd*hbd %f  h*h %f\n",
topy, mll, mtop, 2.0*topx*hbd, -hbd*hbd, h*h);*/


    /*
     *  The points (llx lly llz), (lrx lry lrz) and (topx topy topz)
     *  correspond to the xyz coordinates of the speakers, in
     *  the coordinate frame of the reciever.
     *  (the lr = lower right mic is on the right as seen FACING the mics)
     *
     *  To get an ijk basis vector, let:
     *  i = lr - ll   k = (top - ll) <cross> i    j = i <cross> k
     *
     *  In this fastion, top does not need to be half way between ll and lr.
     */
    ix = lrx - llx; iy = lry - lly; iz = lrz - llz;
    t2 = norm = 1.0/sqrt(ix*ix + iy*iy + iz*iz);
    ix *= norm; iy *= norm; iz *= norm;

    jx = topx - llx; jy = topy - lly; jz = topz - llz;  /* j* is temp!! */
    kx = iy*jz - jy*iz; ky = jx*iz - ix*jz; kz = ix*jy - jx*iy;
    norm = 1.0/sqrt(kx*kx + ky*ky + kz*kz);
    kx *= norm; ky *= norm; kz *= norm;

    jx = ky*iz - iy*kz; jy = ix*kz - kx*iz; jz = kx*iy - ix*ky;

/*printf("::[%6.3f %6.3f %6.3f]\n", -(llx+lrx)/2.0, -(lly+lry)/2.0, -(llz+lrz)/2.0);*/

    /*
     *  Check to see if the tracker is out of range, either in
     *  distance or in angle.
     *  If so, just return early, caller will use old value of track_to_dig
     *
     *  First, make sure we haven't been infected by any NaN's.
     *  Then, check angle by computing dot of z, and check for
     *  any distance being over 1 meter (for now).
     */
    if (!finite(llx) || !finite(lly) || !finite(llz) ||
        !finite(lrx) || !finite(lry) || !finite(lrz) ||
        !finite(topx) || !finite(topy) || !finite(topz)) ERROR_RET(-1.0);


    norm = sqrt(llx*llx + lly*lly + llz*llz);
    if (-llz/norm < MIN_COS || norm > max_track_distance) ERROR_RET(-3.0);

    norm = sqrt(lrx*lrx + lry*lry + lrz*lrz);
    if (-lrz/norm < MIN_COS || norm > max_track_distance) ERROR_RET(-3.0);

    norm = sqrt(topx*topx + topy*topy + topz*topz);
    if (-topz/norm < MIN_COS || norm > max_track_distance) ERROR_RET(-3.0);


    /* Adjust temporal_filter_length by *last* distance from above */
    if (unit->temporal_filter_length >= 8) {
	if (norm < 0.50) unit->temporal_filter_length = 8;
	else {
	    unit->temporal_filter_length = 16.0*norm;
	    if (unit->temporal_filter_length > 16)
		unit->temporal_filter_length = 16;
	    if (unit->temporal_filter_length < 8)
		unit->temporal_filter_length = 8;
	}
    }


    /* New cross norm check added 1/4/94 */
    if (!finite(ix) || !finite(iy) || !finite(iz) ||
        !finite(jx) || !finite(jy) || !finite(jz) ||
        !finite(kx) || !finite(ky) || !finite(kz)) ERROR_RET(-15.0);

    norm = sqrt(ix*ix + jx*jx + kx*kx);
    /*if (norm < 0.98 || norm > 1.025) ERROR_RET(3.0);*/
    if (norm < 0.97 || norm > 1.025) ERROR_RET(3.0);

    norm = sqrt(iy*iy + jy*jy + ky*ky);
    if (norm < 0.98 || norm > 1.025) ERROR_RET(6.0);

    norm = sqrt(iz*iz + jz*jz + kz*kz);
    if (norm < 0.975 || norm > 1.025) ERROR_RET(9.0);


    track_to_dig[0][0] =  ix;		/* Mxx */
    track_to_dig[0][1] =  iy;		/* Myx */
    track_to_dig[0][2] =  iz;		/* Mzx */

    track_to_dig[1][0] =  jx;
    track_to_dig[1][1] =  jy;
    track_to_dig[1][2] =  jz;

    track_to_dig[2][0] =  kx;
    track_to_dig[2][1] =  ky;
    track_to_dig[2][2] =  kz;

    track_to_dig[3][0] = 0.0;
    track_to_dig[3][1] = 0.0;
    track_to_dig[3][2] = 0.0;
    track_to_dig[3][3] = 1.0;

    ox  = -(llx+lrx)/2.0;
    oy  = -(lly+lry)/2.0;
    oz  = -(llz+lrz)/2.0;

/*printf("[%6.3f %6.3f %6.3f]\n", ox, oy, oz);*/

#ifdef EXPERIMENTAL_3
    /* really wants to know glasses unit in test */
    if (unit->ztty_buf == 0) {
	past_pos = past_posA; past_pos_i = past_pos_iA;
    } else {
	past_pos = past_posB; past_pos_i = past_pos_iB;
    }
    past_pos[past_pos_i].time = times[0][bi] + delta_time;
    past_pos[past_pos_i].x = ix*ox + iy*oy + iz*oz;
    past_pos[past_pos_i].y = jx*ox + jy*oy + jz*oz;
    past_pos[past_pos_i].z = kx*ox + ky*oy + kz*oz;

    
    /* Copy time array */
    for (i = 0; i < 8; i++) times[0][i] = past_pos[i].time;

    if (least_sq_fit_avg(times[0], past_pos,
	track_to_dig, past_pos_i) == 0) return 0;
    past_pos_i = (past_pos_i+1) & 0x7;
    /* really wants to know glasses unit in test */
    if (unit->ztty_buf == 0) past_pos_iA = past_pos_i;
    else past_pos_iB = past_pos_i;
#else
    track_to_dig[0][3] = ix*ox + iy*oy + iz*oz;		/* Mwx */
    track_to_dig[1][3] = jx*ox + jy*oy + jz*oz;
    track_to_dig[2][3] = kx*ox + ky*oy + kz*oz;
#endif

/*
for( i = 0 ; i < 4; i++) {
    for( j = 0; j < 4; j++)
       (void) printf("\t%8.5f", track_to_dig[i][j]);
    (void) printf("\n");
}
(void) printf("\n");
*/

    return 1;  /* Success, one can use this pt. */

}  /* end of comp_redbarron_raws */


/*
 *  Check the history of button values, and return the most recent
 *  valid button value iff it was the same as the last valid two before
 *  that. There *seems* to be a bug where 3D mice that have been
 *  extensively used will return a bogus value (e.g. right but becomes left)
 *  when the right button is initially being depressed.
 */
int
redbarron_debounce_buttons(redbarron_unit *unit) {
    int i, bi, but1 = -1, but2 = -1, but3 = -1;
    redbarron_raw_flt *r;

    /* Cache pointer to raw stack */
    r = &unit->raw_stack;
    bi = r->stack_index;
    i = bi==0?NS-1:bi-1;

    for ( ; (but1 < 0 || but2 < 0 || but3 < 0) && i != bi; i = i==0?NS-1:i-1) {
        if (r->valid[i] == 0) continue;
        if (but1 < 0) but1 = r->buttons[i];
        else if (but2 < 0) but2 = r->buttons[i];
        else if (but3 < 0) but3 = r->buttons[i];
        else break;
    }

    /*
     *  Do we have three valid button events AND were they the same?
     */
    if (but1 >= 0 && but2 >= 0  && but3 >= 0) {
        if (((but1 ^ but2 ) == 0) && ((but1 ^ but3 ) == 0) &&
            ((but2 ^ but3 ) == 0) ) {
            return but1; /* return the last button value */
        }
        return -1; /* return failure. */
    } else
        return -1; /* return failure. */
}


/*
 *  Debugging print for raw event record, int format.
 */
static void
print_redbarron_raw(struct redbarron_raw_int *r) {
    fprintf(stderr, "SA_TO_ABC:\t%5d\t%5d\t%5d\n",
	    r->sll_to_mlr, r->sll_to_mll, r->sll_to_mtop);
    fprintf(stderr, "SB_TO_ABC:\t%5d\t%5d\t%5d\n",
	    r->slr_to_mlr, r->slr_to_mll, r->slr_to_mtop);
    fprintf(stderr, "SB_TO_ABC:\t%5d\t%5d\t%5d\n",
	    r->stop_to_mlr, r->stop_to_mll, r->stop_to_mtop);
    fprintf(stderr, "buttons:\t0x%x\text_ref_time:\t%d\n",
	    r->buttons, r->ext_ref_time);
    fflush(stderr);
}
