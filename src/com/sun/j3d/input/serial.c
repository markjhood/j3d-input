/*
 *	@(#)serial.c 1.23 02/10/07 20:01:51
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

#include <math.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/termios.h>
#include <sys/filio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stropts.h>
#include <sys/conf.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "serial.h"

/*
 * Low level serial driver code for VRlib and Java 3D.
 *
 * Derived 8/25/98 from initial version updated 9/5/92 by Michael F. Deering.
 * Revised 2000 - 2002 for dev use by subsequent Java 3D releases, and
 * converted for public com.sun.j3d.input package October 2002 by Mark Hood.
 * 
 * No longer requires the Solaris Realtime Serial Buffer (RSB) package, and is
 * capable of handling an arbitrary number of serial ports instead of just
 * two.  serial_ports_init_open_probe() can now be called multiple times to
 * open ports individually, but will still allow multiple ports to be opened,
 * reset, and initialized for devices that share affinities.
 */

#ifdef SOLARIS
/*
 * The following variables are global to all serial contexts.  They are the
 * file descriptor of the RSB pseudo-device (if used), the shared memory
 * address of the RSB pseudo-device buffers, and the number of devices using
 * it.  Up to to two serial ports are supported for RSB.  They must be
 * /dev/ttya and /dev/ttyb.
 *
 * In Java 3D, all device polling is performed by a single thread, the Input
 * Device Scheduler, but device initialization and closure can be performed
 * from separate threads.  Access to all global data upon initialization and
 * closure is explicitly serialized at the Java level.
 * 
 * RSB (Realtime Serial Buffer) is a low-latency shared memory interface to
 * the serial ports for Solaris.  The RSB package is available only up Solaris
 * 7, as latency is improved in subsequent releases.  If RSB is not used, then
 * character data from the serial ports is read using read(2).
 */
static int rsb_fd = -1 ;
static void *mmap_va = 0 ;
static int mmap_ref_count = 0 ;
#endif

/*
 *  This routine does most of the work to initialize the Sun's serial port(s)
 *  for use with input devices. This routine handles arrays of serial ports
 *  to allow (necessary) sleeps during the baud rate changing period
 *  to be overlapped.
 *
 *  1 is returned on success, 0 on fail.
 *
 *  For all serial ports, in parallel (to gang sleep's):
 *
 *     Open the named serial port.
 *     Reset the serial port to the specified baud rate
 *       (optionally turn on RTS and DTR iff needed).
 *     Pop all streams modules.
 *     Set non-blocking IO.
 */
static int
serial_ports_open(serial_device_substruct *ttys[], int count)
{
    int i, flags, baud ;
    struct termios tio ;
    char exstreamname[FMNAMESZ+1] ;

    for (i = 0 ; i < count ; i++) {
	if (ttys[i] && ttys[i]->state != SERIAL_DEVICE_STATE_OPENED) {

	    /* check port name */
	    if (!ttys[i]->port_name) {
		fprintf(stderr, "no port name specified for device %d\n", i) ;
		return 0 ;
	    }

	    /* open the tty */
	    if ((ttys[i]->fd = open(ttys[i]->port_name, O_RDWR)) == -1) {
		fprintf(stderr, "failed opening %s\n", ttys[i]->port_name) ;
		return 0 ;
	    }

	    DPRINT(("serial_ports_open:  opened %s @ %d baud, rts/dtr %s\n",
		    ttys[i]->port_name, ttys[i]->baud,
		    ttys[i]->needs_rts_and_dtr? "high" : "low")) ;

	    /* set port parameters */
	    switch (ttys[i]->baud) {
	      case   300: baud =   B300 ; break ;
	      case   600: baud =   B600 ; break ;
	      case  1200: baud =  B1200 ; break ;
	      case  2400: baud =  B2400 ; break ;
	      case  4800: baud =  B4800 ; break ;
	      case  9600: baud =  B9600 ; break ;
	      case 19200: baud = B19200 ; break ;
	      default:
		fprintf(stderr, "unsupported baud rate %d set for %s\n",
			ttys[i]->baud, ttys[i]->port_name) ;
		return 0 ;
	    }

	    tio.c_cc[VMIN] = 0 ;
	    tio.c_cc[VTIME] = 1 ; 
	    tio.c_cflag = baud | CS8 | CREAD | CLOCAL ;
	    if (ioctl(ttys[i]->fd, TCSETS, &tio) == -1) {
		fprintf(stderr, "failed setting port parameters on %s (%d)",
			ttys[i]->port_name, errno) ;
		close(ttys[i]->fd) ;
		return 0 ;
	    }

	    /*
	     * Set RTS and DTR high, iff requested.  Some serial devices
	     * (like the gameport) sneak power off of RTS and DTR, so they
	     * must be high.
	     */
	    if (ttys[i]->needs_rts_and_dtr)
		ioctl(ttys[i]->fd, TIOCMBIS, TIOCM_RTS | TIOCM_DTR) ;
	}
    }

    /*
     *  Sleep, waiting for serial port baud changes to take effect.
     */
    sleep(1) ;

    for (i = 0 ; i < count ; i++) {
	if (ttys[i] && ttys[i]->state != SERIAL_DEVICE_STATE_OPENED) {
	    /* pop all stream modules */
	    while (ioctl(ttys[i]->fd, I_LOOK, exstreamname) != -1) {
		if (ioctl(ttys[i]->fd, I_POP, 0) == -1) {
		    fprintf(stderr, "could not pop stream module %s ",
			    exstreamname) ;
		    fprintf(stderr, "from port %s\n", ttys[i]->port_name) ;
		    close(ttys[i]->fd) ;
		    return 0 ;
		}
	    }

	    /* set non-blocking IO */
	    flags = fcntl(ttys[i]->fd, F_GETFL) ;
	    fcntl(ttys[i]->fd, F_SETFL, flags | O_NDELAY) ;
	}
    }

    return 1 ;
}


