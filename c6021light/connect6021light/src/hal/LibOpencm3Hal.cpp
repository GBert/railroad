#ifdef PLATFORMIO_FRAMEWORK_libopencm3

#include "hal/LibOpencm3Hal.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/i2c.h>

#include <stdio.h>

#include <errno.h>
#include <unistd.h>

extern "C" {
/* _write code taken from example at
 * https://github.com/libopencm3/libopencm3-examples/blob/master/examples/stm32/l1/stm32l-discovery/button-irq-printf-lowpower/main.c
 */
int _write(int file, char* ptr, int len) {
  int i;

  if (file == STDOUT_FILENO || file == STDERR_FILENO) {
    for (i = 0; i < len; i++) {
      if (ptr[i] == '\n') {
        usart_send_blocking(USART1, '\r');
      }
      usart_send_blocking(USART1, ptr[i]);
    }
    return i;
  }
  errno = EIO;
  return -1;
}
}

namespace hal {

void LibOpencm3Hal::beginClock() {
  // Enable the overall clock.
  rcc_clock_setup_in_hse_8mhz_out_72mhz();
  
  // Enable the UART clock
  rcc_periph_clock_enable(RCC_USART1);
  
  // Enable the I2C clock
  rcc_periph_clock_enable(RCC_I2C1);

  // Enable the CAN clock
  //rcc_periph_clock_enable(RCC_CAN1);

}

void LibOpencm3Hal::beginSerial() {
  usart_disable(USART1);
  usart_set_baudrate(USART1, 115200);
  usart_set_databits(USART1, 8);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_stopbits(USART1, 1);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
  usart_enable(USART1);
}

void LibOpencm3Hal::beginI2c() { 
  i2c_reset(I2C1);
  i2c_peripheral_disable(I2C1);

  //i2c_set_standard_mode(I2C1);
  i2c_set_speed(I2C1, i2c_speed_sm_100k, I2C_CR2_FREQ_36MHZ);
  i2c_set_own_7bit_slave_address(I2C1, this->i2cLocalAddr);
  i2c_peripheral_enable(I2C1);
  i2c_enable_ack(I2C1);
 }


void LibOpencm3Hal::beginCan() {
}

void LibOpencm3Hal::loopI2c() { printf("LibOpencm3Hal::loopI2C: Implement me!"); }

void LibOpencm3Hal::loopCan() { printf("LibOpencm3Hal::loopCan: Implement me!"); }

void LibOpencm3Hal::SendI2CMessage(MarklinI2C::Messages::AccessoryMsg const& msg) {
  printf("LibOpencm3Hal::SendI2CMessage: Implement me!");
}

void LibOpencm3Hal::SendPacket(RR32Can::Identifier const& id, RR32Can::Data const& data) {
  printf("LibOpencm3Hal::SendI2CMessage: Implement me!");
}

}  // namespace hal

#endif
