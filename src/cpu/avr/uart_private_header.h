// file: ./src/cpu/avr/uart_private_header.h

/**
 * @brief UART peripheral template header (public API via uart.h)
 *
 * @details
 * This header defines the full UART API and register bindings using a
 * macro-based template mechanism. Despite the name, this file is not
 * private in the traditional sense.
 *
 * The public UART interface is exposed by including this file through
 * uart.h, which defines the UART instance prefix (e.g. uart0, uart1)
 * before inclusion.
 *
 * @note
 * Application code MUST include uart.h instead of including this file
 * directly. This header relies on CC_TMPL_PREFIX being defined prior to
 * inclusion, which happens through the uart.h file.
 *
 * @par Template Mechanism
 * - CC_TMPL_PREFIX selects the UART instance if available (uart0, uart1, uart2, uart3)
 * - CC_TMPL_FN() performs symbol name mangling
 * - Multiple UART instances can coexist safely
 *
 * @par Public API
 * All types, functions, and extern symbols declared in this file become
 * part of the public UART API once included via uart.h.
 *
 * @warning
 * Direct access to UART registers declared here should be considered
 * low-level and hardware-specific. Prefer the provided API functions
 * where possible.
 */

#include <cc.h>

/**
 * @fn void uart[0..3]_on_rx_complete(const uart_on_rx_complete_fn callback)
 * @brief Sets the callback function for the UART receive complete event.
 *
 * This function assigns a callback that will be invoked when a byte is successfully received via UART.
 * The callback is of type uart_on_rx_complete_fn, which takes a uint_fast8_t parameter representing the received data.
 * Initially set to NULL; calling this function overrides the previous callback.
 *
 * @param callback The function pointer to the callback handler, or NULL to disable.
 */
CC_EXTERN void CC_TMPL_FN(on_rx_complete)(const uart_on_rx_complete_fn callback);

/**
 *
 * @fn void uart[0..3]_on_rx_error(const uart_on_rx_error_fn callback)
 * @brief Sets the callback function for the UART receive error event.
 *
 * This function assigns a callback that will be invoked when an error occurs during UART reception, such as frame error, data overrun, or parity error.
 * The callback is of type uart_on_rx_error_fn, which takes a uint_fast8_t parameter representing the error code.
 * Initially set to NULL; calling this function overrides the previous callback.
 *
 * @param callback The function pointer to the callback handler, or NULL to disable.
 */
CC_EXTERN void CC_TMPL_FN(on_rx_error)(const uart_on_rx_error_fn callback);

/**
 * @fn void uart[0..3]_on_tx_complete(const uart_on_tx_complete_fn callback)
 * @brief Sets the callback function for the UART transmit complete event.
 *
 * This function assigns a callback that will be invoked when the UART transmit buffer is ready for the next byte (data register empty).
 * The callback is of type uart_on_tx_complete_fn, which should return an int_fast16_t: the next byte to transmit (0-255) or a negative value to indicate no more data.
 * Initially set to NULL; calling this function overrides the previous callback.
 *
 * @param callback The function pointer to the callback handler, or NULL to disable.
 */
CC_EXTERN void CC_TMPL_FN(on_tx_complete)(const uart_on_tx_complete_fn callback);

/**
 * @fn void uart[0..3]_open(uint32_t baud)
 * @brief Initializes and opens the UART peripheral with default settings.
 *
 * Configures the UART for asynchronous operation with 8 data bits, 1 stop bit, no parity, and the specified baud rate.
 * Enables the receiver and transmitter, enables the receive complete interrupt, and disables the data register empty interrupt.
 * This is a convenience wrapper that calls uart[0..3]_openex with default options (0x06 for 8-bit data).
 *
 * @param baud The desired baud rate (e.g., UART_BAUD_9600).
 */
CC_EXTERN void CC_TMPL_FN(open)(uint32_t baud);

/**
 * @fn void uart[0..3]_openex(uint32_t baud, uint8_t options)
 * @brief Initializes and opens the UART peripheral with extended options.
 *
 * Configures the UART baud rate register, control registers, and enables double-speed mode if applicable for better baud rate accuracy.
 * The options parameter is written to UCSRC to set data bits, parity, stop bits, etc.
 * Enables the receiver and transmitter, enables the receive complete interrupt, and disables the data register empty interrupt.
 * Handles special cases like 57600 baud on 16MHz systems.
 *
 * @param baud The desired baud rate (e.g., UART_BAUD_9600).
 * @param options Bitmask for UCSRC configuration (e.g., data bits, parity, stop bits).
 */
CC_EXTERN void CC_TMPL_FN(openex)(uint32_t baud, uint8_t options);

