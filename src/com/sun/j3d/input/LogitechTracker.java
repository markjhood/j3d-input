/*
 *	@(#)LogitechTracker.java 1.28 03/05/21 14:03:53
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
import javax.media.j3d.* ;

/**
 * Implements the Java 3D <code>InputDevice</code> interface for the Logitech
 * ultrasonic head and mouse trackers.  These are 6 degree of freedom devices
 * that use speakers on the transmitter and microphones on the receivers to
 * compute the receiver position and orientation, relative to the transmitter,
 * based upon the local speed of sound.  They are highly accurate, easy to
 * calibrate, and free from electromagnetic interference, but are
 * line-of-sight devices with a fairly limited range of up to about 1.5
 * meters, and used mostly for desktop virtual reality configurations.<p>
 *
 * This implementation uses native code to communicate with the trackers
 * through standard serial ports at 19200 baud.  The associated
 * <code>libj3dInput</code> native code library must be available through
 * the application library search path at runtime.<p>
 * 
 * Calibration and configuration consist of describing the geometry of the
 * triangular transmitter speaker and receiver microphone arrays and how the
 * the transmitter is oriented and positioned with respect to the display
 * screens.  In this implementation the trackers are operated in raw mode and
 * the coordinate systems and transformations are defined to be consistent
 * with Java 3D as described below.<p>
 * 
 * <b>receiver and transmitter origins</b><p>
 * 
 * The origin of the receiver is the point halfway between the left and right
 * microphones.  Logitech defines the origin of the transmitter as 12 inches
 * in Y and 18 inches in Z beyond the center of the baseline between the left
 * and right speakers.  For the <code>InputDevice</code> implementation it is
 * redefined to be consistent with the receiver origin, on the baseline at
 * the point halfway between the left and right speakers.<p>
 *
 * The origin and orientation of the transmitter defines the Java 3D
 * <i>tracker base</i> frame of reference.  The receiver reports its position
 * and orientation relative to the tracker base.<p>
 *
 * <b>orientation</b><p>
 *
 * The left and right speakers or microphones are defined with respect to the
 * observer when the speaker or microphone array is pointed towards the
 * observer.  For the transmitter, the +Z axis points towards the observer,
 * the +X axis points towards the lower right speaker, and the +Y axis points
 * towards the top speaker.<p>
 * 
 * Normally the receiver is pointed away from the observer, towards the
 * transmitter, and so the lower left microphone is actually to the observer's
 * right.  The receiver's coordinate frame is defined with respect to this
 * nominal orientation such that the +Z direction extends towards the
 * observer, the +X direction extends towards the observer's right (the
 * <i>lower left</i> microphone), and +Y extends upward towards the top
 * microphone.<p>
 *
 * <b>dimensions</b><p>
 *
 * A Logitech tracker will report its dimensions as two distances:
 * the baseline distance between the left and right speakers or microphones,
 * and the distance from the center speaker or microphone at the apex of
 * the triangle to the one on the lower left.  In addition the transmitter
 * reports the distance from its lower left speaker to its calibration
 * microphone so that the current local speed of sound may be computed.<p>
 * 
 * This <code>InputDevice</code> implementation will derive the height of the
 * triangle from these parameters in order to perform its computations, but
 * the height may also be specified directly.  The Logitech trackers are
 * designed to be mounted in customized frames if desired, so any of these
 * calibration constants may be overridden through the various properties
 * provided by this class.<p>
 *
 * <b>calibration</b><p>
 *
 * The application must supply additional calibration information to enable
 * Java 3D to transform points from the tracker base frame of reference to
 * virtual world and image plate coordinates.  These are the transforms for
 * <code>CoexistenceToTrackerBase</code> (in the
 * <code>PhysicalEnvironment</code> class) and
 * <code>TrackerBaseToImagePlate</code> (for each <code>Screen3D</code>
 * object).  See the <i>Java 3D Configuration File</i> documentation for an
 * explanation of these transforms.<p>
 *
 * <b>multiple devices</b><p>
 * 
 * Each tracker requires a separate serial port and must be mapped to a
 * separate instance of <code>LogitechTracker</code>.  Each such instance
 * provides a single <code>Sensor</code> to report the receiver's position
 * and orientation relative to the transmitter origin and orientation (the
 * tracker base), as well as the button state for that receiver.<p>
 * 
 * Up to four Logitech trackers may be used at a time; the <i>master</i> is
 * attached to the transmitter and one receiver, while the others are
 * <i>slaves</i> communicating with the master and attached to a single
 * receiver each.  These relationships are defined by the <code>Slave</code>
 * property associated with the master, which can be specified either through
 * a Java 3D configuration file (read by <code>ConfiguredUniverse</code>) or
 * through the <code>setSlave</code> accessor method.<p>
 *
 * NOTE: <i>The master and slave relationships must be established before any
 * of the devices are initialized</i>.<p>
 * 
 * @see SerialDevice
 */
