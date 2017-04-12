// pressure sensors in car tires
// https://www.kaltire.com/the-right-tire-pressure-why-the-maximum-isnt-the-best/


void step_on_gas_pedal()
{
    acceleration additional_acceleration = read_from_gas_pedal_signal();

    /*
     * Do some accelerating stuff here
     *
     * Runtime will pass in a variable called "pressure tire_pressure" implicitly 
     * to Newton API and it has to pass the invariant for temperature ranges
     */
}
