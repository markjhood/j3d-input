/*
 *	@(#)redbarron.h 1.12 02/10/04 20:26:07
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

#include "com_sun_j3d_input_LogitechTracker.h"

/*
 * The Logitech trackers are little-endian devices.  For now we assume this
 * code is most likely to be run on big-endian machines.
 */
/* #if defined(sparc) || defined(__sparc) */
#define SWAPBYTES
/* #endif */

typedef enum {
    D_R_LL_TO_LR_MIC = 
    com_sun_j3d_input_LogitechTracker_D_R_LL_TO_LR_MIC,
    D_R_LL_TO_TOP_MIC =
    com_sun_j3d_input_LogitechTracker_D_R_LL_TO_TOP_MIC,
    D_R_LL_LR_TO_TOP_MIC =
    com_sun_j3d_input_LogitechTracker_D_R_LL_LR_TO_TOP_MIC,
    D_R_TOP_AMS_MIC =
    com_sun_j3d_input_LogitechTracker_D_R_TOP_AMS_MIC,
    D_T_LL_TO_LR_SPK =
    com_sun_j3d_input_LogitechTracker_D_T_LL_TO_LR_SPK,
    D_T_LL_TO_TOP_SPK =
    com_sun_j3d_input_LogitechTracker_D_T_LL_TO_TOP_SPK,
    D_T_LL_TO_CAL_MIC =
    com_sun_j3d_input_LogitechTracker_D_T_LL_TO_CAL_MIC,
    D_T_LL_LR_TO_TOP_SPK =
    com_sun_j3d_input_LogitechTracker_D_T_LL_LR_TO_TOP_SPK
} redbarron_attributes ;


/*
 *  Low level integer structures for Logitech Red Barron 6d mouse event
 *  records.
 */

#define MOUSE6D_RECORD_SIZE     24


/*
 *  Event record of raw mouse event in integer form.
 */
typedef	struct	redbarron_raw_int {
    unsigned	short	buttons;
    unsigned	short	sll_to_mlr, sll_to_mll, sll_to_mtop;
    unsigned	short	sll_to_mcal;
    unsigned	short	slr_to_mlr, slr_to_mll, slr_to_mtop;
    unsigned	short	ext_ref_time;
    unsigned	short	stop_to_mlr, stop_to_mll, stop_to_mtop;
} redbarron_raw_int;



#define NS 16
typedef struct	samples {
    float	dists[NS];
    float	prediction;
    float	variance;
    double	m,b;
    float	good_time;
    int		lost_in_time_count;
} samples;

 
/*
 *  Stack of raw mouse distances.
 */
typedef	struct	redbarron_raw_flt {
    double	last_processed_pworld_time;
    int		stack_index;
    int		valid[NS];
    float	ext_ref_time[NS];
    int		buttons[NS];
    samples	sll_to_mcal;
    samples	sll_to_mlr,  sll_to_mll,  sll_to_mtop;
    samples	slr_to_mlr,  slr_to_mll,  slr_to_mtop;
    samples	stop_to_mlr, stop_to_mll, stop_to_mtop;
} redbarron_raw_flt;

 
/*
 *  Structure of current operational information request
 *  from Logitech unit.
 */
typedef struct redbarron_cur_op_info {
    int         firmware_version_index;
    int         dimension;
    int         reporting_mode; 
    int         reporting_data_type; 
    int         reporting_data_record_size; 
    int         tracking_mode; 
    int         master_slave_status; 
    int         audio_level; 
    int         receiver_type_connected; 
    int         transmitter_type_connected; 
    double      d_r_ll_to_lr_mic; 
    double      d_r_ll_to_top_mic;
    double	d_r_ll_lr_to_top_mic;	/* Extra info: baseline up */
    double	d_r_top_ams_mic;	/* Extra info: center offset  */
    int         buton_disable_status; 
    double      d_t_ll_to_lr_spk;
    double      d_t_ll_to_top_spk;
    double      d_t_ll_to_cal_mic;
    double	d_t_ll_lr_to_top_spk;	/* Extra info: baseline up (not used)*/
    int         reserved22, reserved23, reserved24, reserved25, reserved26; 
    int         reserved27, reserved28, reserved29, reserved30; 
} redbarron_cur_op_info;



/*
 *  Coordinateing structure for information about a tracker unit.
 */
typedef struct redbarron_unit {
    SERIAL_DEVICE_COMMON_FIELDS;  /* common fields from serial.h */

    /* the last good event that we received */
    unsigned char	 current_event[MOUSE6D_RECORD_SIZE];
    /* the event that we're currently working on */
    unsigned char	 event[MOUSE6D_RECORD_SIZE];
    int			 event_size;
    redbarron_raw_flt	 raw_stack;
    double		 clock_mul, clock_off, diff_from_pworld_time;
    int			 prom_revision;
    int			 demand_mode;
    int			 temporal_filter_length;
    redbarron_cur_op_info cur_op_info;

    int			 overide_d_r_ll_to_lr_mic;
    double		 d_r_ll_to_lr_mic;
    int			 overide_d_r_ll_to_top_mic;
    double		 d_r_ll_to_top_mic;
    int			 overide_d_r_ll_lr_to_top_mic;
    double		 d_r_ll_lr_to_top_mic;
    int			 overide_d_r_top_ams_mic;
    double		 d_r_top_ams_mic;
    int			 overide_d_t_ll_to_lr_spk;

    double		 d_t_ll_to_lr_spk;
    int			 overide_d_t_ll_to_top_spk;
    double		 d_t_ll_to_top_spk;
    int			 overide_d_t_ll_to_cal_mic;
    double		 d_t_ll_to_cal_mic;
    int			 overide_d_t_ll_lr_to_top_spk;
    double		 d_t_ll_lr_to_top_spk;

    int                  lost_in_time_count ;
} redbarron_unit;


extern	char	redbarron_message[256];

extern	int errno;

#ifndef METERS_PER_INCH
#define METERS_PER_INCH 0.0254
#endif

/*
 * check an event for validity
 */
#define	REDBARRON_VALID_EVENT(ev)	(((ev)[0]&0x80) != 0)



/* 
 *  Desired: counts per inch.
 *
 *  6144000 counts/sec /(1000 ft/sec * 12 inches)
 */
#define SND (6144000/(1000.0*12.0))

#define ERROR_RET(aa) return 0;

/* Velocity threshold for leagal inter-event movement.  (15 slight over) */
/*#define OK_VEL (1.5*1.5)*/
#define OK_VEL (0.015*0.015)

/* Cosine of maximum angle of speaker to mic, currently cos(85 degrees) */
#define MIN_COS 0.087155743

typedef struct general_time {
    double              time;
    double              last_time;
    double              delta_time;
    double              time_zero; 
} general_time;

typedef struct track_ctx {
    general_time t_pworld_time;
    hrtime_t long_long_pworld_time_zero ;
    int t_track_prediction_enable;
    int t_track_prediction_time_automatic;
    int t_track_prediction_time;
    int t_track_prediction_max_time;
    int t_max_track_distance;
    double time_to_meters ;
} track_ctx;

/*
 * Extern prototypes.
 */
int
redbarron_install_peripheral_driver(nu_serial_ctx_type *ctx) ;

int
redbarron_obtain_current_raw_events(track_ctx *t_ctx, redbarron_unit *unit) ;

int
comp_redbarron_raws(track_ctx *t_ctx,
		    redbarron_unit *unit, matrix_d3d track_to_dig) ;

int
redbarron_debounce_buttons(redbarron_unit *unit) ;