public class LogitechTracker extends SerialDevice implements InputDevice {
    
    static final int D_R_LL_TO_LR_MIC      =  1 + SerialDevice.LAST_ATTRIBUTE ;
    static final int D_R_LL_TO_TOP_MIC     =  2 + SerialDevice.LAST_ATTRIBUTE ;
    static final int D_R_LL_LR_TO_TOP_MIC  =  3 + SerialDevice.LAST_ATTRIBUTE ;
    static final int D_R_TOP_AMS_MIC       =  4 + SerialDevice.LAST_ATTRIBUTE ;
    static final int D_T_LL_TO_LR_SPK      =  5 + SerialDevice.LAST_ATTRIBUTE ;
    static final int D_T_LL_TO_TOP_SPK     =  6 + SerialDevice.LAST_ATTRIBUTE ;
    static final int D_T_LL_TO_CAL_MIC     =  7 + SerialDevice.LAST_ATTRIBUTE ;
    static final int D_T_LL_LR_TO_TOP_SPK  =  8 + SerialDevice.LAST_ATTRIBUTE ;

    private static LogitechTracker master = null ;
    private static int slaveCount = 0 ;

    private Sensor sensor = null ;
    private int[] buttons = null ;
    private Transform3D t3d = null ;
    private double[] matrix = null ;

    // Get the current matrix and button array from the device.
    native int getEvents(long ctx, int deviceIndex,
			 double[] matrix, int[] buttons) ;

    /**
     * A parameterless constructor for this <code>InputDevice</code>.  This 
     * is used for <code>ConfiguredUniverse</code>, which requires such a
     * constructor for configurable input devices.  The serial port assignment
     * must be specified with the <code>SerialPort</code> property.
     * <p>
     * <b>Syntax:</b><br>(NewDevice <i>&lt;name&gt;</i>
     * com.sun.j3d.input.LogitechTracker)
     *
     * @see SerialDevice
     */
    public LogitechTracker() {
	super("RedBarron") ;

	// Each LogitechTracker instance supports one and only one Sensor.
	// Create it with 30 SensorReads and 4 buttons (the glasses will not
	// use this button array).
	buttons = new int[4] ;
	sensor = new Sensor(this, 30, 4) ;

	// Create the matrix to receive the sensor transform and the
	// Transform3D to create the SensorRead.
	t3d = new Transform3D() ;
	matrix = new double[16] ;

	// Set the initial sensor read value.
	sensor.setNextSensorRead(System.currentTimeMillis(), t3d, buttons) ;
    }

    /**
     * Creates a new <code>LogitechTracker</code> instance.<p>
     *
     * @param portName name of the serial port to which this instance is
     *  attached
     * @exception <code>IllegalArgumentException</code> if port assignment
     *  fails
     */
    public LogitechTracker(String portName) {
	this() ;
	super.setSerialPort(portName) ;
    }

    /**
     * Initializes the device.  A device should be initialized before it is
     * registered with Java 3D via the
     * <code>PhysicalEnvironment.addInputDevice</code> method call.<p>
     * 
     * All master and slave relationships between <code>LogitechTracker</code>
     * instances must be established before <code>initialize</code> is called
     * for any of them.
     * 
     * @return true for succesful initialization, false for failure
     * @exception <code>IllegalStateException</code> if a port name has not
     *  been specified
     * @exception <code>RuntimeException</code> if there is an error opening
     *  the serial port
     * @see #Slave Slave()
     * @see #setSlave 
     */
    public boolean initialize() {
	super.openPorts() ;
	return true ;
    }

