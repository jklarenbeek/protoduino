// file: ./src/sys/tmpl/serial_private_header.h

#include <cc.h>

/**
 * @brief Serial communication template header (public API via serial.h)
 *
 * @details
 * This header defines the full Serial API using a macro-based template mechanism.
 * It provides buffered serial communication on top of the UART peripheral, with ring buffers for RX and TX,
 * interrupt-driven transmission and reception, and error handling.
 * Despite the name, this file is not private in the traditional sense.
 *
 * In Arduino, the Serial class is implemented in C++ as HardwareSerial, which handles UART communication
 * with software buffers (typically 64 bytes for RX and TX), using interrupts for efficient data handling.
 * It utilizes AVR UART registers like UDR, UCSRA, UCSRB, UCSRC for configuration and data transfer,
 * with RXCIE and UDRIE interrupts for receive complete and data register empty events.
 * The class inherits from Stream, providing methods like begin(), available(), read(), write(), flush(), etc.,
 * and supports multiple instances (Serial, Serial1, etc.) on boards with multiple UARTs.
 *
 * Protoduino reimplements the Serial functionality to prioritize a C99-first approach, separating the low-level
 * UART handling (in uart.h) from the buffered serial layer (in serial.h). This allows for greater portability,
 * modularity, and independence from the Arduino core, while maintaining compatibility. The core implementation
 * uses C99 with macro templates for multiple instances (serial0, serial1, etc.), ring buffers (ringb8),
 * and callbacks for RX complete, RX error, and TX complete. The SerialClass.hpp then wraps this C99 implementation
 * in a C++ class (e.g., Serial0Class) that mimics Arduino's Serial class, inheriting from Stream and implementing
 * the same interface for seamless integration in C++ sketches.
 *
 * The public Serial interface is exposed by including this file through serial.h, which defines the serial
 * instance prefix (e.g., serial0, serial1) before inclusion.
 *
 * @note
 * Application code MUST include serial.h instead of including this file directly. This header relies on
 * CC_TMPL_PREFIX being defined prior to inclusion, which happens through the serial.h file.
 *
 * @par Template Mechanism
 * - CC_TMPL_PREFIX selects the serial instance (serial0, serial1, serial2, serial3) based on available UARTs.
 * - CC_TMPL_FN() performs symbol name mangling.
 * - Multiple serial instances can coexist safely.
 *
 * @par Public API
 * All functions and extern symbols declared in this file become part of the public Serial API once included via serial.h.
 *
 * @warning
 * For Arduino compatibility in C++, use SerialClass.hpp. Direct use of serial functions is low-level and intended for C code.
 * The reimplementation avoids dependencies on Arduino's core libraries, enabling use in bare-metal or custom environments.
 */

#ifdef SERIAL_REGISTER_ERRORS
/**
 * @fn uint_fast32_t serial[0..3]_get_read_errors(void)
 * @brief Retrieves the total count of receive errors encountered.
 *
 * This function returns the accumulated number of receive errors (such as frame, overrun, or parity errors) since the serial port was opened or the counter was last reset.
 * Available only if SERIAL_REGISTER_ERRORS is defined.
 *
 * @return The count of receive errors.
 */
CC_EXTERN uint_fast32_t CC_TMPL_FN(get_read_errors)(void);

/**
 * @fn uint_fast32_t serial[0..3]_get_read_overflow(void)
 * @brief Retrieves the total count of receive buffer overflow errors.
 *
 * This function returns the number of times data was received but discarded due to a full receive buffer.
 * Available only if SERIAL_REGISTER_ERRORS is defined.
 *
 * @return The count of receive overflow errors.
 */
CC_EXTERN uint_fast32_t CC_TMPL_FN(get_read_overflow)(void);
#endif

/**
 * @fn void serial[0..3]_on_recieved(const serial_onrecieved_fn callback)
 * @brief Sets the callback function for immediate handling of received data.
 *
 * Assigns a callback of type serial_onrecieved_fn, which is invoked upon receiving a byte.
 * The callback receives the data as uint_fast8_t and returns a bool: true if the data was handled (not buffered), false to buffer it.
 * Set to NULL to disable and always buffer received data.
 *
 * @param callback The callback function pointer, or NULL to disable.
 */
CC_EXTERN void CC_TMPL_FN(on_recieved)(const serial_onrecieved_fn callback);

/**
 * @fn void serial[0..3]_open(uint32_t baud)
 * @brief Initializes and opens the serial port with default configuration.
 *
 * Configures the underlying UART with the specified baud rate and default settings (8 data bits, 1 stop bit, no parity).
 * Sets up internal callbacks for RX complete, RX error, and TX complete to manage buffering and interrupts.
 * Initializes ring buffers for RX and TX.
 *
 * @param baud The desired baud rate (e.g., UART_BAUD_9600).
 */
CC_EXTERN void CC_TMPL_FN(open)(uint32_t baud);

/**
 * @fn void serial[0..3]_openex(uint32_t baud, uint8_t config)
 * @brief Initializes and opens the serial port with extended configuration.
 *
 * Similar to serial[0..3]_open, but allows custom configuration for the UART control register (e.g., data bits, parity, stop bits).
 * The config parameter is passed to the underlying UART's UCSRC.
 *
 * @param baud The desired baud rate.
 * @param config Bitmask for UART configuration (e.g., 0x06 for 8-bit data).
 */
CC_EXTERN void CC_TMPL_FN(openex)(uint32_t baud, uint8_t config);

/**
 * @fn void serial[0..3]_close(void)
 * @brief Closes the serial port and disables communication.
 *
 * Flushes any remaining data in the TX buffer, then disables the underlying UART receiver, transmitter, and interrupts.
 * Does not clear the buffers or error counters.
 */
