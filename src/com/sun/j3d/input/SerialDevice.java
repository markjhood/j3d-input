/*
 *	@(#)SerialDevice.java 1.11 02/10/07 19:56:49
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
import java.util.* ;

/**
 * The base class for <code>com.sun.j3d.input.InputDevice</code> classes that
 * use serial ports for communication.  No public constructors are provided
 * for this class.
 */
public class SerialDevice {
    static final int USE_RSB               =  0 ;
    static final int SERIAL_PORT_FILE_NAME =  1 ;
    static final int LAST_ATTRIBUTE        =  1 ;

    static long nativeContext = 0 ;
    static List devices = new ArrayList() ;

    int id = -1 ;
    boolean open = false ;
    String portName = null ;

    // Inits the serial device package, returns malloc'ed context pointer.
    static native long initSerial() ;

    // Creates another instance of serial device.   Returns internal index
    // number for this device for (-1 for failure).
    native int newDevice(long ctx, String deviceName) ;

    // Sets a numbered attribute of the specified device instance
    // to a double value.
    native int deviceAttribute
	(long ctx, int deviceIndex, int attributeNumber, double val) ;

    // Sets a numbered attribute of the specified device instance
    // to a string value.
    native int deviceAttribute
	(long ctx, int deviceIndex, int attributeNumber, String val) ;

    // Processes all the serial device information deposited,
    // probes and inits the described devices (last call of init sequence).
    // Return final status check.
    native int openProbe(long ctx) ;

    // Closes the device and releases resources.
    native void close(long ctx, int deviceIndex) ;

    /**
     * Load the native code library and initialize the native serial driver
     * context. 
     */
    static {
        java.security.AccessController.doPrivileged(
            new java.security.PrivilegedAction() {
            public Object run() {
                System.loadLibrary("j3dInput") ;
		nativeContext = initSerial() ;
                return null ;
            }
        }) ;
    }

    /**
     * Creates a new <code>SerialDevice</code> instance.<p>
     *
     * @param driverName name of peripheral driver to use; "RedBarron" and
     *  "Gameport" are currently supported
     * @exception <code>IllegalArgumentException</code>
     *  if peripheral driver name is unknown or fails instantiation
     */
    SerialDevice(String driverName) {
	synchronized(devices) {
	    id = newDevice(nativeContext, driverName) ;
	    if (id < 0)
		throw new IllegalArgumentException
		    ("peripheral driver assignment failed") ;

	    devices.add(this) ;
	}
    }

    /**
     * Set the serial port name for this instance.
     * 
     * @param portName name of the serial port to which this instance is
     *  attached
     * @exception <code>IllegalArgumentException</code>
     *  if port assignment fails
     */
    public void setSerialPort(String portName) {
	int ret = deviceAttribute(nativeContext, id,
				  SERIAL_PORT_FILE_NAME, portName) ;
	if (ret == 0)
	    throw new IllegalArgumentException
		("port assignment " + portName + " failed") ;

	this.portName = portName ;
    }

    /**
     * Property which sets the serial port name for this instance.<p>
     * 
     * This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.  Note: the port name must be
     * quoted to handle the forward slashes in Unix path names.<p>
     * 
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * SerialPort "<i>&lt;port name&gt;</i>")
     * 
     * @param portName array of length 1 containing an instance of
     * <code>String</code>
     * @exception <code>IllegalArgumentException</code>
     *  if port assignment fails
     */
    public void SerialPort(Object[] portName) {
	if (! (portName.length == 1 && portName[0] instanceof String))
	    throw new IllegalArgumentException("SerialPort must be a name") ;
	
	setSerialPort((String)portName[0]) ;
    }

    /**
     * Indicates whether the Solaris native Realtime Serial Buffer (RSB)
     * package is installed in the operating environment and should be used to
     * read this device.  This package provides a low-latency shared memory
     * interface to the serial ports.<p>
     *
     * There are a few restrictions on the use of RSB.  First, the RSB package
     * must be installed; second, RSB is only supported on
     * <code>/dev/ttya</code> and <code>/dev/ttyb</code>; and third, if RSB is
     * enabled on one of the supported ports, then it must also be enabled on
     * the other.  If these restrictions are violated, the native layer will
     * report warnings and process RSB assignments appropriately.<p>
     * 
     * @param status true if RSB is available and should be used; false
     *  otherwise
     */
    public void setRealtimeSerialBuffer(boolean status) {
	deviceAttribute(nativeContext, id, USE_RSB, status ? 1.0 : 0.0) ;
    }

    /**
     * Property which indicates whether the Solaris native Realtime Serial
     * Buffer (RSB) package is installed in the operating environment and
     * should be used to read this device.  This package provides a
     * low-latency shared memory interface to the serial ports.<p>
     *
     * There are a few restrictions on the use of RSB.  First, the RSB package
     * must be installed; second, RSB is only supported on
     * <code>/dev/ttya</code> and <code>/dev/ttyb</code>; and third, if RSB is
     * enabled on one of the supported ports, then it must also be enabled on
     * the other.  If these restrictions are violated, the native layer will
     * report warnings and process RSB assignments appropriately.<p>
     * 
     * This property is set in the configuration file read by
     * <code>ConfiguredUniverse</code>.<p>
     * 
     * <b>Syntax:</b><br>(DeviceProperty <i>&lt;name&gt;</i>
     * RealtimeSerialBuffer [true | false])
     * 
     * @param status array of length 1 containing an instance of
     *  <code>Boolean</code>
     */
    public void RealtimeSerialBuffer(Object[] status) {
	if (! (status.length == 1 && status[0] instanceof Boolean))
	    throw new IllegalArgumentException
		("RealtimeSerialBuffer must be a Boolean") ;

	setRealtimeSerialBuffer(((Boolean)status[0]).booleanValue()) ;
    }

    /**
     * Opens and initializes the serial ports for all instances of
     * <code>SerialDevice</code> that have not yet been opened.  Some
     * <code>SerialDevice</code> classes require some or all of their
     * instances to be opened together; if not, this method may be called
     * individually for each instance.
     * 
     * @exception <code>IllegalStateException</code> if a port name has not
     *  been specified for any instance
     * @exception <code>RuntimeException</code> if there is an error opening
     *  the serial ports
     */
    void openPorts() {
	synchronized (devices) {
	    boolean newDevice = false ;
	    Iterator i = devices.iterator() ;

	    while (i.hasNext()) {
		SerialDevice d = (SerialDevice)i.next() ;
		if (!d.open) {
		    newDevice = true ;
		    if (d.portName == null)
			throw new IllegalStateException
			    ("SerialDevice " + d.id + ": no port specified") ;
		}
	    }

	    if (!newDevice) return ;
	    if (openProbe(nativeContext) == 0)
		throw new RuntimeException("error opening serial ports") ;

	    i = devices.iterator() ;
	    while (i.hasNext()) ((SerialDevice)i.next()).open = true ;
	}
    }

    /**
     * Cleans up the device and relinquishes associated resources.
     */
    void close() {
	synchronized (devices) {
	    close(nativeContext, id) ;
	    devices.remove(this) ;
	    id = -1 ;
	    open = false ;
	    portName = null ;
	}
    }
}