    /**
     * Sets the device's current position and orientation as the device's
     * nominal position and orientation (establish its reference frame
     * relative to the <i>tracker base</i> reference frame).  The
     * <code>LogitechTracker</code> implementation of this method does
     * nothing since it is an absolute positioning/orientation device.
     */
    public void setNominalPositionAndOrientation() {
	// Nothing to do for absolute position/orientation device.
    }

    /**
     * Causes the device's sensor readings to be updated by the device driver.
     * This is called by the Java 3D input device scheduler.
     */
    public void pollAndProcessInput() {
	if (! open)
	    // This can happen and cause native code exceptions if close() is
	    // called before removing from the PhysicalEnvironment.
	    throw new IllegalStateException
		("\nAttempt to read a device that is not open.") ;

	if (getEvents(nativeContext, id, matrix, buttons) != 0) {
	    t3d.set(matrix) ;
	    sensor.setNextSensorRead
		(System.currentTimeMillis(), t3d, buttons) ;
	}
    }

    /**
     * This method will not be called by the Java 3D implementation and 
     * should be implemented as an empty method.
     */
    public void processStreamInput() {
    }

    /**
     * Sets a device's processing mode to either <code>NON_BLOCKING</code>,
     * <code>BLOCKING</code>, or <code>DEMAND_DRIVEN</code>.  The
     * <code>LogitechTracker</code> implementation only supports
     * <code>NON_BLOCKING</code>; any other mode will throw an
     * <code>IllegalArgumentException</code>.  There isn't any need to call
     * this method for this implementation of <code>InputDevice</code>.<p>
     *
     * NOTE: this method should <i>not</i> be called after the input
     * device has been added to a <code>PhysicalEnvironment</code>.  The
     * processing mode must remain constant while a device is attached
     * to a <code>PhysicalEnvironment</code>.
     *
     * @param mode <code>NON_BLOCKING</code>
     */
    public void setProcessingMode(int mode) {
	if (mode != NON_BLOCKING)
	    throw new IllegalArgumentException("Mode must NON_BLOCKING") ;
    }

    /**
     * Retrieves the device's processing mode.  For the
     * <code>LogitechTracker</code>, this is always
     * <code>NON_BLOCKING</code>.
     * 
     * @return <code>NON_BLOCKING</code>
     */
    public int getProcessingMode() {
	return(InputDevice.NON_BLOCKING) ;
    }

    /**
     * Gets the specified sensor associated with the device.  The sensor
     * indices begin at zero and end at <code>getSensorCount</code> minus
     * one.<p>
     * 
     * A <code>LogitechTracker</code> provides a single sensor for each
     * instance; to use both a head tracker and a mouse tracker requires a
     * master/slave relationship between two instances of
     * <code>LogitechTracker</code> on two separate serial ports.<p>
     *
     * The sensor read value is a matrix containing the full 6DOF position and
     * orientation of the corresponding device.  The reported position and
     * orientation is relative to the <i>tracker base</i>.
     * 
     * @param sensorIndex the sensor to retrieve
     * @return the specified sensor
     */
    public Sensor getSensor(int index) {
	if (index != 0)
	    throw new IllegalArgumentException
		("Sensor index must be 0 for LogitechTracker") ;

	return sensor ;
    }

    /**
     * Gets the number of sensors associated with the device.
     * @return 1
     */
    public int getSensorCount() {
	return 1 ;
    }

    /**
     * Cleans up the device and relinquishes associated resources.  This
     * method should be called only after the device has been unregistered
     * from Java 3D via the <code>PhysicalEnvironment.removeInputDevice</code>
     * method call.
     */
    public void close() {
	if (this == master)
	    master = null ;
	else if (slaveCount > 0)
	    slaveCount-- ;

	super.close() ;
    }