#ifdef SOLARIS
/*
 * Set up Realtime Serial Buffer character by-pass mechanism.
 */
static int
serial_map_rsb(mtty **ztty)
{
    /* buffer size is 4096 bytes for mtty abuf, bbuf, and current positions */
    int mmapsize = 0x1000 ;

    /* data is mapped at offset 65536 from memory object */
    int regs_offset = 0x10000 ;

    if (*ztty) {
	return 1 ;
    }
    else if (mmap_va) {
	*ztty = (struct mtty *)mmap_va ;
	return 1 ;
    }

    usleep(100000) ;
    if ((rsb_fd = open("/devices/pseudo/rsb:rsb", O_RDWR)) < 0) {
	if ((rsb_fd = open("/devices/pseudo/rsb@0:rsb", O_RDWR)) < 0) {
	    fprintf(stderr, "Warning:  open of rsb failed, ") ;
	    fprintf(stderr, "is driver installed?\n") ;
	    fprintf(stderr, "Reading from tty using read(2) instead.\n") ;
	    return 0 ;
	}
    }

    mmap_va = mmap((void *)0, mmapsize,
		   PROT_READ | PROT_WRITE, MAP_SHARED | _MAP_NEW,
		   rsb_fd, regs_offset) ;

    usleep(100000) ;
    if (mmap_va == MAP_FAILED) {
	fprintf(stderr, "Warning:  mmap() of rsb failed\n") ;
	fprintf(stderr, "Reading from tty using read(2) instead.\n") ;
	mmap_va = 0 ;
	return 0 ;
    }

    memset(mmap_va, 0, mmapsize) ;
    *ztty = (struct mtty *)mmap_va ;

    DPRINT(("serial_map_rsb:  mmap()'d ztty @ 0x%x\n", mmap_va)) ;
    return 1 ;
}
#endif /* SOLARIS */


/*
 * Open and probe all currently unopened serial ports.
 */
