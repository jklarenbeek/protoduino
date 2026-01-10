// file: ./src/sys/events.h

// -----------------------------------------------------------------------------
// HYPER-BYTE EVENT TOPOLOGY
// -----------------------------------------------------------------------------
// Generated: 2025-01-10T00:00:00.000Z
// Geometry:  16x16 Matrix (0..255) mapped to 4 Attractor Basins.
// Hierarchy: ROOT(4) -> DOMAIN(12) -> SECTION(48) -> LEAF(192)
// Logic:     Parent = ((Child << 1) & 0xF0) | ((Child >>> 1) & 0x0F)
// Purpose:   Event taxonomy for process_post() and IPC message pump system
// -----------------------------------------------------------------------------

#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <stdint.h>

// -----------------------------------------------------------------------------
// [QUADRANT 1/4] ROOT: 0x00 (UNBALANCED|ABSTRACT|TWIN|MIRROR|ROOT|RESERVED)
// System Lifecycle & Initialization Events
// -----------------------------------------------------------------------------

#define EVENT_SYSTEM_ROOT           0x00  // 00000000 UNBALANCED|ABSTRACT|TWIN|MIRROR|ROOT|RESERVED - System root attractor (reserved)
#define EVENT_NONE                  EVENT_SYSTEM_ROOT

// Domain: System Lifecycle (0x01)
#define EVENT_PROCESS_INIT          0x01  // 00000001 UNBALANCED|DOMAIN|RESERVED - Lifecycle management domain - Process initialization requested

// Section: Process Lifecycle (0x02)
#define EVENT_PROCESS_READY         0x02  // 00000010 UNBALANCED|SECTION|RESERVED - Process lifecycle section - Process ready for work
#define EVENT_PROCESS_POLL          0x04  // 00000100 UNBALANCED|LEAF - Process poll request
#define EVENT_PROCESS_STARTED       0x05  // 00000101 UNBALANCED|LEAF - Process has started execution
#define EVENT_PROCESS_ACTIVE        0x84  // 10000100 UNBALANCED|LEAF - Process is actively running
#define EVENT_PROCESS_CONTINUE      0x85  // 10000101 UNBALANCED|LEAF - Process continue request

// Section: Process Termination (0x03)
#define EVENT_PROCESS_PAUSED        0x03  // 00000011 UNBALANCED|SECTION|RESERVED - Process termination section - Process pause request
#define EVENT_PROCESS_EXIT          0x06  // 00000110 UNBALANCED|LEAF - Process exit requested
#define EVENT_PROCESS_STOPPED       0x07  // 00000111 UNBALANCED|LEAF - Process stopped cleanly
#define EVENT_PROCESS_TERMINATED    0x86  // 10000110 UNBALANCED|LEAF - Process terminated
#define EVENT_PROCESS_ERROR         0x87  // 10000111 BALANCED|SHADOW|LEAF - Process recieves an error

// Section: System State Changes (0x82)
#define EVENT_SYS_STATE_SEC         0x82  // 10000010 UNBALANCED|SECTION - System state section
#define EVENT_SYSTEM_BOOT           0x44  // 01000100 UNBALANCED|TWIN|LEAF - System boot sequence start
#define EVENT_SYSTEM_READY          0x45  // 01000101 UNBALANCED|LEAF - System ready for operation
#define EVENT_SYSTEM_SUSPEND        0xC4  // 11000100 UNBALANCED|LEAF - System entering suspend
#define EVENT_SYSTEM_RESUME         0xC5  // 11000101 BALANCED|LEAF - System resuming from suspend

// Section: Power Management (0x83)
#define EVENT_POWER_SEC             0x83  // 10000011 UNBALANCED|SECTION - Power management section
#define EVENT_POWER_ON              0x46  // 01000110 UNBALANCED|LEAF - Power on event
#define EVENT_POWER_LOW             0x47  // 01000111 BALANCED|LEAF - Low power warning
#define EVENT_POWER_CRITICAL        0xC6  // 11000110 BALANCED|LEAF - Critical power level
#define EVENT_POWER_OFF             0xC7  // 11000111 UNBALANCED|LEAF - Power off requested

// Domain: Scheduler Events (0x80)
#define EVENT_SCHEDULER_DOMAIN      0x80  // 10000000 UNBALANCED|DOMAIN - Scheduler control domain

// Section: Scheduling Control (0x40)
#define EVENT_SCHED_CTRL_SEC        0x40  // 01000000 UNBALANCED|SECTION - Scheduler control section
#define EVENT_SCHEDULE_TICK         0x20  // 00100000 UNBALANCED|LEAF - Scheduler tick event
#define EVENT_SCHEDULE_POLL         0x21  // 00100001 UNBALANCED|LEAF - Poll request for process
#define EVENT_SCHEDULE_YIELD        0xA0  // 10100000 UNBALANCED|LEAF - Process yield requested
#define EVENT_SCHEDULE_PREEMPT      0xA1  // 10100001 UNBALANCED|LEAF - Preemption event