    /**
     * Establishes this <code>LogitechTracker</code> instance as the
     * <i>master</i> with respect to the specified <i>slave</i> instance.  The
     * master is attached to the transmitter and one receiver, while the slave
     * is attached only to a receiver.  There can be only one master.  Up to
     * three slaves can be specified with multiple calls to this method.  This
     * method must not be called if only a single <code>LogitechTracker</code>
     * instance is to be used.<p>
     *
     * Note: The master and the specified slave must both be
     * <i>uninitialized</i> at the time this method is called.
     *
     * @param slave an uninitialized <code>LogitechTracker</code> instance
     * @exception <code>NullPointerException</code> if the slave reference is
     *  null
     * @exception <code>IllegalStateException</code> if either this instance
     *  or the slave instance have already been initialized, or if there is
     *  more than one master, or if there are more than three slaves
     * @see #Slave Slave()
     * @see #initialize
     */
    public void setSlave(LogitechTracker slave) {
	// 
	// For now, we don't actually do much with the master/slave
	// relationship here; the master and slaves are determined by the
	// native code.  This method is currently mainly used to enforce a
	// single native-layer initialization of all the LogitechTracker
	// slaves along with their master.
	//
	if (slave.open)
	    throw new IllegalStateException("slave device is already open") ;

	if (this.open)
	    throw new IllegalStateException("master device is already open") ;

	if (master == null) {
	    master = this ;
	}
	else if (this != master) {
	    // already have a master
	    if (master.portName == null)
		throw new IllegalStateException
		    ("another tracker is already master") ;
	    else
		throw new IllegalStateException
		    ("tracker at " + master.portName + " is already master") ;
	}

	if (slaveCount > 2)
	    throw new IllegalStateException("maximum number of slaves is 3") ;
	else
	    slaveCount++ ;
    }

    /**
     * Property which establishes this <code>LogitechTracker</code> instance
     * as the <i>master</i> with respect to the specified <i>slave</i>
     * instance.  The master is attached to the transmitter and one receiver,
     * while the slave is attached only to a receiver.  There can be only one
     * master.  Up to three slaves can be specified with multiple settings of
     * this property.  This property must not be specified if only a single
     * <code>LogitechTracker</code> instance is to be used.<p>
     *
     * This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.  NOTE: the (Device <i>&lt;name&gt;</i>)
     * built-in command must be used to retrieve an actual device instance
     * from the device name; using just the name itself will throw an
     * <code>IllegalArgumentException</code>.
     * <p>
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;master name&gt;</i>
     * Slave (Device <i>&lt;slave name&gt;</i>))
     * 
     * @param slave array of length 1 containing an instance of
     *  <code>LogitechTracker</code>
     * @exception <code>NullPointerException</code> if the slave reference is
     *  null
     * @exception <code>IllegalArgumentException</code> if the slave reference
     *  is not a LogitechTracker
     * @exception <code>IllegalStateException</code> if either this instance
     *  or the slave instance have already been initialized, or if there is
     *  more than one master, or if there are more than three slaves
     * @see #setSlave
     */
    public void Slave(Object[] slave) {
	// 
	// For now, we don't actually do much with the master/slave
	// relationship here; the master and slaves are determined by the
	// native code.
	//
	// This method is currently used only for error checking and for
	// symmetry with the setSlave() method.  ConfiguredUniverse only
	// initializes InputDevice implementations after all of them have been
	// instantiated, so masters and slaves will always be initialized
	// together in the native code.
	// 
	if (! (slave.length == 1 && slave[0] instanceof LogitechTracker))
	    throw new IllegalArgumentException
		("slave must be a single LogitechTracker instance") ;

	setSlave((LogitechTracker)slave[0]) ;
    }

    /**
     * Property which specifies the distance in meters between the receiver's
     * lower left and lower right microphones.  The default is whatever is
     * reported by the device.  This property is set in the configuration file
     * read by <code>ConfiguredUniverse</code>.
     * <p>
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * ReceiverBaseline <i>&lt;distance&gt;</i>)
     * 
     * @param distance array of length 1 containing an instance of
     * <code>Double</code>
     */
    public void ReceiverBaseline(Object[] distance) {
	if (! (distance.length == 1 && distance[0] instanceof Double))
	    throw new IllegalArgumentException
		("LogitechTracker ReceiverBaseline must be a Double") ;
	
	deviceAttribute(nativeContext, id, D_R_LL_TO_LR_MIC,
			((Double)distance[0]).doubleValue()) ;
    }