CC_EXTERN void CC_TMPL_FN(close)(void);

/**
 * @fn uint_fast8_t serial[0..3]_read_available(void)
 * @brief Returns the number of bytes available in the receive buffer.
 *
 * Queries the RX ring buffer to determine how many bytes are ready to be read.
 *
 * @return The number of available bytes (0 to SERIAL_RX_BUFFER_SIZE).
 */
CC_EXTERN uint_fast8_t CC_TMPL_FN(read_available)(void);

/**
 * @fn int_fast16_t serial[0..3]_peek8(void)
 * @brief Peeks at the next byte in the receive buffer without removing it.
 *
 * Returns the next byte if available, or -1 if the buffer is empty.
 *
 * @return The next byte (0-255) or -1 if no data.
 */
CC_EXTERN int_fast16_t CC_TMPL_FN(peek8)(void);

/**
 * @fn int_fast16_t serial[0..3]_read8(void)
 * @brief Reads and removes a single byte from the receive buffer.
 *
 * Returns the byte if available, or -1 if the buffer is empty.
 *
 * @return The read byte (0-255) or -1 if no data.
 */
CC_EXTERN int_fast16_t CC_TMPL_FN(read8)(void);

/**
 * @fn int_fast32_t serial[0..3]_read16(void)
 * @brief Reads two bytes from the receive buffer as a 16-bit value.
 *
 * Interprets the bytes in little-endian order (low byte first).
 * Returns the value if at least two bytes are available, or -1 otherwise.
 *
 * @return The 16-bit value (0-65535) or -1 if insufficient data.
 */
CC_EXTERN int_fast32_t CC_TMPL_FN(read16)(void);

/**
 * @fn int_fast32_t serial[0..3]_read24(void)
 * @brief Reads three bytes from the receive buffer as a 24-bit value.
 *
 * Interprets the bytes in little-endian order, with the high byte zero-padded to form a 32-bit signed value.
 * Returns the value if at least three bytes are available, or -1 otherwise.
 *
 * @return The 24-bit value (0-16777215) or -1 if insufficient data.
 */
CC_EXTERN int_fast32_t CC_TMPL_FN(read24)(void);

/**
 * @fn uint_fast32_t serial[0..3]_read32(void)
 * @brief Reads four bytes from the receive buffer as a 32-bit value.
 *
 * Interprets the bytes in little-endian order.
 * Returns the value if at least four bytes are available, or 0 otherwise (note: caller must check availability separately if needed).
 *
 * @return The 32-bit value or 0 if insufficient data.
 */
CC_EXTERN uint_fast32_t CC_TMPL_FN(read32)(void);

/**
 * @fn uint_fast8_t serial[0..3]_write_available(void)
 * @brief Returns the available space in the transmit buffer.
 *
 * Queries the TX ring buffer to determine how many bytes can be written without blocking.
 *
 * @return The available space (0 to SERIAL_TX_BUFFER_SIZE).
 */
CC_EXTERN uint_fast8_t CC_TMPL_FN(write_available)();

/**
 * @fn uint_fast8_t serial[0..3]_write8(const uint_fast8_t data)
 * @brief Writes a single byte to the transmit buffer or directly to UART.
 *
 * If the buffer is empty and UART is ready, writes directly. Otherwise, adds to the buffer if space is available and enables TX interrupt.
 * Returns 1 on success, 0 if no space.
 *
 * @param data The byte to write.
 * @return 1 if written, 0 if buffer full.
 */
CC_EXTERN uint_fast8_t CC_TMPL_FN(write8)(const uint_fast8_t data);

/**
 * @fn uint_fast8_t serial[0..3]_write16(const uint_fast16_t data)
 * @brief Writes a 16-bit value as two bytes to the transmit buffer.
 *
 * Writes in little-endian order. Adds to buffer atomically and enables TX interrupt if needed.
 * Returns 2 on success, 0 if insufficient space.
 *
 * @param data The 16-bit value to write.
 * @return 2 if written, 0 if insufficient space.
 */
CC_EXTERN uint_fast8_t CC_TMPL_FN(write16)(const uint_fast16_t data);

/**
 * @fn uint_fast8_t serial[0..3]_write24(const uint_fast32_t data)
 * @brief Writes a 24-bit value as three bytes to the transmit buffer.
 *
 * Writes in little-endian order (low 24 bits). Adds to buffer atomically and enables TX interrupt if needed.
 * Returns 3 on success, 0 if insufficient space.
 *
 * @param data The 24-bit value to write.
 * @return 3 if written, 0 if insufficient space.
 */
CC_EXTERN uint_fast8_t CC_TMPL_FN(write24)(const uint_fast32_t data);

/**
 * @fn uint_fast8_t serial[0..3]_write32(const uint_fast32_t data)
 * @brief Writes a 32-bit value as four bytes to the transmit buffer.
 *
 * Writes in little-endian order. Adds to buffer atomically and enables TX interrupt if needed.
 * Returns 4 on success, 0 if insufficient space.
 *
 * @param data The 32-bit value to write.
 * @return 4 if written, 0 if insufficient space.
 */
CC_EXTERN uint_fast8_t CC_TMPL_FN(write32)(const uint_fast32_t data);

/**
 * @fn uint_fast8_t serial[0..3]_flush(void)
 * @brief Flushes the transmit buffer by sending remaining data.
 *
 * If global interrupts are enabled, ensures TX interrupt is on and returns the current buffer count.
 * If interrupts are disabled, attempts to write one byte directly if UART is ready, and returns the updated count.
 * Returns 0 if buffer is already empty.
 *
 * @return The number of bytes remaining in the buffer after the operation.
 */
CC_EXTERN uint_fast8_t CC_TMPL_FN(flush)(void);
