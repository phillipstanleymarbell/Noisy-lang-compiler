#include "bme680.h"
#include "signalTypedef.h"
//#include <stdio.h>

void user_delay_ms(uint32_t period)
{
    /*
     * Return control or wait,
     * for a period amount of milliseconds
     */
}

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

    /*
     * The parameter dev_id can be used as a variable to store the I2C address of the device
     */

    /*
     * Data on the bus should be like
     * |------------+---------------------|
     * | I2C action | Data                |
     * |------------+---------------------|
     * | Start      | -                   |
     * | Write      | (reg_addr)          |
     * | Stop       | -                   |
     * | Start      | -                   |
     * | Read       | (reg_data[0])       |
     * | Read       | (....)              |
     * | Read       | (reg_data[len - 1]) |
     * | Stop       | -                   |
     * |------------+---------------------|
     */

    return rslt;
}


int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

    /*
     * The parameter dev_id can be used as a variable to store the I2C address of the device
     */

    /*
     * Data on the bus should be like
     * |------------+---------------------|
     * | I2C action | Data                |
     * |------------+---------------------|
     * | Start      | -                   |
     * | Write      | (reg_addr)          |
     * | Write      | (reg_data[0])       |
     * | Write      | (....)              |
     * | Write      | (reg_data[len - 1]) |
     * | Stop       | -                   |
     * |------------+---------------------|
     */

    return rslt;
}


static uint32_t calc_humidity(voltage hum_adc, const struct bme680_dev *dev)
{
	voltage var1;
	relativeHumidityPerVoltage var2;
	relativeHumidity var3;
	relativeHumidity var4;
	relativeHumidity var5;
	relativeHumidity var6;
	temperature temp_scaled;
	relativeHumidity calc_hum;

	temperaturePerVoltageSquaredDimension  unitHavingConstant1=1;
	temperatureSquaredDimension  unitHavingConstant2=1;

	temp_scaled = (((int32_t) dev->calib.t_fine * 5)*unitHavingConstant1 + 128) >> 8;
	var1 = (int32_t) (hum_adc - ((int32_t) ((int32_t) dev->calib.par_h1 * 16)))
		- (((temp_scaled * (int32_t) dev->calib.par_h3) / ((int32_t) 100)) >> 1);
	var2 = ((int32_t) dev->calib.par_h2
		* (((temp_scaled * (int32_t) dev->calib.par_h4) / ((int32_t) 100))
			+ (((temp_scaled * ((temp_scaled * (int32_t) dev->calib.par_h5) / ((int32_t) 100))) >> 6)
				/ ((int32_t) 100)) + (int32_t) (1 << 14)*unitHavingConstant2)) >> 10;
	var3 = var1 * var2;
	var4 = (int32_t) dev->calib.par_h6 << 7;
	var4 = ((var4) + ((temp_scaled * (int32_t) dev->calib.par_h7) / ((int32_t) 100))) >> 4;
	var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
	var6 = (var4 * var5) >> 1;
	calc_hum = (((var3 + var6) >> 10) * ((int32_t) 1000)) >> 12;

	if (calc_hum > 100000) /* Cap at 100%rH */
		calc_hum = 100000;
	else if (calc_hum < 0)
		calc_hum = 0;

	return (uint32_t) calc_hum;
}


int main(){
	struct bme680_dev gas_sensor;

	gas_sensor.dev_id = BME680_I2C_ADDR_PRIMARY;
	gas_sensor.intf = BME680_I2C_INTF;
	gas_sensor.read = user_i2c_read;
	gas_sensor.write = user_i2c_write;
	gas_sensor.delay_ms = user_delay_ms;
	/* amb_temp can be set to 25 prior to configuring the gas sensor
	 * or by performing a few temperature readings without operating the gas sensor.
	 */
	gas_sensor.amb_temp = 25;
	
	int8_t rslt = BME680_OK;
	rslt = bme680_init(&gas_sensor);

	uint32_t temp_adc=10;
	calc_humidity(temp_adc, &gas_sensor);

}


