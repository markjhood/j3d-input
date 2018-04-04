/*
 *	@(#)TrackdInputDevice.java 1.9 02/10/02 17:31:36
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
package com.sun.j3d.input ;

import java.io.* ;
import javax.media.j3d.* ;
import com.sun.j3d.utils.universe.* ;

/**
 * Implements <code>InputDevice</code> for the VRCO trackd device interface.
 * The implementation uses native code and requires the native
 * <code>j3dInput</code> shared library somewhere in the application library
 * search path.  This class also requires a VRCO trackd software installation;
 * in particular, the <code>trackdAPI</code> shared library must also exist
 * somewhere in the application library search path.  The trackd software is
 * not supplied by the <code>com.sun.j3d.input</code> package and must be
 * obtained from VRCO.<p>
 *
 * Trackd is a middleware standard that abstracts most of the popular motion
 * tracking sensor devices into a common API.  <code>TrackdInputDevice</code>
 * uses this generic interface to allow integration of any trackd-supported
 * device into Java 3D applications.<p>
 * 
 * Calibration and configuration of a trackd device is performed through
 * trackd software, which will establish a coordinate frame for the device's
 * output.  In a typical CAVE configuration the origin of this coordinate
 * frame will be the center of the floor and the basis vectors will be aligned
 * with the center front screen.  This device coordinate frame as established
 * by the trackd software must be used as the <i>tracker base</i> frame of
 * reference for Java 3D purposes.  Trackd device coordinates are expressed in
 * feet, but <code>TrackdInputDevice</code> will convert those coordinates
 * into meters for its sensor reads.<p>
 * 
 * The application must supply additional calibration information to enable
 * Java 3D to transform coordinates from tracker coordinates to virtual world
 * and image plate coordinates.  These are the transforms for
 * <code>CoexistenceToTrackerBase</code> (in the
 * <code>PhysicalEnvironment</code> class) and
 * <code>TrackerBaseToImagePlate</code> (for each <code>Screen3D</code>
 * object).  See the <i>Java 3D Configuration File</i> documentation for an
 * explanation of these parameters.<p>
 *
 * @author Paul Gordon, University of Calgary; Mark Hood, Sun Microsystems
 */
public class TrackdInputDevice implements InputDevice {

    // Shared memory keys with default values.
    private int trackerShmKey = 4126 ;
    private int controllerShmKey = 4127 ;

    // Default processing mode is NON_BLOCKING.  DEMAND_DRIVEN will also work,
    // but BLOCKING is unsupported.
    private int processingMode = NON_BLOCKING ;

    // Java 3D treats valuators as Sensors, but trackd considers them a
    // distinct type of device.  Here we refer to the motion-tracked devices
    // supported by trackd as "trackers" to distinguish them from the
    // valuators. 
    private int buttonCount = 0 ;
    private int trackerCount = 0 ;
    private int valuatorCount = 0 ;
    private int sensorCount = 0 ;

    private int[] buttons = null ;
    private Sensor[] sensors = null ;
    private Transform3D[] t3d = null ;
    private float[] trackerMatrix =
        {1, 0, 0, 0,   0, 1, 0, 0,   0, 0, 1, 0,   0, 0, 0, 1} ;
    private float[] valuatorMatrix = 
        {1, 0, 0, 0,   0, 1, 0, 0,   0, 0, 1, 0,   0, 0, 0, 1} ;

    // Native library calls.
    native int initTrackd(int trackerShmKey, int controllerShmKey) ;
    native int getButtonCount() ;
    native int getTrackerCount() ;
    native int getValuatorCount() ;
    native int getButton(int index) ;
    native void getMatrix(float[] matrix, int index) ;
    native float getValuator(int index) ;

    /**
     * Load the native code library.
     */
    static {
        java.security.AccessController.doPrivileged(
            new java.security.PrivilegedAction() {
            public Object run() {
                System.loadLibrary("j3dInput") ;
                return null ;
            }
        }) ;
    }

