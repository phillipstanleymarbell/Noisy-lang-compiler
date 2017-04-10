// engine also has a maximum temperature


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