// Section: Priority & Queue (0x41)
#define EVENT_PRIORITY_SEC          0x41  // 01000001 UNBALANCED|SECTION - Priority management section
#define EVENT_PRIORITY_CHANGE       0x22  // 00100010 UNBALANCED|TWIN|LEAF - Process priority changed
#define EVENT_PRIORITY_BOOST        0x23  // 00100011 UNBALANCED|LEAF - Priority temporarily boosted
#define EVENT_QUEUE_FULL            0xA2  // 10100010 UNBALANCED|LEAF - Event queue full
#define EVENT_QUEUE_AVAILABLE       0xA3  // 10100011 BALANCED|LEAF - Queue space available

// Section: Timer Events (0xC0)
#define EVENT_TIMER_SEC             0xC0  // 11000000 UNBALANCED|SECTION - Timer event section
#define EVENT_TIMER_EXPIRED         0x60  // 01100000 UNBALANCED|LEAF - Timer expired
#define EVENT_TIMER_STARTED         0x61  // 01100001 UNBALANCED|LEAF - Timer started
#define EVENT_TIMER_STOPPED         0xE0  // 11100000 UNBALANCED|LEAF - Timer stopped
#define EVENT_TIMER_RESET           0xE1  // 11100001 BALANCED|SHADOW|LEAF - Timer reset

// Section: Watchdog & Monitoring (0xC1)
#define EVENT_WATCHDOG_SEC          0xC1  // 11000001 UNBALANCED|SECTION - Watchdog section
#define EVENT_WATCHDOG_TRIGGER      0x62  // 01100010 UNBALANCED|LEAF - Watchdog trigger
#define EVENT_WATCHDOG_RESET        0x63  // 01100011 BALANCED|LEAF - Watchdog reset
#define EVENT_MONITOR_ALERT         0xE2  // 11100010 BALANCED|LEAF - System monitor alert
#define EVENT_MONITOR_CLEAR         0xE3  // 11100011 UNBALANCED|LEAF - Monitor alert cleared

// Domain: Thread & Synchronization (0x81)
#define EVENT_THREAD_DOMAIN         0x81  // 10000001 UNBALANCED|MIRROR|DOMAIN - Thread synchronization domain

// Section: Thread Control (0x42)
#define EVENT_THREAD_CTRL_SEC       0x42  // 01000010 UNBALANCED|MIRROR|SECTION - Thread control section
#define EVENT_THREAD_CREATE         0x24  // 00100100 UNBALANCED|MIRROR|LEAF - Thread created
#define EVENT_THREAD_SPAWN          0x25  // 00100101 UNBALANCED|LEAF - Thread spawned
#define EVENT_THREAD_JOIN           0xA4  // 10100100 UNBALANCED|LEAF - Thread join requested
#define EVENT_THREAD_DETACH         0xA5  // 10100101 BALANCED|SHADOW|MIRROR|LEAF - Thread detached

// Section: Synchronization (0x43)
#define EVENT_SYNC_SEC              0x43  // 01000011 UNBALANCED|SECTION - Synchronization section
#define EVENT_MUTEX_ACQUIRED        0x26  // 00100110 UNBALANCED|LEAF - Mutex acquired
#define EVENT_MUTEX_RELEASED        0x27  // 00100111 BALANCED|LEAF - Mutex released
#define EVENT_SEMAPHORE_POST        0xA6  // 10100110 BALANCED|LEAF - Semaphore posted
#define EVENT_SEMAPHORE_WAIT        0xA7  // 10100111 UNBALANCED|LEAF - Semaphore wait

// Section: Condition Variables (0xC2)
#define EVENT_CONDVAR_SEC           0xC2  // 11000010 UNBALANCED|SECTION - Condition variable section
#define EVENT_CONDVAR_SIGNAL        0x64  // 01100100 UNBALANCED|LEAF - Condition signaled
#define EVENT_CONDVAR_BROADCAST     0x65  // 01100101 BALANCED|LEAF - Condition broadcast
#define EVENT_CONDVAR_WAIT          0xE4  // 11100100 BALANCED|LEAF - Condition wait entered
#define EVENT_CONDVAR_TIMEOUT       0xE5  // 11100101 UNBALANCED|LEAF - Condition wait timeout

// Section: Barriers & Latches (0xC3)
#define EVENT_BARRIER_SEC           0xC3  // 11000011 BALANCED|SHADOW|MIRROR|SECTION - Barrier section
#define EVENT_BARRIER_WAIT          0x66  // 01100110 BALANCED|TWIN|MIRROR|LEAF - Barrier wait
#define EVENT_BARRIER_RELEASE       0x67  // 01100111 UNBALANCED|LEAF - Barrier released
#define EVENT_LATCH_COUNT           0xE6  // 11100110 UNBALANCED|LEAF - Latch count decremented
#define EVENT_LATCH_COMPLETE        0xE7  // 11100111 UNBALANCED|MIRROR|LEAF - Latch completed

