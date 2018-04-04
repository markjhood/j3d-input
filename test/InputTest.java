/*
 *	@(#)InputTest.java 1.17 03/04/15 20:56:41
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

import java.awt.event.KeyAdapter ;
import java.awt.event.KeyEvent ;
import java.net.URL ;
import java.net.MalformedURLException ;
import java.text.DecimalFormat ;
import java.text.FieldPosition ;
import java.util.* ;
import javax.media.j3d.* ;
import javax.vecmath.* ;
import com.sun.j3d.input.* ;
import com.sun.j3d.utils.universe.* ;
import com.sun.j3d.utils.behaviors.sensor.* ;
import com.sun.j3d.utils.behaviors.vp.ViewPlatformBehavior ;

/**
 * Displays InputDevice sensor values graphically and optionally prints them
 * out.  Printout is specified with the `-p' application command line
 * parameter.<p>
 * 
 * This is a ConfiguredUniverse application and requires Java 3D 1.3.1 or
 * later to run.  By default it uses the configuration file
 * `trackd-dummy-test.cfg' in this directory, although other configuration
 * files can be specified as application command line parameters: e.g., `java
 * InputTest gameport-test.cfg'.  Configuration files can also be specified
 * with the `j3d.configURL' java command line property: e.g., `java
 * -Dj3d.configURL=file:mouse-test.cfg InputTest'.<p>
 * 
 * InputTest expects at least one InputDevice implementation to be
 * instantiated in the configuration file along with at least one Sensor.
 * The sensors are then polled with every display frame and their current
 * orientations and most recent positions are displayed.  The current
 * orientation and position of a sensor is displayed with a white gnomon,
 * while its recent previous positions are displayed by a trail of dots.  A
 * green transparent gnomon is used to represent the origin of the virtual
 * world.<p>
 * 
 * This application can also be run in event-driven mode with the `-e' flag.
 * To do so, every InputDevice instantiated in the configuration file must
 * implement EventDrivenInputDevice, which allows the device to drive a
 * SensorEventAgent directly.  In the normal case without the `-e' flag a
 * Behavior which wakes up on every frame will be added to the scenegraph to
 * poll sensor values.  Event-driven mode is useful mainly with mouse or
 * joystick devices; trackers generally generate continuous values and should
 * be polled on each frame.<p>
 * 
 * Sensor button states are only available when the `-p' option is used to
 * print out the sensor reads.<p>
 *
 * The application can be exited by typing `q', `Q', or `ESC'.  It will then
 * attempt an orderly close of all input devices.
 * 
 * @author Mark Hood
 */
public class InputTest {
    BoundingSphere bounds = new BoundingSphere(new Point3d(0.0,0.0,0.0),
					       Double.POSITIVE_INFINITY) ;

    public static void main(String[] args) {
	new InputTest(args) ;
    }

    public InputTest(String[] args) {
	boolean print = false ;
	boolean eventDriven = false ;
	String cfgFile = null ;

	for (int i = 0 ; i < args.length ; i++)
	    if (args[i].equals("-e"))
		eventDriven = true ;
	    else if (args[i].equals("-p"))
		print = true ;
	    else
		cfgFile = args[i] ;

	// Get the config file URL from the command line, the j3d.configURL
	// java property, or use the default file "trackd-dummy-test.cfg" in
	// the current directory.
	if (cfgFile == null) cfgFile = "trackd-dummy-test.cfg" ;

	URL configURL = ConfigContainer.getConfigURL("file:" + cfgFile) ;
	ConfigContainer cfg = new ConfigContainer(configURL, true, 1) ;
	final ConfiguredUniverse u = new ConfiguredUniverse(cfg) ;

	// Get sensors.
	Map sensorMap = cfg.getNamedSensors() ;
	if (sensorMap == null) {
	    throw new IllegalStateException
		("\nNo sensors are defined in configuration file.") ;
	}

	int si = 0 ;
	Iterator it =  sensorMap.entrySet().iterator() ;
	Sensor[] sensors = new Sensor[sensorMap.size()] ;
	String[] names = new String[sensors.length] ;

	System.out.println("InputTest:") ;
	while (it.hasNext()) {
	    Map.Entry e = (Map.Entry)it.next() ;
	    names[si] = (String)e.getKey() ;
	    sensors[si] = (Sensor)e.getValue() ;
	    System.out.println("  Found sensor \"" + names[si] + "\"") ;
	    si++ ;
	}

	// Create a TransformGroup array.
	TransformGroup[] tg = new TransformGroup[sensors.length] ;
	for (int i = 0 ; i < sensors.length ; i++) {
	    tg[i] = new TransformGroup() ;
	}

	// Create a scene graph using the sensor TransformGroups.
	BranchGroup scene = createSceneGraph(tg) ;

	// Create a behavior to display the sensor readings and add it to the
	// scene graph.
	Behavior b = new SensorDisplayBehavior
	    (sensors, names, tg, u.getViewer(), scene, eventDriven, print) ;

	// If we're event-driven, don't use the behavior scheduler thread.
	// The input device will drive the event handler directly from the
	// input scheduler thread.
	if (! eventDriven) {
	    b.setSchedulingBounds(bounds) ;
	    scene.addChild(b) ;
	}

	// Make the scenegraph live.
	u.addBranchGraph(scene) ;

	// Listen for a typed "q", "Q", or "Escape" key on each canvas to quit.
	Canvas3D[] canvases;
	canvases = u.getViewer().getCanvas3Ds();
	final PhysicalEnvironment env =
	    u.getViewer().getPhysicalEnvironment() ;

	class QuitListener extends KeyAdapter {
	    public void keyTyped(KeyEvent e) {
		char c = e.getKeyChar();
		if (c == 'q' || c == 'Q' || c == 27) {
		    /* unregister and close input devices */
		    Enumeration devices = env.getAllInputDevices() ;
		    while (devices.hasMoreElements()) {
			InputDevice id = (InputDevice)devices.nextElement() ;
			env.removeInputDevice(id) ;
			id.close() ;
		    }

		    /* clean up universe resources - test */
		    u.cleanup() ;
		    System.exit(0) ;
		}
	    }
	}

	QuitListener quitListener = new QuitListener();
	for (int i = 0; i < canvases.length; i++)
	    canvases[i].addKeyListener(quitListener);
    }

