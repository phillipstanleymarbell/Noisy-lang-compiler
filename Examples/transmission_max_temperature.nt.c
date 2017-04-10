// transmission has a maximum temperature of around 90 Celsius, 
// minimum temperature -50C
//
// Transmission Fluid Temperature Sensor
// http://www.toptransmissions.com/electrical-sensors


void change_gears()
{
    int mode = read_stick_shift_input();

    /*
     * Does some shifting gears here
     * Runtime will pass in a variable called "temperature transmission_temperature" implicitly 
     * to Newton API and it has to pass the invariant for temperature ranges
     */
    mechanically_shift_gears(mode);
}
