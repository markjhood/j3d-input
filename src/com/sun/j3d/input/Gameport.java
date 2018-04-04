/*
 *	@(#)Gameport.java 1.18 03/05/21 14:02:40
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

import java.awt.Dimension ;
import java.awt.Toolkit ;
import javax.media.j3d.InputDevice ;
import javax.media.j3d.Sensor ;
import javax.media.j3d.Transform3D ;
import javax.swing.BorderFactory ;
import javax.swing.Box ;
import javax.swing.JLabel ;
import javax.swing.JWindow ;
import javax.vecmath.Vector3d ;
import com.sun.j3d.utils.behaviors.sensor.SensorEventAgent ;

/**
 * Implements the Java 3D <code>InputDevice</code> interface for joystick
 * devices using the serial <i>workstation gameport</i> interface.  The
 * workstation gameport is a small hardware device that connects between an
 * analog PC-style joystick and a standard serial port.  It is powered from
 * the serial port DTR and RTS lines and is able to measure the resistances of
 * the first four valuator axes and the state of the first four buttons,
 * sending the digitized data across a standard serial port in 6-byte data
 * packets.  No other special joystick features (extra buttons, hat switches,
 * force feedback, etc.) are supported.<p>
 * 
 * NOTE: The workstation gameport device was originally manufactured by
 * Colorado Spectrum for Unix workstations in the mid-1990's.  Although many
 * units were sold to Sun, SGI, HP, DEC, and other Unix vendors at the time,
 * it may no longer be available.<p>
 * 
 * This implementation uses native code to communicate with the trackers
 * through standard serial ports at 9600 baud.  The associated
 * <code>libj3dInput</code> native code library must be available through
 * the application library search path at runtime.<p>
 * 
 * Two <code>Sensors</code> are created for each <code>Gameport</code>
 * instance to handle the two (X, Y) 2D valuator pairs.  The (X, Y) values for
 * each Sensor are passed in the X and Y translation components of the
 * <code>Transform3D</code> associated with a sensor read.  These values are
 * scaled to a normalized floating point range of [-1.0 .. +1.0].<p>
 * 
 * <b>calibration</b><p>
 *
 * Since analog PC joysticks typically use inexpensive potentiometers that
 * drift over small periods of time, calibration is usually required. A
 * <code>Gameport</code> instance offers basic support for centering to
 * neutral joystick values through the <code>center</code> method, and full
 * range calibration with the <code>calibrate</code> method.  These methods
 * require user interaction since the gameport interface delivers events only
 * when a device value has changed.  Applications that wish to implement their
 * own calibration protocols will need to call the <code>getRawEvent</code>
 * method to obtain unscaled and uncentered gameport axis values.<p>
 *
 * The calibration properties or accessor methods
 * (<code>Axis**Calibration</code> and <code>setAxis**Calibration</code>) can
 * be used if the approximate calibration values are already known for a
 * particular gameport device.  Full range calibration is usually not
 * critical, but interactive recentering is recommended whenever the
 * application starts up.<p>
 *
 * @see SerialDevice
 */
public class Gameport extends SerialDevice implements EventDrivenInputDevice {
    
    // The four accessable button values.
    private int[] buttons = new int[4] ;

    // Raw joystick unsigned byte values converted to double by native layer.
    private double[] p1 = new double[2] ;
    private double[] p2 = new double[2] ;

    // Sensor data.
    private Vector3d v3d = new Vector3d() ;
    private Transform3D t3d = new Transform3D() ;
    private Sensor[] sensor = new Sensor[2] ;

    // Property values.
    private boolean centerOnInitialize = false ;
    private boolean calibrateOnInitialize = false ;
    private double thresholdRadius2 = 0.0 ;

    // Holds calibration information for a joystick axis.
    private static class AxisInfo {
	// Nominal values for calibration.
	double low     =   0.0 ;
	double neutral = 128.0 ;
	double high    = 255.0 ;
	double scale = 1.0 / 127.0 ;

	public String toString() {
	    return "low " + low + " neutral " + neutral + " high " + high ;
	}
    }