// -----------------------------------------------------------------------------
// [QUADRANT 2/4] ROOT: 0x55 (BALANCED|MOVING|TWIN|ROOT)
// User Input & Human Interface Events
// -----------------------------------------------------------------------------

#define EVENT_INPUT_ROOT            0x55  // 01010101 BALANCED|MOVING|TWIN|ROOT - User input root attractor

// Domain: Keyboard Input (0x2A)
#define EVENT_KEYBOARD_DOMAIN       0x2A  // 00101010 UNBALANCED|DOMAIN - Keyboard input domain

// Section: Key Press/Release (0x14)
#define EVENT_KEY_BASIC_SEC         0x14  // 00010100 UNBALANCED|SECTION - Basic key section
#define EVENT_KEY_PRESSED           0x08  // 00001000 UNBALANCED|LEAF - Key pressed down
#define EVENT_KEY_RELEASED          0x09  // 00001001 UNBALANCED|LEAF - Key released
#define EVENT_KEY_REPEAT            0x88  // 10001000 UNBALANCED|TWIN|LEAF - Key auto-repeat
#define EVENT_KEY_CHORD             0x89  // 10001001 UNBALANCED|LEAF - Key chord detected

// Section: Modifier Keys (0x15)
#define EVENT_KEY_MOD_SEC           0x15  // 00010101 UNBALANCED|SECTION - Modifier key section
#define EVENT_KEY_SHIFT             0x0A  // 00001010 UNBALANCED|LEAF - Shift key state change
#define EVENT_KEY_CTRL              0x0B  // 00001011 UNBALANCED|LEAF - Control key state change
#define EVENT_KEY_ALT               0x8A  // 10001010 UNBALANCED|LEAF - Alt key state change
#define EVENT_KEY_META              0x8B  // 10001011 BALANCED|LEAF - Meta/Win key state change

// Section: Special Keys (0x94)
#define EVENT_KEY_SPECIAL_SEC       0x94  // 10010100 UNBALANCED|SECTION - Special key section
#define EVENT_KEY_FUNCTION          0x48  // 01001000 UNBALANCED|LEAF - Function key pressed
#define EVENT_KEY_NAVIGATION        0x49  // 01001001 UNBALANCED|LEAF - Navigation key (arrows, home, end)
#define EVENT_KEY_MEDIA             0xC8  // 11001000 UNBALANCED|LEAF - Media control key
#define EVENT_KEY_SYSTEM            0xC9  // 11001001 BALANCED|LEAF - System key (power, sleep)

// Section: Input Method (0x95)
#define EVENT_INPUT_METHOD_SEC      0x95  // 10010101 BALANCED|SECTION - Input method section
#define EVENT_IME_START             0x4A  // 01001010 UNBALANCED|LEAF - IME composition started
#define EVENT_IME_UPDATE            0x4B  // 01001011 BALANCED|SHADOW|LEAF - IME composition updated
#define EVENT_IME_COMMIT            0xCA  // 11001010 BALANCED|LEAF - IME text committed
#define EVENT_IME_CANCEL            0xCB  // 11001011 UNBALANCED|LEAF - IME composition cancelled

// Domain: Mouse & Pointer (0x2B)
#define EVENT_MOUSE_DOMAIN          0x2B  // 00101011 BALANCED|DOMAIN - Mouse/pointer domain

// Section: Mouse Buttons (0x16)
#define EVENT_MOUSE_BTN_SEC         0x16  // 00010110 UNBALANCED|SECTION - Mouse button section
#define EVENT_MOUSE_LEFT_DOWN       0x0C  // 00001100 UNBALANCED|LEAF - Left button pressed
#define EVENT_MOUSE_LEFT_UP         0x0D  // 00001101 UNBALANCED|LEAF - Left button released
#define EVENT_MOUSE_RIGHT_DOWN      0x8C  // 10001100 UNBALANCED|LEAF - Right button pressed
#define EVENT_MOUSE_RIGHT_UP        0x8D  // 10001101 BALANCED|LEAF - Right button released

// Section: Mouse Movement (0x17)
#define EVENT_MOUSE_MOVE_SEC        0x17  // 00010111 BALANCED|SECTION - Mouse movement section
#define EVENT_MOUSE_MOVE            0x0E  // 00001110 UNBALANCED|LEAF - Mouse moved
#define EVENT_MOUSE_DRAG            0x0F  // 00001111 BALANCED|SHADOW|LEAF - Mouse drag operation
#define EVENT_MOUSE_ENTER           0x8E  // 10001110 BALANCED|LEAF - Mouse entered region
#define EVENT_MOUSE_LEAVE           0x8F  // 10001111 UNBALANCED|LEAF - Mouse left region

