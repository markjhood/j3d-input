package com.sun.j3d.input ;

import javax.media.j3d.InputDevice ;
import com.sun.j3d.utils.behaviors.sensor.SensorEventAgent ;

//
// Experimental for now.
// 
public interface EventDrivenInputDevice extends InputDevice {

    public void setSensorEventAgent(SensorEventAgent agent) ;
    public SensorEventAgent getSensorEventAgent() ;
}
