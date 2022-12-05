#include <mbed.h>
#include <bme280.h>

I2C i2c(PB_9, PB_8);            // i2c(SDA, SCL)
const int addr7bit = 0x77;      // 7-bit I2C address
const int addr8bit = 0x77 << 1; // 8-bit I2C address

void user_delay_ms(uint32_t period, void *intf_ptr)
{
  /*
   * Return control or wait,
   * for a period amount of milliseconds
   */
}

int8_t user_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
  // int reg_addr_int = (int)reg_addr;

  /*
   * The parameter intf_ptr can be used as a variable to store the I2C address of the device
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
  i2c.start();
  i2c.write(addr8bit, (unsigned char)reg_addr, len);
  // i2c.transfer might be effective
  i2c.stop();
  i2c.start();
  i2c.read(addr8bit, reg_data, len);
  i2c.stop();

  return rslt;
}

int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

  /*
   * The parameter intf_ptr can be used as a variable to store the I2C address of the device
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

int main()
{

  // put your setup code here, to run once:
  int counter = 0;

  struct bme280_dev dev;
  int8_t rslt = BME280_OK;
  uint8_t dev_addr = BME280_I2C_ADDR_SEC; // 0x77

  dev.intf_ptr = &dev_addr;
  dev.intf = BME280_I2C_INTF;
  dev.read = user_i2c_read;
  dev.write = user_i2c_write;
  dev.delay_us = user_delay_ms;

  rslt = bme280_init(&dev);

  while (1)
  {
    // put your main code here, to run repeatedly:
    printf("run %i ...", counter++);
    printf("result: %i \n", rslt);

    thread_sleep_for(500);
  }
}