    /**
     * Parameterless constructor for this <code>InputDevice</code>.  This is
     * used for <code>ConfiguredUniverse</code>, which requires such a
     * constructor for configurable input devices.  The tracker and controller
     * shared memory keys must be specified with the
     * <code>SharedMemoryKeys</code> property.<p>
     * 
     * <b>Syntax:</b><br>(NewDevice <i>&lt;name&gt;</i>
     * com.sun.j3d.input.TrackdInputDevice)
     */
    public TrackdInputDevice() {
    }

    /**
     * Creates a new <code>TrackdInputDevice</code> with the specified tracker
     * and controller shared memory keys.  See the trackd API for details
     * about specifying these values; they are usually found in the
     * <code>trackd.conf</code> file.
     *
     * @param trackerShmKey the shared memory key for the tracker
     * @param controllerShmKey the shared memory key for the controller
     */
    public TrackdInputDevice(int trackerShmKey, int controllerShmKey) {
	this.trackerShmKey = trackerShmKey ;
	this.controllerShmKey = controllerShmKey ;
    }

    /**
     * Property which specifies the tracker and controller shared memory keys.
     * See the trackd API for details about specifying these values; they are
     * usually found in the <code>trackd.conf</code> file.  This property is
     * set in the configuration file read by
     * <code>ConfiguredUniverse</code>.<p>
     * 
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * SharedMemoryKeys <i>&lt;tracker key&gt; &lt;controller key&gt;</i>)
     * 
     * @param keys array of length 2 containing <code>Doubles</code>; the
     *  int value of the first is the tracker shared memory key, and the int
     *  value of the second is the controller shared memory key
     */
    public void SharedMemoryKeys(Object[] keys) {
        if (! (keys.length == 2 &&
               keys[0] instanceof Double && keys[1] instanceof Double))
            throw new IllegalArgumentException
                ("\nSharedMemoryKeys must be numbers") ;
        
	this.trackerShmKey = ((Double)keys[0]).intValue() ;
	this.controllerShmKey = ((Double)keys[1]).intValue() ;
    }
    
    /**
     * Initializes the device.  A device should be initialized before it is
     * registered with Java 3D via the
     * <code>PhysicalEnvironment.addInputDevice</code> method call.
     *
     * @return true for succesful initialization, false for failure
     */
    public boolean initialize() {
	int status = initTrackd(trackerShmKey, controllerShmKey) ;
	
	String message = "" ;
	if ((status & 0x4) == 0x4) {
	    message += "\nTrackdInputDevice: trackdAPI not available " ;
	}
	if ((status & 0x2) == 0x2) {
	    message += "\nTrackdInputDevice: failed initializing controller " ;
	    message += "using shared memory key " + controllerShmKey ;
	}
	if ((status & 0x1) == 0x1) {
	    message += "\nTrackdInputDevice: failed initializing tracker " ;
	    message += "using shared memory key " + trackerShmKey ;
	}
	if (status != 0) {
	    System.out.println(message) ;
	    return false ;
	}

        buttonCount = getButtonCount() ;
        trackerCount = getTrackerCount() ;
        valuatorCount = getValuatorCount() ;
        sensorCount = trackerCount + (valuatorCount > 0 ? 1 : 0) ;

        System.out.println("TrackdInputDevice: using " +
                           buttonCount + " buttons, " +
                           trackerCount + " trackers, " +
			   valuatorCount + " valuators") ;

        buttons = new int[buttonCount] ;
        sensors = new Sensor[sensorCount] ; 
        t3d = new Transform3D[sensors.length] ;

        for (int i = 0 ; i < sensors.length ; i++) {
	    sensors[i] = new Sensor(this, 30, buttonCount);
	    t3d[i] = new Transform3D() ;
        }

	return true ;
    }

    /**
     * Sets the device's current position and orientation as the
     * device's nominal position and orientation (establish its reference
     * frame relative to the <i>tracker base</i> reference frame).<p>
     * 
     * The <code>TrackdInputDevice</code> implementation of this method does
     * nothing since it is an absolute position and orientation device.
     */
    public void setNominalPositionAndOrientation() {
        // Nothing to do for absolute position/orientation device.
    }