// Section: Mouse Wheel (0x96)
#define EVENT_WHEEL_SEC             0x96  // 10010110 BALANCED|SHADOW|SECTION - Mouse wheel section
#define EVENT_WHEEL_VERTICAL        0x4C  // 01001100 UNBALANCED|LEAF - Vertical scroll
#define EVENT_WHEEL_HORIZONTAL      0x4D  // 01001101 BALANCED|LEAF - Horizontal scroll
#define EVENT_WHEEL_TILT            0xCC  // 11001100 BALANCED|TWIN|LEAF - Wheel tilt
#define EVENT_WHEEL_ZOOM            0xCD  // 11001101 UNBALANCED|LEAF - Zoom gesture via wheel

// Section: Advanced Pointer (0x97)
#define EVENT_POINTER_SEC           0x97  // 10010111 UNBALANCED|SECTION - Advanced pointer section
#define EVENT_POINTER_DOWN          0x4E  // 01001110 BALANCED|LEAF - Pointer/pen down
#define EVENT_POINTER_UP            0x4F  // 01001111 UNBALANCED|LEAF - Pointer/pen up
#define EVENT_POINTER_HOVER         0xCE  // 11001110 UNBALANCED|LEAF - Pointer hovering
#define EVENT_POINTER_CANCEL        0xCF  // 11001111 UNBALANCED|LEAF - Pointer cancelled

// Domain: Touch & Gesture (0xAB)
#define EVENT_TOUCH_DOMAIN          0xAB  // 10101011 UNBALANCED|DOMAIN - Touch/gesture domain

// Section: Touch Events (0x56)
#define EVENT_TOUCH_SEC             0x56  // 01010110 BALANCED|SECTION - Touch event section
#define EVENT_TOUCH_START           0x2C  // 00101100 UNBALANCED|LEAF - Touch started
#define EVENT_TOUCH_MOVE            0x2D  // 00101101 BALANCED|SHADOW|LEAF - Touch moved
#define EVENT_TOUCH_END             0xAC  // 10101100 BALANCED|LEAF - Touch ended
#define EVENT_TOUCH_CANCEL          0xAD  // 10101101 UNBALANCED|LEAF - Touch cancelled

// Section: Multi-Touch (0x57)
#define EVENT_MULTITOUCH_SEC        0x57  // 01010111 UNBALANCED|SECTION - Multi-touch section
#define EVENT_TOUCH_TAP             0x2E  // 00101110 BALANCED|LEAF - Single tap
#define EVENT_TOUCH_DOUBLE_TAP      0x2F  // 00101111 UNBALANCED|LEAF - Double tap
#define EVENT_TOUCH_LONG_PRESS      0xAE  // 10101110 UNBALANCED|LEAF - Long press
#define EVENT_TOUCH_FORCE           0xAF  // 10101111 UNBALANCED|LEAF - Force touch/3D touch

// Section: Gestures (0xD6)
#define EVENT_GESTURE_SEC           0xD6  // 11010110 UNBALANCED|SECTION - Gesture section
#define EVENT_GESTURE_SWIPE         0x6C  // 01101100 BALANCED|LEAF - Swipe gesture
#define EVENT_GESTURE_PINCH         0x6D  // 01101101 UNBALANCED|LEAF - Pinch gesture
#define EVENT_GESTURE_ROTATE        0xEC  // 11101100 UNBALANCED|LEAF - Rotation gesture
#define EVENT_GESTURE_PAN           0xED  // 11101101 UNBALANCED|LEAF - Pan gesture

// Section: Custom Gestures (0xD7)
#define EVENT_CUSTOM_GESTURE_SEC    0xD7  // 11010111 UNBALANCED|SECTION - Custom gesture section
#define EVENT_GESTURE_CUSTOM_1      0x6E  // 01101110 UNBALANCED|LEAF - Custom gesture 1
#define EVENT_GESTURE_CUSTOM_2      0x6F  // 01101111 UNBALANCED|LEAF - Custom gesture 2
#define EVENT_GESTURE_CUSTOM_3      0xEE  // 11101110 UNBALANCED|TWIN|LEAF - Custom gesture 3
#define EVENT_GESTURE_RECOGNIZED    0xEF  // 11101111 UNBALANCED|LEAF - Gesture recognition complete

// -----------------------------------------------------------------------------
// [QUADRANT 3/4] ROOT: 0xAA (BALANCED|MOVING|TWIN|ROOT)
// Communication & I/O Events
// -----------------------------------------------------------------------------

#define EVENT_IO_ROOT               0xAA  // 10101010 BALANCED|MOVING|TWIN|ROOT - I/O and communication root

