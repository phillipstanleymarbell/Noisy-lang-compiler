// engine maximum speed of Formula One car is 18000 rpm
//
// http://www.toptransmissions.com/electrical-sensors
// should stop engine when: Power Train Control Module tells us tranmission is at third gear.
// Power Train Control Module is the adaptive microprocessor
// Uses Transmission Range sensor or manual lever position switch to read what position transmission is in
// If in 3rd gear, cannot stop the engine
//
// Newton does not yet have logic operations to be able to say If User Transmission Range sensor says 3rd gear,
// engine speed should be below the maximum
// For now, just check that engine speed is below the maximum


float calculate_power()
{
    float engine_speed = read_engine_speed();
    const float torque = get_torque();
    float power = engine_speed * torque;
}


power calculate_power()
{
    angular_velocity engine_speed = read_engine_speed();
    torque engine_torque = get_torque_from_machine_specs();
    power engine_power = engine_speed * torque;
}