    // Calibration info for each supported axis valuator.
    private AxisInfo x1Info = new AxisInfo() ;
    private AxisInfo y1Info = new AxisInfo() ;
    private AxisInfo x2Info = new AxisInfo() ;
    private AxisInfo y2Info = new AxisInfo() ;

    // Event agent to use in event driven mode.
    private SensorEventAgent eventAgent = null ;

    // Get the current values from the device.
    native int getEvents
	(long ctx, int deviceIndex, double[] p1, double[] p2, int[] buttons) ;

    /**
     * A parameterless constructor for this <code>InputDevice</code>.  This 
     * is used for <code>ConfiguredUniverse</code>, which requires such a
     * constructor for configurable input devices.  The serial port assignment
     * must be specified with the <code>SerialPort</code> property.
     * <p>
     * <b>Syntax:</b><br>(NewDevice <i>&lt;name&gt;</i>
     * com.sun.j3d.input.Gameport)
     *
     * @see SerialDevice
     */
    public Gameport() {
	super("Gameport") ;

	// Each instance has two 2D valuator Sensors.
	sensor[0] = new Sensor(this, 30, 4) ;
	sensor[1] = new Sensor(this, 30, 4) ;

	// Set the initial sensor read values.
	sensor[0].setNextSensorRead(System.currentTimeMillis(), t3d, buttons) ;
	sensor[1].setNextSensorRead(System.currentTimeMillis(), t3d, buttons) ;
    }

    /**
     * Creates a new <code>Gameport</code> instance.<p>
     *
     * @param portName name of the serial port to which this instance is
     *  attached
     * @exception <code>IllegalArgumentException</code> if port assignment
     *  fails
     */
    public Gameport(String portName) {
	this() ;
	super.setSerialPort(portName) ;
    }

    /**
     * Initializes the device.  A device should be initialized before it is
     * registered with Java 3D via the
     * <code>PhysicalEnvironment.addInputDevice</code> method call.<p>
     * 
     * @return true for successful initialization, false for failure
     * @exception <code>IllegalStateException</code> if a port name has not
     *  been specified
     * @exception <code>RuntimeException</code> if there is an error opening
     *  the serial port
     */
    public boolean initialize() {
	super.openPorts() ;

	if (centerOnInitialize)
	    center() ;
	else if (calibrateOnInitialize)
	    calibrate() ;

	return true ;
    }

    /**
     * Sets the device's current position and orientation as the device's
     * nominal position and orientation.  This method does nothing in the
     * <code>Gameport</code> implementation.  The gameport interface delivers
     * events only when an axis or button value changes, so user interaction
     * is required to set the nominal position and orientation.  The
     * <code>center</code> method should be be used instead to calibrate the
     * neutral positions for the gameport axes.
     */
    public void setNominalPositionAndOrientation() {
    }