// Domain: Serial Communication (0x54)
#define EVENT_SERIAL_DOMAIN         0x54  // 01010100 UNBALANCED|DOMAIN - Serial communication domain

// Section: UART Events (0x28)
#define EVENT_UART_SEC              0x28  // 00101000 UNBALANCED|SECTION - UART section
#define EVENT_UART_RX_READY         0x10  // 00010000 UNBALANCED|LEAF - UART receive ready
#define EVENT_UART_RX_COMPLETE      0x11  // 00010001 UNBALANCED|TWIN|LEAF - UART receive complete
#define EVENT_UART_TX_READY         0x90  // 10010000 UNBALANCED|LEAF - UART transmit ready
#define EVENT_UART_TX_COMPLETE      0x91  // 10010001 UNBALANCED|LEAF - UART transmit complete

// Section: Serial Errors (0x29) - OBSOLETE
// TODO: replace this section, since we have only one error event `EVENT_PROCESS_ERROR`.
//       the error itself can be given as data to the `EVENT_PROCESS_ERROR`.
#define EVENT_SERIAL_ERR_SEC        0x29  // 00101001 UNBALANCED|SECTION - Serial error section
#define EVENT_UART_OVERRUN          0x12  // 00010010 UNBALANCED|LEAF - UART overrun detected
#define EVENT_UART_FRAMING          0x13  // 00010011 UNBALANCED|LEAF - UART framing error
#define EVENT_UART_PARITY           0x92  // 10010010 UNBALANCED|LEAF - UART parity error
#define EVENT_UART_BREAK            0x93  // 10010011 BALANCED|LEAF - UART break detected

// Section: SPI Events (0xA8)
#define EVENT_SPI_SEC               0xA8  // 10101000 UNBALANCED|SECTION - SPI section
#define EVENT_SPI_TRANSFER_START    0x50  // 01010000 UNBALANCED|LEAF - SPI transfer started
#define EVENT_SPI_TRANSFER_DONE     0x51  // 01010001 UNBALANCED|LEAF - SPI transfer complete
#define EVENT_SPI_SELECT            0xD0  // 11010000 UNBALANCED|LEAF - SPI slave selected
#define EVENT_SPI_DESELECT          0xD1  // 11010001 BALANCED|LEAF - SPI slave deselected

// Section: I2C Events (0xA9)
#define EVENT_I2C_SEC               0xA9  // 10101001 BALANCED|SECTION - I2C section
#define EVENT_I2C_START             0x52  // 01010010 UNBALANCED|LEAF - I2C start condition
#define EVENT_I2C_STOP              0x53  // 01010011 BALANCED|LEAF - I2C stop condition
#define EVENT_I2C_ACK               0xD2  // 11010010 BALANCED|SHADOW|LEAF - I2C acknowledge
#define EVENT_I2C_NACK              0xD3  // 11010011 UNBALANCED|LEAF - I2C not-acknowledge

// Domain: Network Events (0xD4)
#define EVENT_NETWORK_DOMAIN        0xD4  // 11010100 BALANCED|DOMAIN - Network communication domain

// Section: Connection (0x68)
#define EVENT_NET_CONN_SEC          0x68  // 01101000 UNBALANCED|SECTION - Network connection section
#define EVENT_NET_CONNECT           0x30  // 00110000 UNBALANCED|LEAF - Network connection established
#define EVENT_NET_DISCONNECT        0x31  // 00110001 UNBALANCED|LEAF - Network disconnected
#define EVENT_NET_RECONNECT         0xB0  // 10110000 UNBALANCED|LEAF - Network reconnection attempt
#define EVENT_NET_TIMEOUT           0xB1  // 10110001 BALANCED|LEAF - Network timeout

// Section: Data Transfer (0x69)
#define EVENT_NET_DATA_SEC          0x69  // 01101001 BALANCED|SHADOW|SECTION - Network data section
#define EVENT_NET_DATA_RECEIVED     0x32  // 00110010 UNBALANCED|LEAF - Network data received
#define EVENT_NET_DATA_SENT         0x33  // 00110011 BALANCED|TWIN|LEAF - Network data sent
#define EVENT_NET_PACKET_READY      0xB2  // 10110010 BALANCED|LEAF - Network packet ready
#define EVENT_NET_BUFFER_FULL       0xB3  // 10110011 UNBALANCED|LEAF - Network buffer full

// Section: Protocol Events (0xE8)
#define EVENT_PROTOCOL_SEC          0xE8  // 11101000 BALANCED|SECTION - Protocol section
#define EVENT_PROTOCOL_HANDSHAKE    0x70  // 01110000 UNBALANCED|LEAF - Protocol handshake
#define EVENT_PROTOCOL_NEGOTIATED   0x71  // 01110001 BALANCED|LEAF - Protocol negotiated
#define EVENT_PROTOCOL_UPGRADE      0xF0  // 11110000 BALANCED|SHADOW|LEAF - Protocol upgrade
#define EVENT_PROTOCOL_DOWNGRADE    0xF1  // 11110001 UNBALANCED|LEAF - Protocol downgrade