    BranchGroup createSceneGraph(TransformGroup[] tg) {
	// Create the root of the branch graph
	BranchGroup objRoot = new BranchGroup() ;

	// Place a green transparent gnomon at the origin.
	Shape3D origin = new SensorGnomonEcho(new Transform3D(),
					      0.02, 0.10, true) ;
	Appearance a = origin.getAppearance() ;
	Material m = a.getMaterial() ;
	TransparencyAttributes ta = a.getTransparencyAttributes() ;
	m.setDiffuseColor(new Color3f(0.0f, 1.0f, 0.0f)) ;
	ta.setTransparency(0.5f) ;
	ta.setTransparencyMode(TransparencyAttributes.BLENDED) ;
	objRoot.addChild(origin) ;

	// Add the TransformGroups for the sensor display.
	for (int i = 0 ; i < tg.length ; i++) {
	    tg[i].setCapability(TransformGroup.ALLOW_TRANSFORM_WRITE) ;
	    tg[i].setCapability(TransformGroup.ALLOW_TRANSFORM_READ) ;
	    objRoot.addChild(tg[i]) ;
	}

        // Set up the background
        Color3f bgColor = new Color3f(0.05f, 0.05f, 0.5f) ;
        Background bgNode = new Background(bgColor) ;
        bgNode.setApplicationBounds(bounds) ;
        objRoot.addChild(bgNode) ;

        // Set up the ambient light
        Color3f ambientColor = new Color3f(0.1f, 0.1f, 0.1f) ;
        AmbientLight ambientLightNode = new AmbientLight(ambientColor) ;
        ambientLightNode.setInfluencingBounds(bounds) ;
        objRoot.addChild(ambientLightNode) ;

        // Set up the directional lights
        Color3f light1Color = new Color3f(1.0f, 1.0f, 0.9f) ;
        Vector3f light1Direction  = new Vector3f(1.0f, 1.0f, 1.0f) ;
        Color3f light2Color = new Color3f(1.0f, 1.0f, 1.0f) ;
        Vector3f light2Direction  = new Vector3f(-1.0f, -1.0f, -1.0f) ;

        DirectionalLight light1
            = new DirectionalLight(light1Color, light1Direction) ;
        light1.setInfluencingBounds(bounds) ;
        objRoot.addChild(light1) ;

        DirectionalLight light2
            = new DirectionalLight(light2Color, light2Direction) ;
        light2.setInfluencingBounds(bounds) ;
        objRoot.addChild(light2) ;

	return objRoot ;
    }