    /**
     * Causes the device's sensor readings to be updated by the device driver.
     * For <code>BLOCKING</code> and <code>NON_BLOCKING</code> drivers, this
     * method is called regularly and the Java 3D implementation can cache the
     * sensor values.  For <code>DEMAND_DRIVEN</code> drivers this method is
     * called each time one of the <code>Sensor.getRead</code> methods is
     * called, and is not otherwise called.<p>
     * 
     * The <code>TrackdInputDevice</code> implementation does not support
     * <code>BLOCKING</code> mode.
     */
    public void pollAndProcessInput() {
	long time ;
        int index ;

	// Set button values.  In the Trackd implementation there is only one
	// set of buttons for all sensors and valuators.
        for (int i = 0 ; i < buttons.length ; i++) {
            buttons[i] = getButton(i) ;
        }

	// Set 6DOF sensor values.
        for (index = 0 ; index < trackerCount ; index++) {
            getMatrix(trackerMatrix, index) ;
            t3d[index].set(trackerMatrix) ;
	    time = System.currentTimeMillis() ;
            sensors[index].setNextSensorRead(time, t3d[index], buttons) ;
        }

	// Set 2D/3D sensor values from available valuators.
	if (valuatorCount > 0) {
	    for (int i = 0 ; i < valuatorCount && i < 3 ; i++) {
		// Put values in matrix translational components.
		valuatorMatrix[(i*4) + 3] = getValuator(i) ;
	    }

            t3d[index].set(valuatorMatrix) ;
	    time = System.currentTimeMillis() ;
            sensors[index].setNextSensorRead(time, t3d[index], buttons) ;
        }
    }

    /**
     * This method is not called by the Java 3D implementation and is
     * implemented as an empty method.
     */
    public void processStreamInput() {
    }

    /**
     * Sets the device's processing mode to either <code>NON_BLOCKING</code>
     * or <code>DEMAND_DRIVEN</code>.  The default mode is
     * <code>NON_BLOCKING</code>. <code>TrackdInputDevice</code> does not
     * support <code>BLOCKING</code> mode.<p>
     *
     * NOTE: this method should <i>not</i> be called after the input
     * device has been added to a <code>PhysicalEnvironment</code>.  The
     * processing mode must remain constant while a device is attached
     * to a <code>PhysicalEnvironment</code>.
     *
     * @param mode either <code>NON_BLOCKING</code> or
     *  <code>DEMAND_DRIVEN</code>
     */
    public void setProcessingMode(int mode) {
	if (mode != NON_BLOCKING && mode != DEMAND_DRIVEN)
            throw new IllegalArgumentException
                ("Mode must be NON_BLOCKING or DEMAND_DRIVEN") ;

	processingMode = mode ;
    }

    /**
     * Retrieves the device's processing mode, either
     * <code>NON_BLOCKING</code> or <code>DEMAND_DRIVEN</code>.
     * 
     * @return the processing mode, either <code>NON_BLOCKING</code>
     *  or <code>DEMAND_DRIVEN</code>
     */
    public int getProcessingMode() {
        return processingMode ;
    }

    /**
     * Gets the specified sensor associated with the device.  The sensor
     * indices begin at zero and end at <code>getSensorCount</code> minus
     * one.<p>
     * 
     * The usual ordering for trackd sensors puts the head tracker at index 0,
     * the hand tracker/wand at index 1, and a 2D valuator/controller at index
     * 2.  The tracker sensors return the full 6DOF positions and orientations
     * of the devices in the sensor read matrix; the reported positions and
     * orientations are relative to the <i>tracker base</i>.  The controller
     * sensor returns its 2D values in the (X, Y) translation components of
     * the sensor read matrix; these are normalized to the range [-1.0
     * .. +1.0].
     * 
     * @param sensorIndex the sensor to retrieve
     * @return the specified sensor
     */
    public Sensor getSensor(int index) {
        if (index < 0 || index >= sensors.length)
            throw new IllegalArgumentException
                ("Sensor index must be between 0 and " + (sensors.length-1)) ;

        return sensors[index] ;
    }

    /**
     * Gets the number of sensors associated with the device.
     * 
     * @return the device's sensor count
     */
    public int getSensorCount() {
        return sensorCount ;
    }

    /**
     * Cleans up the device and relinquishes associated resources.  This
     * method should be called after the device has been unregistered from
     * Java 3D via the <code>PhysicalEnvironment.removeInputDevice</code>
     * method call.
     */
    public void close() {
    }
}