// Section: MQTT/PubSub (0xE9)
#define EVENT_MQTT_SEC              0xE9  // 11101001 UNBALANCED|SECTION - MQTT/PubSub section
#define EVENT_MQTT_CONNECT          0x72  // 01110010 BALANCED|LEAF - MQTT connected
#define EVENT_MQTT_PUBLISH          0x73  // 01110011 UNBALANCED|LEAF - MQTT message published
#define EVENT_MQTT_SUBSCRIBE        0xF2  // 11110010 UNBALANCED|LEAF - MQTT topic subscribed
#define EVENT_MQTT_MESSAGE          0xF3  // 11110011 UNBALANCED|LEAF - MQTT message received

// Domain: File & Storage (0xD5)
#define EVENT_STORAGE_DOMAIN        0xD5  // 11010101 UNBALANCED|DOMAIN - File/storage domain

// Section: File Operations (0x6A)
#define EVENT_FILE_SEC              0x6A  // 01101010 BALANCED|SECTION - File operation section
#define EVENT_FILE_OPEN             0x34  // 00110100 UNBALANCED|LEAF - File opened
#define EVENT_FILE_CLOSE            0x35  // 00110101 BALANCED|LEAF - File closed
#define EVENT_FILE_READ             0xB4  // 10110100 BALANCED|SHADOW|LEAF - File read complete
#define EVENT_FILE_WRITE            0xB5  // 10110101 UNBALANCED|LEAF - File write complete

// Section: Directory (0x6B)
#define EVENT_DIR_SEC               0x6B  // 01101011 UNBALANCED|SECTION - Directory section
#define EVENT_DIR_CREATED           0x36  // 00110110 BALANCED|LEAF - Directory created
#define EVENT_DIR_DELETED           0x37  // 00110111 UNBALANCED|LEAF - Directory deleted
#define EVENT_DIR_SCAN_START        0xB6  // 10110110 UNBALANCED|LEAF - Directory scan started
#define EVENT_DIR_SCAN_DONE         0xB7  // 10110111 UNBALANCED|LEAF - Directory scan complete

// Section: Storage Media (0xEA)
#define EVENT_MEDIA_SEC             0xEA  // 11101010 UNBALANCED|SECTION - Storage media section
#define EVENT_MEDIA_INSERTED        0x74  // 01110100 BALANCED|LEAF - Media inserted
#define EVENT_MEDIA_REMOVED         0x75  // 01110101 UNBALANCED|LEAF - Media removed
#define EVENT_MEDIA_FULL            0xF4  // 11110100 UNBALANCED|LEAF - Storage media full
#define EVENT_MEDIA_ERROR           0xF5  // 11110101 UNBALANCED|LEAF - Storage media error

// Section: Flash/EEPROM (0xEB)
#define EVENT_FLASH_SEC             0xEB  // 11101011 UNBALANCED|SECTION - Flash/EEPROM section
#define EVENT_FLASH_ERASE_START     0x76  // 01110110 UNBALANCED|LEAF - Flash erase started
#define EVENT_FLASH_ERASE_DONE      0x77  // 01110111 UNBALANCED|TWIN|LEAF - Flash erase complete
#define EVENT_FLASH_PROGRAM         0xF6  // 11110110 UNBALANCED|LEAF - Flash programmed
#define EVENT_FLASH_VERIFY          0xF7  // 11110111 UNBALANCED|LEAF - Flash verification complete

// -----------------------------------------------------------------------------
// [QUADRANT 4/4] ROOT: 0xFF (UNBALANCED|ABSTRACT|TWIN|MIRROR|ROOT)
// Hardware, Sensors & Display Events
// -----------------------------------------------------------------------------

#define EVENT_HARDWARE_ROOT         0xFF  // 11111111 UNBALANCED|ABSTRACT|TWIN|MIRROR|ROOT - Hardware root attractor

// Domain: GPIO & Digital I/O (0x7E)
#define EVENT_GPIO_DOMAIN           0x7E  // 01111110 UNBALANCED|MIRROR|DOMAIN - GPIO domain

// Section: Pin State (0x3C)
#define EVENT_PIN_SEC               0x3C  // 00111100 BALANCED|SHADOW|MIRROR|SECTION - Pin state section
#define EVENT_PIN_HIGH              0x18  // 00011000 UNBALANCED|MIRROR|LEAF - Pin went high
#define EVENT_PIN_LOW               0x19  // 00011001 UNBALANCED|LEAF - Pin went low
#define EVENT_PIN_TOGGLE            0x98  // 10011000 UNBALANCED|LEAF - Pin toggled
#define EVENT_PIN_CHANGE            0x99  // 10011001 BALANCED|TWIN|MIRROR|LEAF - Pin change interrupt