    /**
     * Property which sets the distance between the transmitters's lower left
     * and lower right speakers.  The default is whatever is reported by the
     * device. This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.
     * <p>
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * TransmitterBaseline <i>&lt;distance&gt;</i>)
     * 
     * @param distance array of length 1 containing an instance of
     * <code>Double</code>
     */
    public void TransmitterBaseline(Object[] distance) {
	if (! (distance.length == 1 && distance[0] instanceof Double))
	    throw new IllegalArgumentException
		("LogitechTracker TransmitterBaseline must be a Double") ;
	
	deviceAttribute(nativeContext, id, D_T_LL_TO_LR_SPK,
			((Double)distance[0]).doubleValue()) ;
    }

    /**
     * Property which sets the distance between the receiver's lower left and
     * top microphones.  The default is whatever is reported by the
     * device. This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.
     * <p>
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * ReceiverLeftLeg <i>&lt;distance&gt;</i>)
     * 
     * @param distance array of length 1 containing an instance of
     * <code>Double</code>
     */
    public void ReceiverLeftLeg(Object[] distance) {
	if (! (distance.length == 1 && distance[0] instanceof Double))
	    throw new IllegalArgumentException
		("LogitechTracker ReceiverLeftLeg must be a Double") ;
	
	deviceAttribute(nativeContext, id, D_R_LL_TO_TOP_MIC,
			((Double)distance[0]).doubleValue()) ;
    }

    /**
     * Property which sets the distance between the transmitters's lower left
     * and top speakers.  The default is whatever is reported by the
     * device. This property is set in the configuration file read
     * by <code>ConfiguredUniverse</code>.
     * <p>
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * TransmitterLeftLeg <i>&lt;distance&gt;</i>)
     * 
     * @param distance array of length 1 containing an instance of
     * <code>Double</code>
     */
    public void TransmitterLeftLeg(Object[] distance) {
	if (! (distance.length == 1 && distance[0] instanceof Double))
	    throw new IllegalArgumentException
		("LogitechTracker TransmitterLeftLeg must be a Double") ;
	
	deviceAttribute(nativeContext, id, D_T_LL_TO_TOP_SPK,
			((Double)distance[0]).doubleValue()) ;
    }

    /**
     * Property which sets the distance between the receiver's baseline and
     * top microphone.  The default is computed from ReceiverLeftLeg and half
     * the ReceiverBaseline distance, assuming that the apex of the triangle
     * is centered over the baseline.  This property is set in the
     * configuration file read by <code>ConfiguredUniverse</code>.
     * <p>
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * ReceiverHeight <i>&lt;distance&gt;</i>)
     * 
     * @param distance array of length 1 containing an instance of
     * <code>Double</code>
     */
    public void ReceiverHeight(Object[] distance) {
	if (! (distance.length == 1 && distance[0] instanceof Double))
	    throw new IllegalArgumentException
		("LogitechTracker ReceiverHeight must be a Double") ;
	
	deviceAttribute(nativeContext, id, D_R_LL_LR_TO_TOP_MIC,
			((Double)distance[0]).doubleValue()) ;
    }

    /**
     * Property which sets the distance between the transmitters's baseline
     * and top speaker.  The default is computed from TransmitterLeftLeg and
     * half the TransmitterBaseline distance, assuming that the apex of the
     * triangle is centered over the baseline.  This property is set in the
     * configuration file read by <code>ConfiguredUniverse</code>.
     * <p>
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * TransmitterHeight <i>&lt;distance&gt;</i>)
     * 
     * @param distance array of length 1 containing an instance of
     * <code>Double</code>
     */
    public void TransmitterHeight(Object[] distance) {
	if (! (distance.length == 1 && distance[0] instanceof Double))
	    throw new IllegalArgumentException
		("LogitechTracker TransmitterHeight must be a Double") ;
	
	deviceAttribute(nativeContext, id, D_T_LL_LR_TO_TOP_SPK,
			((Double)distance[0]).doubleValue()) ;
    }

    /**
     * Property which sets the X offset distance between the receiver's top
     * microphone and the point halfway between the lower left and lower right
     * microphones.  The default is 0.  This property is set in the
     * configuration file read by <code>ConfiguredUniverse</code>.
     * <p>
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * ReceiverTopOffset <i>&lt;distance&gt;</i>)
     * 
     * @param distance array of length 1 containing an instance of
     * <code>Double</code>
     */
    public void ReceiverTopOffset(Object[] distance) {
	if (! (distance.length == 1 && distance[0] instanceof Double))
	    throw new IllegalArgumentException
		("LogitechTracker ReceiverTopOffset must be a Double") ;
	
	deviceAttribute(nativeContext, id, D_R_TOP_AMS_MIC,
			((Double)distance[0]).doubleValue()) ;
    }