    /**
     * Causes the device's sensor readings to be updated by the device driver.
     * This is called by the Java 3D input device scheduler.
     */
    public void pollAndProcessInput() {
	if (! open)
	    throw new IllegalStateException
		("\nAttempt to read a device that is not open.") ;

	if (getRawEvent(p1, p2, buttons)) {
	    double x1 = (p1[0] - x1Info.neutral) * x1Info.scale ;
	    if (x1 < -1.0) x1 = -1.0 ;
	    else if (x1 >  1.0) x1 =  1.0 ;

	    double y1 = (y1Info.neutral - p1[1]) * y1Info.scale ;
	    if (y1 < -1.0) y1 = -1.0 ;
	    else if (y1 >  1.0) y1 =  1.0 ;

	    v3d.set(x1, y1, 0) ;
	    if (v3d.lengthSquared() < thresholdRadius2)
		v3d.set(0, 0, 0) ;

	    t3d.set(v3d) ;
	    sensor[0].setNextSensorRead
		(System.currentTimeMillis(), t3d, buttons) ;

	    double x2 = (p2[0] - x2Info.neutral) * x2Info.scale ;
	    if (x2 < -1.0) x2 = -1.0 ;
	    else if (x2 >  1.0) x2 =  1.0 ;

	    double y2 = (y2Info.neutral - p2[1]) * y2Info.scale ;
	    if (y2 < -1.0) y2 = -1.0 ;
	    else if (y2 >  1.0) y2 =  1.0 ;

	    v3d.set(x2, y2, 0) ;
	    if (v3d.lengthSquared() < thresholdRadius2)
		v3d.set(0, 0, 0) ;

	    t3d.set(v3d) ;
	    sensor[1].setNextSensorRead
		(System.currentTimeMillis(), t3d, buttons) ;

	    // Dispatch events if in event driven mode.
	    if (eventAgent != null)
		eventAgent.dispatchEvents() ;
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
     * <code>Gameport</code> implementation only supports
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
     * Retrieves the device's processing mode.  For the <code>Gameport</code>,
     * this is always <code>NON_BLOCKING</code>.
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
     * A <code>Gameport</code> provides two <code>Sensor</code> instances: the
     * first places the (X1, Y1) axis values in the (X, Y) translation
     * components of its sensor read matrix, while the second places the (X2,
     * Y2) axis values in the (X, Y) translation components of its sensor read
     * matrix.  These values are normalized to the range [-1.0 .. +1.0].<p>
     *
     * Two sensors are always available.  If the joystick supports only two
     * axes, then the second sensor's read values are undefined.
     * 
     * @param sensorIndex the sensor to retrieve
     * @return the specified sensor
     */
    public Sensor getSensor(int index) {
	if (index < 0 || index > 1)
	    throw new IllegalArgumentException
		("Sensor index must be 0 or 1 for Gameport") ;

	return sensor[index] ;
    }

    /**
     * Gets the number of sensors associated with the device.
     * @return 2
     */
    public int getSensorCount() {
	return 2 ;
    }

    /**
     * Cleans up the device and relinquishes associated resources.  This
     * method should only be called after the device has been unregistered
     * from Java 3D via the <code>PhysicalEnvironment.removeInputDevice</code>
     * method call.
     */
    public void close() {
	super.close() ;
    }

    /**
     * Puts the device into <i>event driven</i> mode.  The sensor bindings
     * encapsulated in the given <code>SensorEventAgent</code> will be used to
     * invoke sensor event handlers whenever <code>pollAndProcessInput</code>
     * is called by the input device scheduler thread and a new event has been
     * generated by the device.
     *
     * @param agent the SensorEventAgent to use; if <code>null</code>, event
     *  driven mode is disabled
     */
    public void setSensorEventAgent(SensorEventAgent agent) {
	this.eventAgent = agent ;
    }

    /**
     * Returns the <code>SensorEventAgent</code> use by this device; if
     * <code>null</code>, then this device is not in event driven mode.
     *
     * @return the SensorEventAgent in use, or <code>null</code> if not in
     *  event driven mode
     */
    public SensorEventAgent getSensorEventAgent() {
	return eventAgent ;
    }

    /**
     * Calibrates the gameport valuators so that their neutral positions are
     * interpreted as 0.0.  The gameport interface delivers events only when
     * an axis or button value changes, so this method will cause the current
     * thread to poll the the gameport interface every 100ms until the user
     * clicks a gameport button.<p>
     *
     * The neutral values delivered will override the calibration information
     * set with the property and accessor methods.  
     */
    public void center() {
	JWindow window = createPrompt
	    ("Gameport: centering calibration in progress.",
	     "Click a gameport button in neutral position to continue...") ;

	pollForButton(p1, p2, buttons) ;
	window.dispose() ;

	calibrateAxis(x1Info, x1Info.low, p1[0], x1Info.high) ;
	calibrateAxis(y1Info, y1Info.low, p1[1], y1Info.high) ;
	calibrateAxis(x2Info, x2Info.low, p2[0], x2Info.high) ;
	calibrateAxis(y2Info, y2Info.low, p2[1], y2Info.high) ;

	System.out.println("Axis X1 calibration: " + x1Info) ;
	System.out.println("Axis Y1 calibration: " + y1Info) ;
	System.out.println("Axis X2 calibration: " + x2Info) ;
	System.out.println("Axis Y2 calibration: " + y2Info) ;
    }

    private JWindow createPrompt(String s1, String s2) {
	Box box = javax.swing.Box.createVerticalBox() ;
	box.setBorder(BorderFactory.createRaisedBevelBorder()) ;

	if (s1 != null) 
	    box.add(new JLabel(s1)) ;

	if (s2 != null)
	    box.add(new JLabel(s2)) ;

	JWindow window = new JWindow() ;
	window.getContentPane().add(box) ;

	Dimension scrnSize = Toolkit.getDefaultToolkit().getScreenSize() ;
	Dimension boxSize = box.getPreferredSize() ;
	window.setLocation(scrnSize.width/2  - boxSize.width/2,
			   scrnSize.height/2 - boxSize.height/2) ;
	window.pack() ;
	window.show() ;
	return window ;
    }

    /**
     * Calibrates the gameport valuators so that their low and high positions
     * are normalized to the [-1.0, +1.0] range and their neutral, centered
     * positions are interpreted as 0.0.  The gameport interface delivers
     * events only when an axis or button value changes, so this method will
     * cause the current thread to poll the the gameport interface every 100ms
     * while the user clicks gameport buttons for the neutral, low, and high
     * device values.<p>
     *
     * The calibration values delivered will override the calibration
     * information set with the property and accessor methods.
     */
    public void calibrate() {
	JWindow window = createPrompt
	    ("Gameport: range calibration in progress.",
	     "Click a gameport button in neutral position to continue...") ;

	pollForButton(p1, p2, buttons) ;
	window.dispose() ; 

	x1Info.neutral = p1[0] ; y1Info.neutral = p1[1] ;
	x2Info.neutral = p2[0] ; y2Info.neutral = p2[1] ;

	window = createPrompt
	    ("Move axes to their low positions (left/forward) and click...",
	     null) ;

	pollForButton(p1, p2, buttons) ;
	window.dispose() ;

	x1Info.low = p1[0] ; y1Info.low = p1[1] ;
	x2Info.low = p2[0] ; y2Info.low = p2[1] ;

	window = createPrompt
	    ("Move axes to their high positions (right/back) and click...",
	     null) ;

	pollForButton(p1, p2, buttons) ;
	window.dispose() ;

	x1Info.high = p1[0] ; y1Info.high = p1[1] ;
	x2Info.high = p2[0] ; y2Info.high = p2[1] ;

	calibrateAxis(x1Info, x1Info.low, x1Info.neutral, x1Info.high) ;
	calibrateAxis(y1Info, y1Info.low, y1Info.neutral, y1Info.high) ;
	calibrateAxis(x2Info, x2Info.low, x2Info.neutral, x2Info.high) ;
	calibrateAxis(y2Info, y2Info.low, y2Info.neutral, y2Info.high) ;

	System.out.println("Axis X1 calibration: " + x1Info) ;
	System.out.println("Axis Y1 calibration: " + y1Info) ;
	System.out.println("Axis X2 calibration: " + x2Info) ;
	System.out.println("Axis Y2 calibration: " + y2Info) ;
    }

    // Poll every 100ms for a button press.
    private void pollForButton(double[] p1, double[] p2, int[] buttons) {
	while ((getRawEvent(p1, p2, buttons) == false) || 
	       (buttons[0] == 0 && buttons[1] == 0 &&
		buttons[2] == 0 && buttons[3] == 0)) {

	    try {
		Thread.sleep(100) ;
	    } catch (InterruptedException e) {
		throw new RuntimeException
		    ("joystick calibration polling interrupted") ;
	    }
	}
    }

    // Sets calibration info for the given axis.
    private void calibrateAxis(AxisInfo a, double low,
			       double neutral, double high) {
	a.low = low ;
	a.neutral = neutral ;
	a.high = high ;

	double dLow = neutral - low ;
	double dHigh = high - neutral ;

	if (dLow == 0.0 || dHigh == 0.0)
	    a.scale = 1.0 / 255.0 ;
	else if (dLow < dHigh)
	    a.scale = 1.0 / dLow ;
	else
	    a.scale = 1.0 / dHigh ;
    }

    /**
     * Property which indicates whether the Gameport instance should perform a
     * centering calibration when initialized.  This is accomplished by
     * calling the <code>center</code> method.<p>
     * 
     * This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.<p>
     * 
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * CenterOnInitialize [true | false])
     * 
     * @param value array of length 1 containing a <code>Boolean</code>
     * @see #center
     * @see #setCenterOnInitialize
     * @see #calibrate
     * @see #CalibrateOnInitialize CalibrateOnInitialize()
     * @see #setCalibrateOnInitialize
     */
    public void CenterOnInitialize(Object[] value) {
        if (! (value.length == 1 && value[0] instanceof Boolean))
            throw new IllegalArgumentException
                ("\nCenterOnInitialize must be a Boolean") ;
        
	setCenterOnInitialize(((Boolean)value[0]).booleanValue()) ;
    }

    /**
     * Indicates whether the Gameport instance should perform a centering
     * calibration when initialized.  This is accomplished by calling the
     * <code>center</code> method.<p>
     * 
     * @param value <code>true</code> if centering calibration should be
     *  performed when initialized, <code>false</code> otherwise
     * @see #center
     * @see #CenterOnInitialize CenterOnInitialize()
     * @see #calibrate
     * @see #CalibrateOnInitialize CalibrateOnInitialize()
     * @see #setCalibrateOnInitialize
     */
    public void setCenterOnInitialize(boolean value) {
	centerOnInitialize = value ;
    }

    /**
     * Property which indicates whether the Gameport instance should perform a
     * full range calibration when initialized.  This is accomplished by
     * calling the <code>calibrate</code> method.<p>
     * 
     * This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.<p>
     * 
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * CalibrateOnInitialize [true | false])
     * 
     * @param value array of length 1 containing a <code>Boolean</code>
     * @see #calibrate
     * @see #setCalibrateOnInitialize
     * @see #center
     * @see #CenterOnInitialize CenterOnInitialize()
     * @see #setCenterOnInitialize
     */
    public void CalibrateOnInitialize(Object[] value) {
        if (! (value.length == 1 && value[0] instanceof Boolean))
            throw new IllegalArgumentException
                ("\nCalibrateOnInitialize must be a Boolean") ;
        
	setCalibrateOnInitialize(((Boolean)value[0]).booleanValue()) ;
    }

    /**
     * Indicates whether the Gameport instance should perform a full range
     * calibration when initialized.  This is accomplished by calling the
     * <code>calibrate</code> method.<p>
     * 
     * @param value <code>true</code> if full range calibration should be
     *  performed when initialized, <code>false</code> otherwise
     * @see #calibrate
     * @see #CalibrateOnInitialize CalibrateOnInitialize()
     * @see #center
     * @see #CenterOnInitialize CenterOnInitialize()
     * @see #setCenterOnInitialize
     */
    public void setCalibrateOnInitialize(boolean value) {
	calibrateOnInitialize = value ;
    }

    /**
     * Property which sets the X1 axis calibration.  The X1 axis is usually
     * mapped to the left and right motion of the first joystick, although
     * this may actually map to the rotation of a steering wheel or some
     * other interface to the sensing hardware.<p>
     *
     * This property has three double precision floating point parameters.
     * These values are raw gameport unsigned byte values converted to double,
     * ranging from 0.0 to 255.0.  The <i>low</i> parameter is the smallest
     * value the device can generate on the X1 axis and should be close to 0.0
     * when the stick is moved to its leftmost position.  The <i>high</i>
     * parameter is the largest value that can be generated when the stick is
     * moved to its rightmost position and should be close to 255.0.  The
     * <i>neutral</i> parameter is the value the device reports in its neutral
     * centered state.<p>
     *
     * The calibration values are used to map the usable device axis range to
     * a normalized range of [-1.0 .. +1.0].  The <i>neutral</i> parameter
     * setting maps to 0.0.  Values ranging between <i>neutral</i> and
     * <i>high</i> are mapped to to positive numbers, while values ranging
     * from <i>low</i> to <i>neutral</i> are mapped to negative numbers.<p>
     *
     * In order to obtain equal scaling on both sides of 0.0, the smaller of
     * the positive and negative ranges reported by the device is used as the
     * unit length, and the larger range is clamped to unity.  As a special
     * case, if one of the ranges is 0.0 (when <code>(high - neutral) ==
     * 0.0</code> or <code>(neutral - low) == 0.0</code>), then the non-zero
     * range is mapped to unity instead.<p>
     * 
     * This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.<p>
     * 
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * AxisX1Calibration <i>&lt;low&gt;</i> <i>&lt;neutral&gt;</i>
     * <i>&lt;high&gt;</i>)
     * 
     * @param ranges array of length 3; first element is a <code>Double</code>
     *  for the <i>low</i> value, the second is a <code>Double</code> for the
     *  <i>neutral</i> value, and the third is a <code>Double</code> for the
     *  <i>high</i> value
     * @see #setAxisX1Calibration
     */
    public void AxisX1Calibration(Object[] ranges) {
        if (! (ranges.length == 3 && ranges[0] instanceof Double &&
               ranges[1] instanceof Double && ranges[2] instanceof Double))
            throw new IllegalArgumentException
                ("\nAxisX1Calibration must be 3 numbers: low, mid, and high") ;
        
	setAxisX1Calibration(((Double)ranges[0]).doubleValue(),
			     ((Double)ranges[1]).doubleValue(),
			     ((Double)ranges[2]).doubleValue()) ;
    }

    /**
     * Sets the X1 axis calibration values.
     *
     * @param low the smallest value the device can generate on the X1 axis;
     *  should be close to 0.0 when the stick is moved to its leftmost position
     * @param neutral the value the device reports in its neutral centered
     *  state
     * @param high the largest value that can be generated when the stick is
     *  moved to its rightmost position; should be close to 255.0
     * @see #AxisX1Calibration AxisX1Calibration()
     */
    public void setAxisX1Calibration(double low, double neutral, double high) {
	calibrateAxis(x1Info, low, neutral, high) ;
    }

    /**
     * Property which sets the Y1 axis calibration.  The Y1 axis is usually
     * mapped to the front and back motion of the first joystick.<p>
     *
     * This property has three double precision floating point parameters.
     * These values are raw gameport unsigned byte values converted to double,
     * ranging from 0.0 to 255.0.  The <i>low</i> parameter is the smallest
     * value the device can generate on the Y1 axis and should be close to 0.0
     * when the stick is moved to its most forward position.  The <i>high</i>
     * parameter is the largest value that can be generated when the stick is
     * moved to its backmost position and should be close to 255.0.  The
     * <i>neutral</i> parameter is the value the device reports in its neutral
     * centered state.<p>
     *
     * The calibration values are used to map the usable device axis range to
     * a normalized range of [-1.0 .. +1.0].  The <i>neutral</i> parameter
     * setting maps to 0.0.  In order to conform to Java 3D coordinate
     * conventions, values ranging between <i>neutral</i> and <i>high</i> -
     * where the stick is held back from the neutral position - are mapped to
     * <i>negative</i> numbers, while values ranging from <i>low</i> to
     * <i>neutral</i> - where the stick is held forward of the neutral
     * position - are mapped to <i>positive</i> numbers.<p>
     *
     * Equal scaling on both sides of 0.0 is obtained by taking the smaller of
     * the positive and negative ranges reported by the device and using that
     * as the unit length, while clamping the larger range to unity.  As a
     * special case, if one of the ranges is 0.0 (when <code>(high - neutral)
     * == 0.0</code> or <code>(neutral - low) == 0.0</code>), then the
     * non-zero range is mapped to unity instead.<p>
     * 
     * This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.<p>
     * 
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * AxisY1Calibration <i>&lt;low&gt;</i> <i>&lt;neutral&gt;</i>
     * <i>&lt;high&gt;</i>)
     * 
     * @param ranges array of length 3; first element is a <code>Double</code>
     *  for the <i>low</i> value, the second is a <code>Double</code> for the
     *  <i>neutral</i> value, and the third is a <code>Double</code> for the
     *  <i>high</i> value
     * @see #setAxisY1Calibration
     */
    public void AxisY1Calibration(Object[] ranges) {
        if (! (ranges.length == 3 && ranges[0] instanceof Double &&
               ranges[1] instanceof Double && ranges[2] instanceof Double))
            throw new IllegalArgumentException
                ("\nAxisY1Calibration must be 3 numbers: low, mid, and high") ;
        
	setAxisY1Calibration(((Double)ranges[0]).doubleValue(),
			     ((Double)ranges[1]).doubleValue(),
			     ((Double)ranges[2]).doubleValue()) ;
    }

    /**
     * Sets the Y1 axis calibration values.
     *
     * @param low the smallest value the device can generate on the Y1 axis;
     *  should be close to 0.0 when the stick is moved to its forward position
     * @param neutral the value the device reports in its neutral centered
     *  state
     * @param high the largest value that can be generated when the stick is
     *  moved to its backmost position; should be close to 255.0
     * @see #AxisY1Calibration AxisY1Calibration()
     */
    public void setAxisY1Calibration(double low, double neutral, double high) {
	calibrateAxis(y1Info, low, neutral, high) ;
    }

    /**
     * Property which sets the X2 axis calibration.  The X2 axis is often
     * mapped to the left and right motion of a second joystick, although a
     * mapping to simulated rudder or vehicle pedals is also fairly common.
     * The property component values are the same as those discussed in the
     * documentation for the <code>AxisX1Calibration</code> property.<p>
     * 
     * This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.<p>
     * 
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * AxisX2Calibration <i>&lt;low&gt;</i> <i>&lt;neutral&gt;</i>
     * <i>&lt;high&gt;</i>)
     * 
     * @param ranges array of length 3; first element is a <code>Double</code>
     *  for the <i>low</i> value, the second is a <code>Double</code> for the
     *  <i>neutral</i> value, and the third is a <code>Double</code> for the
     *  <i>high</i> value
     * @see #AxisX1Calibration AxisX1Calibration()
     * @see #setAxisX2Calibration
     */
    public void AxisX2Calibration(Object[] ranges) {
        if (! (ranges.length == 3 && ranges[0] instanceof Double &&
               ranges[1] instanceof Double && ranges[2] instanceof Double))
            throw new IllegalArgumentException
                ("\nAxisX2Calibration must be 3 numbers: low, mid, and high") ;
        
	setAxisX2Calibration(((Double)ranges[0]).doubleValue(),
			     ((Double)ranges[1]).doubleValue(),
			     ((Double)ranges[2]).doubleValue()) ;
    }

    /**
     * Sets the X2 axis calibration values.
     *
     * @param low the smallest value the device can generate on the X2 axis;
     *  should be close to 0.0 when the stick is moved to its leftmost position
     * @param neutral the value the device reports in its neutral centered
     *  state
     * @param high the largest value that can be generated when the stick is
     *  moved to its rightmost position; should be close to 255.0
     * @see #AxisX1Calibration AxisX1Calibration()
     * @see #AxisX2Calibration AxisX2Calibration()
     */
    public void setAxisX2Calibration(double low, double neutral, double high) {
	calibrateAxis(x2Info, low, neutral, high) ;
    }

    /**
     * Property which sets the Y2 axis calibration.  The Y2 axis is often
     * mapped to the front and back motion of a second joystick, although it
     * is also common to map the Y2 axis to a velocity throttle or to a Z
     * dimension.  The property component values are the same as those
     * discussed in the documentation for the <code>AxisY1Calibration</code>
     * property.<p>
     *
     * This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.<p>
     * 
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * AxisY2Calibration <i>&lt;low&gt;</i> <i>&lt;neutral&gt;</i>
     * <i>&lt;high&gt;</i>)
     * 
     * @param ranges array of length 3; first element is a <code>Double</code>
     *  for the <i>low</i> value, the second is a <code>Double</code> for the
     *  <i>neutral</i> value, and the third is a <code>Double</code> for the
     *  <i>high</i> value
     * @see #AxisY1Calibration AxisY1Calibration()
     * @see #setAxisY2Calibration
     */
    public void AxisY2Calibration(Object[] ranges) {
        if (! (ranges.length == 3 && ranges[0] instanceof Double &&
               ranges[1] instanceof Double && ranges[2] instanceof Double))
            throw new IllegalArgumentException
                ("\nAxisY2Calibration must be 3 numbers: low, mid, and high") ;
        
	setAxisY2Calibration(((Double)ranges[0]).doubleValue(),
			     ((Double)ranges[1]).doubleValue(),
			     ((Double)ranges[2]).doubleValue()) ;
    }

    /**
     * Sets the Y2 axis calibration values.
     *
     * @param low the smallest value the device can generate on the Y2 axis;
     *  should be close to 0.0 when the stick is moved to its forward position
     * @param neutral the value the device reports in its neutral centered
     *  state
     * @param high the largest value that can be generated when the stick is
     *  moved to its backmost position; should be close to 255.0
     * @see #AxisY1Calibration AxisY1Calibration()
     * @see #AxisY2Calibration AxisY2Calibration()
     */
    public void setAxisY2Calibration(double low, double neutral, double high) {
	calibrateAxis(y2Info, low, neutral, high) ;
    }

    /**
     * Property which sets the deadzone threshold radius.  This is a double
     * value between 0.0 and 1.0 used to filter noisy device valuators.
     * Any calibrated (X, Y) axis pair value that is less than this
     * distance from (0.0, 0.0) will be reported as (0.0, 0.0) in the sensor
     * read matrix for that axis pair.<p>
     * 
     * This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.<p>
     * 
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * ThresholdRadius <i>&lt;radius&gt;</i>)
     * 
     * @param radius array of length 1 containing a <code>Double</code>
     *  ranging in value from 0.0 to 1.0 for the deadzone threshold radius
     * @see #setThresholdRadius
     */
    public void ThresholdRadius(Object[] radius) {
        if (! (radius.length == 1 && radius[0] instanceof Double))
            throw new IllegalArgumentException
                ("\nThresholdRadius must be a number") ;
        
	setThresholdRadius(((Double)radius[0]).doubleValue()) ;
    }

    /**
     * Sets the deadzone threshold radius.  This is a double value between 0.0
     * and 1.0 used to handle noisy device valuators.  Any calibrated (X,
     * Y) axis pair value that is less than this distance from (0.0, 0.0)
     * will be reported as (0.0, 0.0) in the sensor read matrix for that axis
     * pair.<p>
     * 
     * @param radius deadzone threshold radius ranging from 0.0 to 1.0
     * @see #ThresholdRadius ThresholdRadius()
     */
    public void setThresholdRadius(double radius) {
	if (radius < 0.0 || radius > 1.0)
            throw new IllegalArgumentException
                ("\nthreshold radius must range from 0.0 to 1.0") ;
        
	thresholdRadius2 = radius * radius ;
    }

    /**
     * Gets the next available uncalibrated gameport values and button state.
     * These values are raw gameport unsigned byte values converted to double,
     * ranging from 0.0 to 255.0.  This method is only intended for use by
     * applications that wish to implement their own calibration protocols.<p>
     * 
     * The values returned are valid only if the method returns
     * <code>true</code>.  A <code>false</code> return indicates that a new
     * event is not yet available and that the application should wait some
     * small amount of time and try again.  The gameport generates new events
     * only when an axis value or a button changes state, so user interaction
     * is required to receive new values.  Only the most recent event is
     * returned.  If an application waits too long in its polling loop it
     * may miss some button state transitions.
     *
     * @param p1 a double array of length 2 to receive the raw X1, Y1 axis
     *  values
     * @param p2 a double array of length 2 to receive the raw X2, Y2 axis
     *  values
     * @param buttons an int array of length 4 to receive the button
     *  state; a 1 value in the button array indicates the corresponding
     *  button is down
     * @return <code>true</code> if an event was available; <code>false</code>
     *  if a complete event packet was not read
     */
    public boolean getRawEvent(double[] p1, double[] p2, int[] buttons) {
	// TODO:  put serial driver in blocking mode to avoid polling.
	return getEvents(nativeContext, id, p1, p2, buttons) == 1 ;
    }
}