// Section: Interrupts (0x3D)
#define EVENT_INTERRUPT_SEC         0x3D  // 00111101 UNBALANCED|SECTION - Interrupt section
#define EVENT_INT_RISING            0x1A  // 00011010 UNBALANCED|LEAF - Rising edge interrupt
#define EVENT_INT_FALLING           0x1B  // 00011011 BALANCED|LEAF - Falling edge interrupt
#define EVENT_INT_BOTH              0x9A  // 10011010 BALANCED|LEAF - Both edges interrupt
#define EVENT_INT_LEVEL             0x9B  // 10011011 UNBALANCED|LEAF - Level-triggered interrupt

// Section: PWM & Analog (0xBC)
#define EVENT_PWM_SEC               0xBC  // 10111100 UNBALANCED|SECTION - PWM section
#define EVENT_PWM_CYCLE_COMPLETE    0x58  // 01011000 UNBALANCED|LEAF - PWM cycle complete
#define EVENT_PWM_DUTY_CHANGED      0x59  // 01011001 BALANCED|LEAF - PWM duty cycle changed
#define EVENT_ADC_CONVERSION_DONE   0xD8  // 11011000 BALANCED|LEAF - ADC conversion complete
#define EVENT_DAC_UPDATE            0xD9  // 11011001 UNBALANCED|LEAF - DAC output updated

// Section: Timers & Counters (0xBD)
#define EVENT_COUNTER_SEC           0xBD  // 10111101 UNBALANCED|MIRROR|SECTION - Counter section
#define EVENT_COUNTER_OVERFLOW      0x5A  // 01011010 BALANCED|SHADOW|MIRROR|LEAF - Counter overflow
#define EVENT_COUNTER_MATCH         0x5B  // 01011011 UNBALANCED|LEAF - Counter match event
#define EVENT_COUNTER_CAPTURE       0xDA  // 11011010 UNBALANCED|LEAF - Input capture event
#define EVENT_COUNTER_COMPARE       0xDB  // 11011011 UNBALANCED|MIRROR|LEAF - Compare match event

// Domain: Sensors (0x7F)
#define EVENT_SENSOR_DOMAIN         0x7F  // 01111111 UNBALANCED|DOMAIN - Sensor domain

// Section: Motion Sensors (0x3E)
#define EVENT_MOTION_SEC            0x3E  // 00111110 UNBALANCED|SECTION - Motion sensor section
#define EVENT_ACCEL_DATA            0x1C  // 00011100 UNBALANCED|LEAF - Accelerometer data ready
#define EVENT_GYRO_DATA             0x1D  // 00011101 BALANCED|LEAF - Gyroscope data ready
#define EVENT_COMPASS_DATA          0x9C  // 10011100 BALANCED|LEAF - Compass/magnetometer data
#define EVENT_IMU_CALIBRATED        0x9D  // 10011101 UNBALANCED|LEAF - IMU calibration complete

// Section: Environmental (0x3F)
#define EVENT_ENV_SEC               0x3F  // 00111111 UNBALANCED|SECTION - Environmental sensor section
#define EVENT_TEMP_READING          0x1E  // 00011110 BALANCED|SHADOW|LEAF - Temperature reading ready
#define EVENT_HUMIDITY_READING      0x1F  // 00011111 UNBALANCED|LEAF - Humidity reading ready
#define EVENT_PRESSURE_READING      0x9E  // 10011110 UNBALANCED|LEAF - Pressure reading ready
#define EVENT_LIGHT_READING         0x9F  // 10011111 UNBALANCED|LEAF - Light sensor reading

// Section: Proximity & Distance (0xBE)
#define EVENT_PROXIMITY_SEC         0xBE  // 10111110 UNBALANCED|SECTION - Proximity section
#define EVENT_PROXIMITY_NEAR        0x5C  // 01011100 BALANCED|LEAF - Object near
#define EVENT_PROXIMITY_FAR         0x5D  // 01011101 UNBALANCED|LEAF - Object far
#define EVENT_DISTANCE_MEASURED     0xDC  // 11011100 UNBALANCED|LEAF - Distance measurement ready
#define EVENT_ULTRASONIC_ECHO       0xDD  // 11011101 UNBALANCED|TWIN|LEAF - Ultrasonic echo received