    /**
     * Displays sensor values.  For event-driven mode this behavior must not
     * be added to the scenegraph -- the input devices will invoke its event
     * handler directly.
     */
    static class SensorDisplayBehavior
	extends Behavior implements GeometryUpdater {

	SensorEventAgent eventAgent = null ;
	WakeupCondition wakeupConditions = null ;
	Transform3D t3d = new Transform3D() ;
	double[] m = new double[16] ;
	boolean eventDriven = false ;
	boolean print = false ;

	// Points updated from sensors reads.
	Vector3f[] newPoints = null ;
	int curNewPoint = 0 ;

	// Leaves a static trail of points behind each sensor.
	final static int pointCount = 16000 ;
	float[] points = new float[pointCount*3] ;
	PointArray pointArray = null ;
	int curPoint = 0 ;

	SensorDisplayBehavior(Sensor[] s, String[] name,
			      TransformGroup[] tg, Viewer viewer,
			      BranchGroup scene, boolean eventDriven,
			      boolean print) {

	    this.eventAgent = new SensorEventAgent(this) ;
	    this.newPoints = new Vector3f[s.length] ;
	    this.eventDriven = eventDriven ;
	    this.print = print ;

	    for (int i = 0 ; i < s.length ; i++) {
		int bCount = s[i].getSensorButtonCount() ;
		newPoints[i] = new Vector3f() ;
		eventAgent.addSensorReadListener
		    (s[i], new ReadListener(name[i], tg[i], bCount)) ;
	    }

	    if (eventDriven) {
		// The config file should create EventDrivenInputDevice
		// instances which will use the SensorEventAgent directly.
		PhysicalEnvironment env = viewer.getPhysicalEnvironment() ;
		Enumeration devices = env.getAllInputDevices() ;
		while (devices.hasMoreElements()) {
		    Object d = devices.nextElement() ;
		    if (d instanceof EventDrivenInputDevice)
			((EventDrivenInputDevice)d).setSensorEventAgent
			    (eventAgent) ;
		    else
			throw new IllegalArgumentException
			    ("\nevent driven mode specified, but device\n" +
			     d + " is not an EventDrivenInputDevice") ;
		}
	    }

	    pointArray = new PointArray(pointCount,
					GeometryArray.COORDINATES |
					GeometryArray.BY_REFERENCE) ;
					 
	    pointArray.setCoordRefFloat(points) ;
	    pointArray.setCapability(GeometryArray.ALLOW_REF_DATA_WRITE) ;

	    Shape3D pointShape = new Shape3D(pointArray) ;
	    scene.addChild(pointShape) ;
	}

	class ReadListener implements SensorReadListener {
	    String name = null ;
	    TransformGroup tg = null ;
	    int[] buttons = null ;

	    ReadListener(String name, TransformGroup tg, int buttonCount) {
		this.name = name ;
		this.tg = tg ;
		this.buttons = new int[buttonCount] ;
		tg.addChild(new SensorGnomonEcho(t3d, 0.02, 0.10, true)) ;
	    }

	    public void read(SensorEvent e) {
		// This will always be called serially by either the behavior
		// thread or, if event-driven, by the input device thread, so
		// there should be no synchronization issues.
		e.getSensorRead(t3d) ;
		tg.setTransform(t3d) ;
		t3d.get(newPoints[curNewPoint++]) ;

		if (print) {
		    e.getButtonState(buttons) ;
		    printValues(name, t3d, buttons) ;
		}

		if (eventDriven) {
		    pointArray.updateData(SensorDisplayBehavior.this) ;
		}
	    }
	}

	public void initialize() {
	    // Called by the behavior scheduler only when non-event-driven.
	    wakeupConditions = new WakeupOnElapsedFrames(0) ;
	    wakeupOn(wakeupConditions) ;
	}
	
	public void processStimulus(Enumeration e) {
	    // Called by the behavior scheduler only when non-event-driven.
	    eventAgent.dispatchEvents() ;
	    pointArray.updateData(this) ;
	    wakeupOn(wakeupConditions) ;
	}

	public void updateData(Geometry g) {
	    for (int i = 0 ; i < curNewPoint ; i++) {
		points[curPoint*3+0] = newPoints[i].x ;
		points[curPoint*3+1] = newPoints[i].y ;
		points[curPoint*3+2] = newPoints[i].z ;

		if (++curPoint == pointCount) curPoint = 0 ;
	    }

	    curNewPoint = 0 ;
	}

	void printValues(String name, Transform3D t3d, int[] buttons) {
	    t3d.get(m) ;
	    System.out.println("\n" + name + " sensor:\nmatrix:") ;
	    printMatrix4x4(m) ;

	    System.out.println("buttons: ") ;
	    for (int j = 0 ; j < buttons.length ; j++)
		System.out.print(buttons[j] + " ") ;
	    System.out.println() ;
	}
    }

    /**
     * Prints a 4x4 matrix with fixed fractional digits and integer padding to
     * align the decimal points in columns.  Non-negative numbers print up to
     * 7 integer digits, while negative numbers print up to 6 integer digits
     * to account for the negative sign.  6 fractional digits are printed.
     */
    static void printMatrix4x4(double[] m) {
        DecimalFormat df = new DecimalFormat("0.000000") ;
        FieldPosition fp = new FieldPosition(DecimalFormat.INTEGER_FIELD) ;
        StringBuffer sb0 = new StringBuffer() ;
        StringBuffer sb1 = new StringBuffer() ;

        for (int i = 0 ; i < 4 ; i++) {
            sb0.setLength(0) ;
            for (int j = 0 ; j < 4 ; j++) {
                sb1.setLength(0) ;
                df.format(m[i*4+j], sb1, fp) ;
                int pad = 8 - fp.getEndIndex() ;
                for (int k = 0 ; k < pad ; k++) {
                    sb1.insert(0, " ") ;
                }
                sb0.append(sb1) ;
            }
            System.out.println(sb0) ;
        }
    }
}
