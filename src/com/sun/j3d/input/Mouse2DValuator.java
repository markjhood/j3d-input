/*
 *	@(#)Mouse2DValuator.java 1.4 03/05/21 14:05:25
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

import java.awt.event.MouseEvent ;
import java.awt.AWTEvent ;
import java.awt.Component ;
import java.awt.Rectangle ;
import java.util.ArrayList ;
import javax.media.j3d.* ;
import javax.swing.event.MouseInputAdapter ;
import javax.vecmath.* ;
import com.sun.j3d.utils.behaviors.sensor.SensorEventAgent ;

//
// TODO: documentation
//       allow initial invisibility
// 
public class Mouse2DValuator extends MouseInputAdapter
    implements EventDrivenInputDevice {

    int[] buttons = new int[] {0, 0, 0} ;
    ArrayList queue = new ArrayList() ;
    Vector3d v3d = new Vector3d() ;
    Transform3D t3d = new Transform3D() ;
    Sensor sensor = new Sensor(this, 30, 3) ;
    Rectangle rv = new Rectangle() ;
    ArrayList components = new ArrayList() ;
    SensorEventAgent eventAgent = null ;
    int x, y, originX, originY ;
    double scale ;

    public Mouse2DValuator() {
    }

    public Mouse2DValuator(Component component) {
	this(new Component[] {component}) ;
    }

    public Mouse2DValuator(Component[] components) {
	setComponents(components) ;
    }     
    
    public synchronized void setComponents(Component[] components) {
	for (int i = 0 ; i < components.length ; i++)
	    this.components.add(components[i]) ;
    }

    public void Components(Object[] components) {
	for (int i = 0 ; i < components.length ; i++) {
	    if (! (components[i] instanceof Component))
		throw new IllegalArgumentException
		    ("Property arguments must all be AWT Components") ;

	    this.components.add((Component)components[i]) ;
	}
    }

    public boolean initialize() {
	int maxDimension = 0 ;
	for (int i = 0 ; i < components.size() ; i++) {
	    Component component = (Component)components.get(i) ;
	    component.addMouseListener(this) ;
	    component.addMouseMotionListener(this) ;
	    component.getBounds(rv) ;
	    if (rv.width > maxDimension) maxDimension = rv.width ;
	    if (rv.height > maxDimension) maxDimension = rv.height ;
	}

	scale = 2.0 / maxDimension ;
	return true ;
    }

    public void setNominalPositionAndOrientation() {
    }

    public void processStreamInput() {
    }

    public void setProcessingMode(int mode) {
	if (mode != InputDevice.NON_BLOCKING)
	    throw new IllegalArgumentException("mode must be NON_BLOCKING") ;
    }

    public int getProcessingMode() {
	return(InputDevice.NON_BLOCKING) ;
    }

    public Sensor getSensor(int index) {
	if (index != 0)
	    throw new IllegalArgumentException("Sensor index must be 0") ;

	return sensor ;
    }

    public int getSensorCount() {
	return 1 ;
    }

    public void close() {
    }

    public void setSensorEventAgent(SensorEventAgent agent) {
	this.eventAgent = agent ;
    }

    public SensorEventAgent getSensorEventAgent() {
	return eventAgent ;
    }

    public void pollAndProcessInput() {
	// Access to queue must be synchronized.
	AWTEvent[] events = null ;
	synchronized(queue) {
	    events = (AWTEvent[])queue.toArray(new AWTEvent[queue.size()]) ;
	    queue.clear() ;
	}

	processEvents(events, v3d) ;
	t3d.set(v3d) ;
	sensor.setNextSensorRead(System.currentTimeMillis(), t3d, buttons) ;

	if (eventAgent != null && events.length > 0)
	    eventAgent.dispatchEvents() ;
    }

    final void setButtons(MouseEvent m, int[] buttons) {
	if (!m.isAltDown() && !m.isMetaDown())
	    if (m.getID() == MouseEvent.MOUSE_PRESSED)
		buttons[0] = 1 ;
	    else
		buttons[0] = 0 ;

	if (m.isAltDown() && !m.isMetaDown())
	    if (m.getID() == MouseEvent.MOUSE_PRESSED)
		buttons[1] = 1 ;
	    else
		buttons[1] = 0 ;

	if (!m.isAltDown() && m.isMetaDown())
	    if (m.getID() == MouseEvent.MOUSE_PRESSED)
		buttons[2] = 1 ;
	    else
		buttons[2] = 0 ;
    }

    void processEvents(AWTEvent events[], Vector3d v3d) {
	for (int i = 0 ; i < events.length ; i++) {
	    if (events[i] instanceof MouseEvent) {
		MouseEvent m = (MouseEvent)events[i] ;
		if (m.getID() == MouseEvent.MOUSE_PRESSED) {
		    x = y = 0 ;
		    originX = m.getX() ;
		    originY = m.getY() ;
		    setButtons(m, buttons) ;
		}
		else if (m.getID() == MouseEvent.MOUSE_RELEASED) {
		    setButtons(m, buttons) ;
		}
		else if (m.getID() == MouseEvent.MOUSE_DRAGGED) {
		    x = m.getX() - originX ;
		    y = m.getY() - originY ;
		}
	    }
	}

	if (buttons[0] == 1 || buttons[1] == 1 || buttons[2] == 1) {
	    v3d.set(x * scale, -y *scale, 0.0) ;
	}
	else {
	    v3d.set(0.0, 0.0, 0.0) ;
	}
    }

    void queueAWTEvent(AWTEvent e) {
	// Access to queue must be synchronized.
	synchronized (queue) {
	    queue.add(e) ;
	}
    }
    
    public void mouseClicked(final MouseEvent e) {
        queueAWTEvent(e) ;
    }
    
    public void mousePressed(final MouseEvent e) {
        queueAWTEvent(e) ;
    }

    public void mouseReleased(final MouseEvent e) {
        queueAWTEvent(e) ;
    }

    public void mouseDragged(final MouseEvent e) {
        queueAWTEvent(e) ;
    }
}