// Section: Biometric (0xBF)
#define EVENT_BIOMETRIC_SEC         0xBF  // 10111111 UNBALANCED|SECTION - Biometric section
#define EVENT_FINGERPRINT_MATCH     0x5E  // 01011110 UNBALANCED|LEAF - Fingerprint matched
#define EVENT_HEARTRATE_READING     0x5F  // 01011111 UNBALANCED|LEAF - Heart rate reading
#define EVENT_BIOMETRIC_TIMEOUT     0xDE  // 11011110 UNBALANCED|LEAF - Biometric scan timeout
#define EVENT_BIOMETRIC_FAIL        0xDF  // 11011111 UNBALANCED|LEAF - Biometric authentication failed

// Domain: Display & Graphics (0xFE)
#define EVENT_DISPLAY_DOMAIN        0xFE  // 11111110 UNBALANCED|DOMAIN - Display domain

// Section: Screen Events (0x7C)
#define EVENT_SCREEN_SEC            0x7C  // 01111100 UNBALANCED|SECTION - Screen section
#define EVENT_SCREEN_REFRESH        0x38  // 00111000 UNBALANCED|LEAF - Screen refresh complete
#define EVENT_VSYNC                 0x39  // 00111001 BALANCED|LEAF - Vertical sync signal
#define EVENT_HSYNC                 0xB8  // 10111000 BALANCED|LEAF - Horizontal sync signal
#define EVENT_FRAME_COMPLETE        0xB9  // 10111001 UNBALANCED|LEAF - Frame rendering complete

// Section: Rendering (0x7D)
#define EVENT_RENDER_SEC            0x7D  // 01111101 UNBALANCED|SECTION - Rendering section
#define EVENT_RENDER_START          0x3A  // 00111010 BALANCED|LEAF - Rendering started
#define EVENT_RENDER_DONE           0x3B  // 00111011 UNBALANCED|LEAF - Rendering complete
#define EVENT_BUFFER_SWAP           0xBA  // 10111010 UNBALANCED|LEAF - Display buffer swapped
#define EVENT_SPRITE_UPDATE         0xBB  // 10111011 UNBALANCED|TWIN|LEAF - Sprite updated

// Section: Window Events (0xFC)
#define EVENT_WINDOW_SEC            0xFC  // 11111100 UNBALANCED|SECTION - Window section
#define EVENT_WINDOW_RESIZE         0x78  // 01111000 BALANCED|SHADOW|LEAF - Window resized
#define EVENT_WINDOW_FOCUS          0x79  // 01111001 UNBALANCED|LEAF - Window gained focus
#define EVENT_WINDOW_BLUR           0xF8  // 11111000 UNBALANCED|LEAF - Window lost focus
#define EVENT_WINDOW_MINIMIZE       0xF9  // 11111001 UNBALANCED|LEAF - Window minimized

// Section: GUI Controls (0xFD)
#define EVENT_GUI_SEC               0xFD  // 11111101 UNBALANCED|SECTION - GUI control section
#define EVENT_BUTTON_CLICK          0x7A  // 01111010 UNBALANCED|LEAF - Button clicked
#define EVENT_SLIDER_CHANGE         0x7B  // 01111011 UNBALANCED|LEAF - Slider value changed
#define EVENT_DROPDOWN_SELECT       0xFA  // 11111010 UNBALANCED|LEAF - Dropdown item selected
#define EVENT_CHECKBOX_TOGGLE       0xFB  // 11111011 UNBALANCED|LEAF - Checkbox toggled

// -----------------------------------------------------------------------------
// EVENT TAXONOMY OPERATORS & CLASSIFICATION MACROS
// -----------------------------------------------------------------------------

// Event type checking (mirrors error taxonomy structure)
#define EVENT_IS_SYSTEM(e)      (((e) & 0x80) == 0x00)  // Quadrant 1: system/lifecycle
#define EVENT_IS_INPUT(e)       (((e) & 0x80) == 0x40)  // Quadrant 2: user input
#define EVENT_IS_IO(e)          (((e) & 0x80) == 0x80)  // Quadrant 3: communication/I/O
#define EVENT_IS_HARDWARE(e)    (((e) & 0x80) == 0xC0)  // Quadrant 4: hardware/sensors

// Root determination via center operation
static inline uint8_t event_op_center(uint8_t ev) {
    return (uint8_t)(((ev << 1) & 0xF0) | ((ev >> 1) & 0x0F));
}

static inline uint8_t event_op_root(uint8_t ev) {
    uint8_t v = ev;
    for (int i = 0; i < 8 && v != 0x00 && v != 0x55 && v != 0xAA && v != 0xFF; i++) {
        v = event_op_center(v);
    }
    return v;
}

// Hierarchical depth (0=root, 1=domain, 2=section, 3=leaf)
static inline uint8_t event_op_depth(uint8_t ev) {
    uint8_t v = ev;
    uint8_t d = 0;
    while (v != 0x00 && v != 0x55 && v != 0xAA && v != 0xFF && d < 4) {
        v = event_op_center(v);
        d++;
    }
    return d;
}

#endif /* __EVENTS_H__ */