int
serial_ports_init_open_probe(nu_serial_ctx_type *ctx)
{
    int i, j ;

    /* cache pointers to drivers and devices */
    peripheral_driver *drivers = ctx->peripheral_drivers ;
    serial_device_substruct **devices = ctx->peripheral_assignments ;

#ifdef SOLARIS
    /* RSB requires special handling for ttya and ttyb */
    serial_device_substruct *ttya = 0 ;
    serial_device_substruct *ttyb = 0 ;

    /*
     * Make initial pass for ttya and ttyb.
     */
    for (i = 0 ; i < MAX_NUMBER_PERIPHERAL_ASSIGNMENTS ; i++) {
	/* skip unused or deallocated entries */
	if (!devices[i] || !devices[i]->port_name) continue ;

	/* get last character of port name */
	switch (devices[i]->port_name[strlen(devices[i]->port_name)-1]) {
	  case 'a':
	    ttya = devices[i] ;
	    break ;
	  case 'b':
	    ttyb = devices[i] ;
	    break ;
	  default:
	    if (devices[i]->use_rsb) {
		devices[i]->use_rsb = 0 ;
		fprintf(stderr, "Warning: RSB port must be ttya or ttyb.\n") ;
		fprintf(stderr, "%s is not mapped to RSB; using read(2).\n",
			devices[i]->port_name) ;
	    }
	}
    }

    /*
     * Check RSB usage on ttya and ttyb.
     */
    if (ttya && ttya->use_rsb && ttya->state != SERIAL_DEVICE_STATE_OPENED) {
	if (ttyb && ttyb->state == SERIAL_DEVICE_STATE_OPENED) {
	    if (!ttyb->use_rsb) {
		ttya->use_rsb = 0 ;
		fprintf(stderr, "Warning: can't use RSB on ttya ") ;
		fprintf(stderr, "since ttyb opened without RSB\n") ;
	    }
	}
	else {
	    if (ttyb && !ttyb->use_rsb) {
		ttyb->use_rsb = 1 ;
		fprintf(stderr, "Warning: requesting RSB on ttyb ") ;
		fprintf(stderr, "since ttya requested RSB\n") ;
	    }
	}
    }

    if (ttyb && ttyb->use_rsb && ttyb->state != SERIAL_DEVICE_STATE_OPENED) {
	if (ttya && ttya->state == SERIAL_DEVICE_STATE_OPENED) {
	    if (!ttya->use_rsb) {
		ttyb->use_rsb = 0 ;
		fprintf(stderr, "Warning: can't use RSB on ttyb ") ;
		fprintf(stderr, "since ttya opened without RSB\n") ;
	    }
	}
	else {
	    if (ttya && !ttya->use_rsb) {
		ttya->use_rsb = 1 ;
		fprintf(stderr, "Warning: requesting RSB on ttya ") ;
		fprintf(stderr, "since ttyb requested RSB\n") ;
	    }
	}
    }
#endif /* SOLARIS */

    DPRINT(("serial_ports_init_open_probe\n")) ;

    /*
     * Attempt to open all currently unopened ports.  Note that
     * serial_ports_open() doesn't set states to SERIAL_DEVICE_STATE_OPENED.
     */
    if (!serial_ports_open(devices, MAX_NUMBER_PERIPHERAL_ASSIGNMENTS)) {
	fprintf(stderr, "Error:  cannot open one or more serial ports\n") ;
	return 0 ;
    }

    /*
     * Loop through all devices again.
     */
    for (i = 0 ; i < MAX_NUMBER_PERIPHERAL_ASSIGNMENTS ; i++) {
	/* skip unused or deallocated entries */
	if (!devices[i]) continue ;

	/* skip initialized devices */
	if (devices[i]->state == SERIAL_DEVICE_STATE_OPENED) continue ;

#ifdef SOLARIS
	/* if RSB seems OK, try to map it */
	if (devices[i]->use_rsb && serial_map_rsb(&devices[i]->ztty)) {
	    /* RSB mapping successful */
	    mmap_ref_count++ ;
	}
	else
#endif
	{
	    /* RSB not requested or not successful -- use read(2) instead */
	    struct mtty *m = (struct mtty *)malloc(sizeof(struct mtty)) ;
	    if (!m) {
		fprintf(stderr, "Error:  cannot malloc mtty\n") ;
		return 0 ;
	    }

	    memset((void *)m, 0, sizeof(struct mtty)) ;
	    devices[i]->ztty = m ;
	    devices[i]->use_rsb = 0 ; /* in case serial_map_rsb() failed */
	}

	/* backward compatibility: use abuf in ztty for ttya, bbuf otherwise */
	if (devices[i]->port_name[strlen(devices[i]->port_name)-1] == 'a')
	    devices[i]->ztty_buf = 0 ;
	else
	    devices[i]->ztty_buf = 1 ;

	/* check if affinities supported */
	j = devices[i]->peripheral_driver_index ;
	if (drivers[j].reset_device_array && drivers[j].probe_device_array) {
	    /* just record affinity device for now and continue */
	    drivers[j].affinities[drivers[j].affinity_count++] = devices[i] ;
	    continue ;
	}

	/* reset and probe the device */
	drivers[j].reset_device(devices[i]) ;
	if (! drivers[j].probe_device(devices[i])) {
	    fprintf(stderr, "Error:  cannot probe device %s\n",
		    devices[i]->port_name) ;
	    return 0 ;
	}

	/* it's open now */
	devices[i]->state = SERIAL_DEVICE_STATE_OPENED ;
    }

    /*
     * Initialize any affinity device arrays.  These are used for devices that
     * need to be initialized together, such as the Logitech master and slave
     * devices, but any group of devices that share common reset and
     * initialization commands can also be treated as affinity devices.  The
     * caller must ensure that serial_ports_init_open_probe() is called only
     * after all such device instances have been created.
     */
    for (j = 0 ; j < ctx->number_peripheral_drivers ; j++) {
	if (drivers[j].affinity_count > 0) {
	    drivers[j].reset_device_array(drivers[j].affinities,
					  drivers[j].affinity_count) ;

	    if (!drivers[j].probe_device_array(drivers[j].affinities,
					       drivers[j].affinity_count)) {
		fprintf(stderr, "Error:  cannot probe one or more devices\n") ;
		return 0 ;
	    }

	    /* mark these devices as opened */
	    for (i = 0 ; i < drivers[j].affinity_count ; i++)
		drivers[j].affinities[i]->state = SERIAL_DEVICE_STATE_OPENED ;

	    /* reset affinity count */
	    drivers[j].affinity_count = 0 ;
	}
    }

    return 1 ;
}