/**
 * @fn void uart[0..3]_close(void)
 * @brief Closes the UART peripheral and disables all related features.
 *
 * Disables the receiver, transmitter, and all interrupts. Resets the stored baud rate to 0.
 * This function is atomic to ensure safe disabling during interrupt contexts.
 */
CC_EXTERN void CC_TMPL_FN(close)(void);

/**
 *
 * @fn uint_fast32_t uart[0..3]_baudrate(void)
 * @brief Retrieves the currently configured baud rate.
 *
 * Returns the approximate baud rate computed during initialization based on the system clock (F_CPU) and baud rate register settings.
 * This may differ slightly from the requested baud due to clock division limitations.
 *
 * @return The current baud rate, or 0 if the UART is closed.
 */
CC_EXTERN uint_fast32_t CC_TMPL_FN(baudrate)(void);

/**
 * @fn bool uart[0..3]_rx_is_ready(void)
 * @brief Checks if received data is available in the UART receive buffer.
 *
 * Queries the RXC flag in UCSRA to determine if a complete byte has been received and is ready to be read.
 *
 * @return true if data is available, false otherwise.
 */
CC_EXTERN bool CC_TMPL_FN(rx_is_ready)(void);

/**
 * @fn uint_fast8_t uart[0..3]_rx_error(void)
 * @brief Retrieves the current receive error status.
 *
 * Checks for data overrun (DOR) and parity error (UPE) in UCSRA. Frame error (FE) is skipped as it cannot be reliably cleared.
 * Returns an error code from errors.h (e.g., ERR_SUCCESS, ERR_IO_PARITY_ERROR, ERR_DATA_OVERFLOW).
 *
 * @return The error code, or ERR_SUCCESS if no error.
 */
CC_EXTERN uint_fast8_t CC_TMPL_FN(rx_error)(void);

/**
 * @fn void uart[0..3]_rx_clear_errors(void)
 * @brief Clears all receive error flags.
 *
 * Resets the FE, DOR, and UPE flags in UCSRA to clear any pending error states.
 */
CC_EXTERN void CC_TMPL_FN(rx_clear_errors)(void);

/**
 * @fn uint_fast8_t uart[0..3]_rx_read8(void)
 * @brief Reads a byte from the UART receive buffer.
 *
 * Retrieves the received data from the UDR register. This also clears the RXC flag.
 * Should be called only after confirming data is ready with uart[0..3]_rx_is_ready().
 *
 * @return The received 8-bit data.
 */
CC_EXTERN uint_fast8_t CC_TMPL_FN(rx_read8)(void);

/**
 * @fn void uart[0..3]_tx_enable_int(void)
 * @brief Enables the UART data register empty interrupt.
 *
 * Sets the UDRIE flag in UCSRB to enable interrupts when the transmit buffer is empty.
 * This is used for interrupt-driven transmission.
 */
CC_EXTERN void CC_TMPL_FN(tx_enable_int)(void);

/**
 * @fn bool uart[0..3]_tx_is_int_enabled(void)
 * @brief Checks if the data register empty interrupt is enabled.
 *
 * Queries the UDRIE flag in UCSRB.
 *
 * @return true if enabled, false otherwise.
 */
CC_EXTERN bool CC_TMPL_FN(tx_is_int_enabled)(void);

/**
 * @fn bool uart[0..3]_tx_is_ready(void)
 * @brief Checks if the UART transmit buffer is ready for new data.
 *
 * Queries the UDRE flag in UCSRA to determine if the transmit buffer (UDR) can accept a new byte.
 *
 * @return true if ready, false if busy.
 */
CC_EXTERN bool CC_TMPL_FN(tx_is_ready)(void);

/**
 * @fn bool uart[0..3]_tx_is_available(void)
 * @brief Checks if the transmit buffer is available for polling-based transmission.
 *
 * Returns true only if the data register empty interrupt is disabled and the buffer is ready.
 * This prevents interference between polled and interrupt-driven transmission.
 *
 * @return true if available for writing, false otherwise.
 */
CC_EXTERN bool CC_TMPL_FN(tx_is_available)(void);

/**
 * @fn void uart[0..3]_tx_write8(const uint_fast8_t data)
 * @brief Writes a byte to the UART transmit buffer.
 *
 * Atomically writes the data to UDR and clears the TXC flag by setting it (to ensure proper interrupt handling).
 * Should be called only after confirming readiness with uart[0..3]_tx_is_ready() or uart[0..3]_tx_is_available().
 *
 * @param data The 8-bit data to transmit.
 */
CC_EXTERN void CC_TMPL_FN(tx_write8)(const uint_fast8_t data);