    /**
     * Property which sets the distance between the transmitter's lower left
     * speaker and its calibration microphone.  This distance is used to
     * compute the local speed of sound.  The default is whatever is reported
     * by the device.  This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.
     * <p>
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * TransmitterCalibrationDistance <i>&lt;distance&gt;</i>)
     * 
     * @param distance array of length 1 containing an instance of
     * <code>Double</code>
     */
    public void TransmitterCalibrationDistance(Object[] distance) {
	if (! (distance.length == 1 && distance[0] instanceof Double))
	    throw new IllegalArgumentException
		("LogitechTracker TransmitterCalibrationDistance " +
		 "must be a Double") ;
	
	deviceAttribute(nativeContext, id, D_T_LL_TO_CAL_MIC,
			((Double)distance[0]).doubleValue()) ;
    }

    /**
     * Sets the given attribute/value pair for the device.  This is provided
     * for applications not using <code>ConfiguredUniverse</code>. Accepted
     * attributes are the following calibration constants, measured in
     * meters:<p>
     *
     * "d_r_ll_to_lr_mic"<p>
     * Distance between the receiver's lower left and lower right
     * microphones.<p>
     * 
     * "d_r_ll_to_top_mic"<p>
     * Distance between receivers's lower left and top microphones.<p>
     *
     * "d_r_ll_lr_to_top_mic"<p>
     * Distance of the top microphone from the triangle base between lower
     * left and lower right microphones.<p>
     *
     * "d_r_top_ams_mic"<p>
     * X offset of top microphone from the point halfway between the lower
     * left and lower right microphones.<p>
     * 
     * "d_t_ll_to_lr_spk"<p>
     * Distance between the transmitters's lower left and lower right
     * speakers<p>
     * 
     * "d_t_ll_to_top_spk"<p>
     * Distance between transmitters's lower left and top speakers.<p>
     * 
     * "d_t_ll_to_cal_mic"<p>
     * Distance from the transmitters's lower left speaker to the calibration
     * microphone.<p>
     *
     * "d_t_ll_lr_to_top_spk"<p>
     * Distance of the top speaker from the triangle base between lower
     * left and lower right speaker.<p>
     *
     * @param attribute the attribute to be set, represented as a
     *  <code>String</code>
     * @param value the value of the attribute, represented as a
     *  <code>double</code> 
     * @return <code>boolean</code> representing whether or not the attribute
     *  setting was successful
     */
    public boolean deviceAttribute(String attribute, double val) {
	int attributeNumber ;
	int ret ;

	if (attribute.equals("d_r_ll_to_lr_mic"))
	    attributeNumber = D_R_LL_TO_LR_MIC ;
	else
	if (attribute.equals("d_r_ll_to_top_mic"))
	    attributeNumber = D_R_LL_TO_TOP_MIC ;
	else
	if (attribute.equals("d_r_ll_lr_to_top_mic"))
	    attributeNumber = D_R_LL_LR_TO_TOP_MIC ;
	else
	if (attribute.equals("d_r_top_ams_mic"))
	    attributeNumber = D_R_TOP_AMS_MIC ;
	else
	if (attribute.equals("d_t_ll_to_lr_spk"))
	    attributeNumber = D_T_LL_TO_LR_SPK ;
	else
	if (attribute.equals("d_t_ll_to_top_spk"))
	    attributeNumber = D_T_LL_TO_TOP_SPK ;
	else
	if (attribute.equals("d_t_ll_to_cal_mic"))
	    attributeNumber = D_T_LL_TO_CAL_MIC ;
	else
	if (attribute.equals("d_t_ll_lr_to_top_spk"))
	    attributeNumber = D_T_LL_LR_TO_TOP_SPK ;
	else
	    return false ;

	ret = deviceAttribute(nativeContext, id, attributeNumber, val) ;

	return ret == 0 ? false : true ;
    }
}