/*
 *  Send a multi-character command to the specified units.
 *  Pre/post sleep arguments are now passed in, so that the caller
 *  can avoid overstacking of sleeps.  Sleeps are in units of us.
 */
void
serial_command(serial_device_substruct *units[], int count,
	       int pre_sleep, int post_sleep,
	       unsigned char *command)
{
    int i ;

    /* first make sure any existing commands have been flushed out */
    for (i = 0 ; i < count ; i++)
	if (units[i] && units[i]->fd != -1)
	    ioctl(units[i]->fd, I_FLUSH, FLUSHRW) ;

    /* sleep by users pre amount (in us) */
    usleep(pre_sleep) ;

    /* actually send the command */
    for (i = 0 ; i < count ; i++)
	if (units[i] && units[i]->fd != -1)
	    write(units[i]->fd, command, strlen((char *)command)) ;

    /* sleep by users post amount (in us) */
    usleep(post_sleep) ;
}


/*
 * Read characters from the tty when RSB is not used.  The character data is
 * read into circular FIFOs that emulate the RSB since there is much existing
 * code which directly manipulates those FIFOs.
 *
 * This leads to a minor memory inefficiency here, since a non-RSB tty always
 * gets its own ztty with two 2044-byte buffers (abuf and bbuf), and only one
 * will be used.  A single ztty could theoretically be shared between any two
 * ttys, but this entails more cumbersome bookkeeping, so for now a ztty is
 * shared between two ports only when RSB is in use.
 *
 * There is also some existing code that looks at ztty_buf to see if it is
 * reading from /dev/ttya or /dev/ttyb, so anything that is not /dev/ttya will
 * have ztty_buf set to 1.
 */
void
serial_read(serial_device_substruct *s) {
    int *i, length, byteCount ;
    unsigned char *buf ;

    if (s->ztty_buf == 0) {
	i = &s->ztty->a_off ;
	buf = s->ztty->abuf ;
    }
    else {
	i = &s->ztty->b_off ;
	buf = s->ztty->bbuf ;
    }

    length = 2044 - *i ;
    byteCount = read(s->fd, (void *)(buf + *i), length) ;

    if (byteCount == length) {
	/* try to read some more */
	byteCount = read(s->fd, (void *)buf, 2044) ;
        if (byteCount > 0) {
	    *i = byteCount ;
	}
    }
    else if (byteCount > 0) {
	*i += byteCount ;
    }
}


/*
 * Close a serial port and release resources.
 */
void
serial_close(nu_serial_ctx_type *ctx, int deviceIndex)
{
    serial_device_substruct *unit ;

    DPRINT(("serial_close:  device %d\n", deviceIndex)) ;

    if (! ctx) return ;
    if (deviceIndex >= MAX_NUMBER_PERIPHERAL_ASSIGNMENTS) return ;

    unit = ctx->peripheral_assignments[deviceIndex] ;
    ctx->peripheral_assignments[deviceIndex] = 0 ;
    if (! unit) return ;

    if (unit->port_name) {
	DPRINT(("  port name %s\n", unit->port_name)) ;
	free(unit->port_name) ;
	unit->port_name = 0 ;
    }

    if (unit->fd >= 0) {
	DPRINT(("  port file descriptor %d\n", unit->fd)) ;
	close(unit->fd) ;
	unit->fd = -1 ;
	unit->state = SERIAL_DEVICE_STATE_CLOSED ;
    }

#ifdef SOLARIS
    if (unit->use_rsb) {
	DPRINT(("  current mmap_ref_count is %d\n", mmap_ref_count)) ;
	if (mmap_va && --mmap_ref_count == 0) {
	    /* memory-mapped RSB in use and no more references */
	    DPRINT(("  unmapping RSB @ 0x%x\n", mmap_va)) ;
	    DPRINT(("  closing RSB pseudo-device %d\n", rsb_fd)) ;
	    munmap(mmap_va, 0x1000) ;
	    close(rsb_fd) ;
	    mmap_va = 0 ;
	    rsb_fd = -1 ;
	}
    }
    else 
#endif
    if (unit->ztty) {
	DPRINT(("  freeing ztty @ 0x%x\n", unit->ztty)) ;
	free(unit->ztty) ;
    }

    unit->ztty = 0 ;
    unit->peripheral_driver_index = -1 ;
    free(unit) ;
}
