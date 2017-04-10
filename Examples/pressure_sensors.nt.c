// https://en.wikipedia.org/wiki/Aeroper%C3%BA_Flight_603

// This is an accident where the cleaning crew had the pitot tube of an airplane blocked with tape.
// That means static port was blocked in the pitot tube, and pressure sensor would report wrong static pressure readings.
// As a result, airspeed indicator, altitutde indicator, overspeed, underspeed, and flying too low
// However, there is another static pressure sensor in altimeter that should report report the correct static pressure readings.
// The pilot only sees the computation result from the pressure sensors in altimeter and pitot tubes, not the pressure readings themselves.
// If there was some assert saying the static pressure sensors from the pitot tube and the altimeter should report same values,
// the accident may have been prevented.
//
// For this code assume that the default Newton description is pressure_sensors_airplane.nt. 
// A simple Newton invariant states altimeter static pressure sensor readings should equal pitot tube static pressure readings.

// Using equations from https://en.wikipedia.org/wiki/Pitot_tube#Aircraft

/*
 * Not using Newton
 */
float get_flow_velocity()
{
    float stagnation_pressure = read_from_stagnation_pressure_sensor_pitot_tube();
    float static_pressure = read_from_static_pressure_sensor_pitot_tube();
    float fluid_density = 3;

    return math.sqrt(2 * (stagnation_pressure - static_pressure) / fluid_density);
}


/*
 * Using Newton
 */
// 0 is the static pressure sensor for altimeter
// 1 is the static pressure sensor for pitot tube
// 2 is the stagnation pressure sensor for pitot tube
velocity get_flow_velocity()
{
    pressure@2 stagnation_pressure = read_from_stagnation_pressure_sensor_pitot_tube();
    pressure@1 static_pressure = read_from_static_pressure_sensor_pitot_tube();
    density fluid_density = 3;

    /*
     * Here, Newton will assert that the value of static_pressure from pitot tube 
     * should equal the value of static_pressure from altimeter.
     * If the invariant returns constraints are not satisfied, then the compiler writer
     * can make the code trigger some warning 
     *
     * For this to work, the runtime will need to pass any subset of variables that match
     * the invariant signature in the set S = {vars in basic block} U {Global set of sensor values declared with }
     */
    return math.sqrt(2 * (stagnation_pressure - static_pressure) / fluid_density);
}
