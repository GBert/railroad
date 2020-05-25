#ifdef LIBOPENCM3

#include "hal/LibOpencm3Hal.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include <stdio.h>

namespace hal {

void LibOpencm3Hal::beginSerial() {
  // Enable the UART clock
  rcc_periph_clock_enable(RCC_USART1);
  // We need a brief delay before we can access UART config registers
  __asm__("nop");
  __asm__("nop");
  __asm__("nop");
  // Disable the UART while we mess with its settings
  usart_disable(USART1);
  // Configure the UART clock source as precision internal oscillator
  // usart_clock_from_piosc();
  // Set communication parameters
  usart_set_baudrate(USART1, 115200);
  usart_set_databits(USART1, 8);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_stopbits(USART1, 1);
  // Enable SW Flow Control
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
  // Now that we're done messing with the settings, enable the UART
  usart_enable(USART1);
}

void LibOpencm3Hal::beginI2c() { printf("LibOpencm3Hal::beginI2C: Implement me!"); }


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
