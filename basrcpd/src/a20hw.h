// a20hw.h : direct hardware access routines for A20-SOC on BananaPi
// C 2017 - 2024 Rainer Müller
// This file is subject of the GNU general public license 3 (GPL3)
// inspired by bcm2835.h written by Mike McCauley:
//
// Copyright (C) 2011-2013 Mike McCauley
// $Id: bcm2835.h,v 1.14 2014/08/21 01:26:42 mikem Exp mikem $
//

// Defines for A20
#ifndef A20HW_H
#define A20HW_H

#include <stdint.h>

// pin states
#define LOW  0
#define HIGH 1

// pin numbers
#define PORTAPIN    0
#define PORTBPIN    32
#define PORTCPIN    64
#define PORTDPIN    96
#define PORTEPIN    128
#define PORTFPIN    160
#define PORTGPIN    192
#define PORTHPIN    224
#define PORTIPIN    256


/// \brief a20PortFunction
/// Port function select modes for a20_gpio_fsel()
typedef enum
{
    A20_GPIO_FSEL_INPT  = 0b000,   ///< Input
    A20_GPIO_FSEL_OUTP  = 0b001,   ///< Output
    A20_GPIO_FSEL_ALT0  = 0b100,   ///< Alternate function 0
    A20_GPIO_FSEL_ALT1  = 0b101,   ///< Alternate function 1
    A20_GPIO_FSEL_ALT2  = 0b110,   ///< Alternate function 2
    A20_GPIO_FSEL_ALT3  = 0b111,   ///< Alternate function 3
    A20_GPIO_FSEL_ALT4  = 0b011,   ///< Alternate function 4
    A20_GPIO_FSEL_ALT5  = 0b010,   ///< Alternate function 5
    A20_GPIO_FSEL_MASK  = 0b111    ///< Function select bits mask
} a20FunctionSelect;

/// \brief a20PUDControl
/// Pullup/Pulldown defines for a20_gpio_pud()
typedef enum
{
    A20_GPIO_PUD_OFF   = 0b00,  // Off = disable pull-up/down
    A20_GPIO_PUD_DOWN  = 0b10,  // Enable Pull Down control 
    A20_GPIO_PUD_UP    = 0b01,  // Enable Pull Up control 
    A20_GPIO_PUD_MASK  = 0b11  
} a20PUDControl;


#ifdef __cplusplus
extern "C" {
#endif

    /// Initialise the library by opening /dev/mem and getting pointers to the 
    /// internal memory for BCM 2835 device registers. You must call this (successfully)
    /// before calling any other 
    /// functions in this library (except a20_set_debug). 
    /// If a20_init() fails by returning 0, 
    /// calling any other function may result in crashes or other failures.
    /// Prints messages to stderr in case of errors.
    /// \return 1 if successful else 0
    extern int a20_init(void);

    /// Close the library, deallocating any allocated memory and closing /dev/mem
    /// \return 1 if successful else 0
    extern int a20_close(void);

    /// Reads 32 bit value from a peripheral address
    /// The read is done twice, and is therefore always safe in terms of 
    /// manual section 1.3 Peripheral access precautions for correct memory ordering
    /// \param[in] paddr Physical address to read from. See A20_GPIO_BASE etc.
    /// \return the value read from the 32 bit register
    /// \sa Physical Addresses
    extern uint32_t a20_peri_read(const volatile uint32_t* paddr);

    /// Writes 32 bit value from a peripheral address
    /// The write is done twice, and is therefore always safe in terms of 
    /// manual section 1.3 Peripheral access precautions for correct memory ordering
    /// \param[in] paddr Physical address to read from. See A20_GPIO_BASE etc.
    /// \param[in] value The 32 bit value to write
    /// \sa Physical Addresses
    extern void a20_peri_write(volatile uint32_t* paddr, uint32_t value);

    /// Alters a number of bits in a 32 peripheral regsiter.
    /// It reads the current valu and then alters the bits deines as 1 in mask, 
    /// according to the bit value in value. 
    /// All other bits that are 0 in the mask are unaffected.
    /// Use this to alter a subset of the bits in a register.
    /// The write is done twice, and is therefore always safe in terms of 
    /// manual section 1.3 Peripheral access precautions for correct memory ordering
    /// \param[in] paddr Physical address to read from. See A20_GPIO_BASE etc.
    /// \param[in] value The 32 bit value to write, masked in by mask.
    /// \param[in] mask Bitmask that defines the bits that will be altered in the register.
    /// \sa Physical Addresses
    extern void a20_peri_set_bits(volatile uint32_t* paddr, uint32_t value, uint32_t mask);

    /// Sets the Function Select register for the given pin, which configures
    /// the pin as Input, Output or one of the 6 alternate functions.
    /// \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    /// \param[in] mode Mode to set the pin to, one of A20_GPIO_FSEL_* from \ref a20FunctionSelect
    extern void a20_gpio_set_fsel(uint16_t pin, uint8_t mode);

    // Reads the Function Select register for the given pin, which configures
    // the pin as Input, Output or one of the 6 alternate functions.
    extern uint8_t a20_gpio_fsel(uint16_t pin);

    /// Sets the specified pin output to HIGH.
    /// \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    /// \sa a20_gpio_write()
    extern void a20_gpio_set(uint16_t pin);

    /// Sets the specified pin output to LOW.
    /// \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    /// \sa a20_gpio_write()
    extern void a20_gpio_clr(uint16_t pin);

    /// Reads the current level on the specified 
    /// pin and returns either HIGH or LOW. Works whether or not the pin
    /// is an input or an output.
    /// \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    /// \return the current level  either HIGH or LOW
    extern uint8_t a20_gpio_lev(uint16_t pin);

    /// Sets the Pull-up/down mode for the specified pin. This is more convenient than
    /// clocking the mode in with a20_gpio_pud() and a20_gpio_pudclk().
    /// \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    /// \param[in] pud The desired Pull-up/down mode. One of A20_GPIO_PUD_* from a20PUDControl
    /// \sa a20_gpio_set_pud()
    extern void a20_gpio_set_pud(uint16_t pin, uint8_t pud);

    // Reads the Pull-up/down mode for the specified pin. 
    extern uint8_t a20_gpio_pud(uint16_t pin);

    /// Reads and returns the Pad Control for the given GPIO group.
    /// \param[in] group The GPIO pad group number, one of A20_PAD_GROUP_GPIO_*
    /// \return Mask of bits from A20_PAD_* from \ref a20PadGroup
    extern uint8_t a20_gpio_pad(uint16_t pin);

    /// Sets the Pad Control for the given GPIO group.
    /// \param[in] group The GPIO pad group number, one of A20_PAD_GROUP_GPIO_*
    /// \param[in] control Mask of bits from A20_PAD_* from \ref a20PadGroup. Note 
    /// that it is not necessary to include A20_PAD_PASSWRD in the mask as this
    /// is automatically included.
    extern void a20_gpio_set_pad(uint16_t pin, uint8_t control);

    /// Sets the output state of the specified pin
    /// \param[in] pin GPIO number, or one of RPI_GPIO_P1_* from \ref RPiGPIOPin.
    /// \param[in] on HIGH sets the output to HIGH and LOW to LOW.
    extern void a20_gpio_write(uint16_t pin, uint8_t on);

    /// @} 
#ifdef __cplusplus
}
#endif

#endif // A20HW_H
