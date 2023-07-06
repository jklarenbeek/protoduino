
CC_EXTERN void CC_FN(on_rx_complete)(uart_on_rx_complete_fn callback);
CC_EXTERN void CC_FN(on_rx_error)(uart_on_rx_error_fn callback);
CC_EXTERN void CC_FN(on_tx_complete)(uart_on_tx_complete_fn callback);

CC_EXTERN void CC_FN(open)(uint32_t baud);
CC_EXTERN void CC_FN(openex)(uint32_t baud, uint8_t options);
CC_EXTERN void CC_FN(close)(void);
CC_EXTERN uint_fast32_t CC_FN(baudrate)(void);

CC_EXTERN bool CC_FN(rx_is_ready)(void);
CC_EXTERN uint_fast8_t CC_FN(rx_error)(void);
CC_EXTERN uint_fast8_t CC_FN(rx_read8)(void);

CC_EXTERN void CC_FN(tx_enable)();
CC_EXTERN bool CC_FN(tx_is_enabled)(void);
CC_EXTERN bool CC_FN(tx_is_ready)(void);
CC_EXTERN bool CC_FN(tx_is_available)(void);
CC_EXTERN void CC_FN(tx_write8)(const uint_fast8_t data);
