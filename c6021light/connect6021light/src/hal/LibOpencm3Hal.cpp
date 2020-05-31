#ifdef PLATFORMIO_FRAMEWORK_libopencm3

#include "hal/LibOpencm3Hal.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

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

volatile uint_fast8_t LibOpencm3Hal::bytesRead;

void LibOpencm3Hal::beginClock() {
  // Enable the overall clock.
  rcc_clock_setup_in_hse_8mhz_out_72mhz();

  // Enable GPIO Pin Banks used for GPIO or alternate functions
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  // rcc_periph_clock_enable(RCC_GPIOC);
  // Enable Clock for alternate functions
  rcc_periph_clock_enable(RCC_AFIO);

  // Enable the UART clock
  rcc_periph_clock_enable(RCC_USART1);

  // Enable the I2C clock
  rcc_periph_clock_enable(RCC_I2C1);

  // Enable the CAN clock
  // rcc_periph_clock_enable(RCC_CAN1);
}

void LibOpencm3Hal::beginSerial() {
  usart_disable(USART1);

  // Enable the USART TX Pin in the GPIO controller
  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

  // Set Serial speed
  usart_set_baudrate(USART1, 115200);
  usart_set_databits(USART1, 8);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

  // Enable Serial TX
  usart_set_mode(USART1, USART_MODE_TX);
  usart_enable(USART1);
}

void LibOpencm3Hal::beginI2c() {
  i2c_peripheral_disable(I2C1);
  i2c_reset(I2C1);

  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
                GPIO_I2C1_SCL | GPIO_I2C1_SDA);
  gpio_set(GPIOB, GPIO_I2C1_SCL | GPIO_I2C1_SDA);

  // Basic I2C configuration
  // i2c_set_speed(I2C1, i2c_speed_sm_100k, I2C_CR2_FREQ_36MHZ);
  i2c_set_standard_mode(I2C1);
  i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_36MHZ);
  i2c_set_trise(I2C1, 36);
  i2c_set_dutycycle(I2C1, I2C_CCR_DUTY_DIV2);
  i2c_set_ccr(I2C1, 180);

  i2c_set_own_7bit_slave_address(I2C1, this->i2cLocalAddr);

  // Set I2C IRQ to support slave mode
  bytesRead = 0;
  nvic_enable_irq(NVIC_I2C1_EV_IRQ);
  i2c_enable_interrupt(I2C1, I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN);

  i2c_peripheral_enable(I2C1);
  i2c_enable_ack(I2C1);
}

// i2c1 event ISR
// Code based on
// https://amitesh-singh.github.io/stm32/2018/01/07/making-i2c-slave-using-stm32f103.html
extern "C" void i2c1_ev_isr(void) { LibOpencm3Hal::i2cEvInt(); }

void LibOpencm3Hal::i2cEvInt(void) {
  // ISR appears to be called once per I2C byte received

  if (HalBase::i2cMessageReceived) {
    // Already a message received - abort
    return;
  }

  uint32_t sr1, sr2;

  sr1 = I2C_SR1(I2C1);

  // Address matched (Slave)
  if (sr1 & I2C_SR1_ADDR) {
    bytesRead = 1;
    if (!HalBase::i2cMessageReceived) {
      HalBase::i2cMsg.destination = HalBase::i2cLocalAddr;
    }
    // Clear the ADDR sequence by reading SR2.
    sr2 = I2C_SR2(I2C1);
    (void)sr2;
  }
  // Receive buffer not empty
  else if (sr1 & I2C_SR1_RxNE) {
    switch (bytesRead) {
      case 1:
        if (!HalBase::i2cMessageReceived) {
          HalBase::i2cMsg.source = i2c_get_data(I2C1);
        }
        ++bytesRead;
        break;
      case 2:
        if (!HalBase::i2cMessageReceived) {
          HalBase::i2cMsg.data = i2c_get_data(I2C1);
        }
        ++bytesRead;
        break;
      default:
        // Ignore reading byte 0 or bytes past 2
        break;
    }
  }
  // Transmit buffer empty & Data byte transfer not finished
  else if ((sr1 & I2C_SR1_TxE) && !(sr1 & I2C_SR1_BTF)) {
    // send dummy data to master in MSB order
    i2c_send_data(I2C1, 0xBF);
  }
  // done by master by sending STOP
  // this event happens when slave is in Recv mode at the end of communication
  else if (sr1 & I2C_SR1_STOPF) {
    i2c_peripheral_enable(I2C1);
    if (bytesRead == 3) {
      HalBase::i2cMessageReceived = true;
    }
  }
  // this event happens when slave is in transmit mode at the end of communication
  else if (sr1 & I2C_SR1_AF) {
    //(void) I2C_SR1(I2C1);
    I2C_SR1(I2C1) &= ~(I2C_SR1_AF);
    if (bytesRead == 3) {
      HalBase::i2cMessageReceived = true;
    }
  }
}

void LibOpencm3Hal::beginCan() {}

void LibOpencm3Hal::loopCan() { printf("LibOpencm3Hal::loopCan: Implement me! Read %d bytes.\n", bytesRead); }

void LibOpencm3Hal::SendI2CMessage(MarklinI2C::Messages::AccessoryMsg const& msg) {
  uint8_t databytes[2];
  databytes[0] = msg.source;
  databytes[1] = msg.data;
  // i2c_transfer7(I2C1, msg.destination, databytes, 2, nullptr, 0);
  // i2c_wait_busy(I2C1);
  i2c_send_start(I2C1);
  i2c_send_7bit_address(I2C1, msg.destination, I2C_WRITE);
  i2c_send_data(I2C1, msg.source);
  i2c_send_data(I2C1, msg.data);
  i2c_send_stop(I2C1);
}

void LibOpencm3Hal::SendPacket(RR32Can::Identifier const& id, RR32Can::Data const& data) {
  printf("LibOpencm3Hal::SendI2CMessage: Implement me!");
}

}  // namespace hal

#endif
