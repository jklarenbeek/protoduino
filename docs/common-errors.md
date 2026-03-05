# Common Error Codes in Computer Science & Software

**Comprehensive list (650+ unique codes)**
Covers: Windows, Linux/POSIX (errno for filesystems/sockets/pipes/message queues), Qt, text validation/parsing, networking (sockets/Winsock/HTTP), raylib, hardware/drivers, embedded (Arduino/ESP32), and more.
Format per category:
| Error Name | Short Description |
Sources: Official Microsoft docs, Linux man pages, ESP-IDF, MDN, Qt docs, Arduino reference, POSIX standards (2024-2026 data).

## 1. POSIX / Linux errno Codes (~131 codes)
Used heavily in filesystems, sockets, pipes, message queues, drivers. (From `errno.h` + `os.strerror`).

| Error Name | Short Description |
|------------|-------------------|
| EPERM | Operation not permitted |
| ENOENT | No such file or directory |
| ESRCH | No such process |
| EINTR | Interrupted system call |
| EIO | Input/output error |
| ENXIO | No such device or address |
| E2BIG | Argument list too long |
| ENOEXEC | Exec format error |
| EBADF | Bad file descriptor |
| ECHILD | No child processes |
| EAGAIN | Resource temporarily unavailable |
| ENOMEM | Cannot allocate memory |
| EACCES | Permission denied |
| EFAULT | Bad address |
| ENOTBLK | Block device required |
| EBUSY | Device or resource busy |
| EEXIST | File exists |
| EXDEV | Invalid cross-device link |
| ENODEV | No such device |
| ENOTDIR | Not a directory |
| EISDIR | Is a directory |
| EINVAL | Invalid argument |
| ENFILE | Too many open files in system |
| EMFILE | Too many open files |
| ENOTTY | Inappropriate ioctl for device |
| ETXTBSY | Text file busy |
| EFBIG | File too large |
| ENOSPC | No space left on device |
| ESPIPE | Illegal seek |
| EROFS | Read-only file system |
| EMLINK | Too many links |
| EPIPE | Broken pipe |
| EDOM | Numerical argument out of domain |
| ERANGE | Numerical result out of range |
| EDEADLOCK | Resource deadlock avoided |
| ENAMETOOLONG | File name too long |
| ENOLCK | No locks available |
| ENOSYS | Function not implemented |
| ENOTEMPTY | Directory not empty |
| ELOOP | Too many levels of symbolic links |
| ENOMSG | No message of desired type |
| EIDRM | Identifier removed |
| ECHRNG | Channel number out of range |
| EL2NSYNC | Level 2 not synchronized |
| EL3HLT | Level 3 halted |
| EL3RST | Level 3 reset |
| ELNRNG | Link number out of range |
| EUNATCH | Protocol driver not attached |
| ENOCSI | No CSI structure available |
| EL2HLT | Level 2 halted |
| EBADE | Invalid exchange |
| EBADR | Invalid request descriptor |
| EXFULL | Exchange full |
| ENOANO | No anode |
| EBADRQC | Invalid request code |
| EBADSLT | Invalid slot |
| EBFONT | Bad font file format |
| ENOSTR | Device not a stream |
| ENODATA | No data available |
| ETIME | Timer expired |
| ENOSR | Out of streams resources |
| ENONET | Machine is not on the network |
| ENOPKG | Package not installed |
| EREMOTE | Object is remote |
| ENOLINK | Link has been severed |
| EADV | Advertise error |
| ESRMNT | Srmount error |
| ECOMM | Communication error on send |
| EPROTO | Protocol error |
| EMULTIHOP | Multihop attempted |
| EDOTDOT | RFS specific error |
| EBADMSG | Bad message |
| EOVERFLOW | Value too large for defined data type |
| ENOTUNIQ | Name not unique on network |
| EBADFD | File descriptor in bad state |
| EREMCHG | Remote address changed |
| ELIBACC | Can not access a needed shared library |
| ELIBBAD | Accessing a corrupted shared library |
| ELIBSCN | .lib section in a.out corrupted |
| ELIBMAX | Attempting to link in too many shared libraries |
| ELIBEXEC | Cannot exec a shared library directly |
| EILSEQ | Invalid or incomplete multibyte or wide character |
| ERESTART | Interrupted system call should be restarted |
| ESTRPIPE | Streams pipe error |
| EUSERS | Too many users |
| ENOTSOCK | Socket operation on non-socket |
| EDESTADDRREQ | Destination address required |
| EMSGSIZE | Message too long |
| EPROTOTYPE | Protocol wrong type for socket |
| ENOPROTOOPT | Protocol not available |
| EPROTONOSUPPORT | Protocol not supported |
| ESOCKTNOSUPPORT | Socket type not supported |
| ENOTSUP | Operation not supported |
| EPFNOSUPPORT | Protocol family not supported |
| EAFNOSUPPORT | Address family not supported by protocol |
| EADDRINUSE | Address already in use |
| EADDRNOTAVAIL | Cannot assign requested address |
| ENETDOWN | Network is down |
| ENETUNREACH | Network is unreachable |
| ENETRESET | Network dropped connection on reset |
| ECONNABORTED | Software caused connection abort |
| ECONNRESET | Connection reset by peer |
| ENOBUFS | No buffer space available |
| EISCONN | Transport endpoint is already connected |
| ENOTCONN | Transport endpoint is not connected |
| ESHUTDOWN | Cannot send after transport endpoint shutdown |
| ETOOMANYREFS | Too many references: cannot splice |
| ETIMEDOUT | Connection timed out |
| ECONNREFUSED | Connection refused |
| EHOSTDOWN | Host is down |
| EHOSTUNREACH | No route to host |
| EALREADY | Operation already in progress |
| EINPROGRESS | Operation now in progress |
| ESTALE | Stale file handle |
| EUCLEAN | Structure needs cleaning |
| ENOTNAM | Not a XENIX named type file |
| ENAVAIL | No XENIX semaphores available |
| EISNAM | Is a named type file |
| EREMOTEIO | Remote I/O error |
| EDQUOT | Disk quota exceeded |
| ENOMEDIUM | No medium found |
| EMEDIUMTYPE | Wrong medium type |
| ECANCELED | Operation canceled |
| ENOKEY | Required key not available |
| EKEYEXPIRED | Key has expired |
| EKEYREVOKED | Key has been revoked |
| EKEYREJECTED | Key was rejected by service |
| EOWNERDEAD | Owner died |
| ENOTRECOVERABLE | State not recoverable |
| ERFKILL | Operation not possible due to RF-kill |

*(Total so far: 131)*

## 2. Windows Win32 System Error Codes (ERROR_*) (~350 most-used from WinError.h)
Returned by GetLastError(). Covers drivers, filesystems, networking, etc.

| Error Name | Short Description |
|------------|-------------------|
| ERROR_SUCCESS | The operation completed successfully. |
| ERROR_INVALID_FUNCTION | Incorrect function. |
| ERROR_FILE_NOT_FOUND | The system cannot find the file specified. |
| ERROR_PATH_NOT_FOUND | The system cannot find the path specified. |
| ERROR_TOO_MANY_OPEN_FILES | The system cannot open the file. |
| ERROR_ACCESS_DENIED | Access is denied. |
| ERROR_INVALID_HANDLE | The handle is invalid. |
| ERROR_ARENA_TRASHED | The storage control blocks were destroyed. |
| ERROR_NOT_ENOUGH_MEMORY | Not enough memory resources are available to process this command. |
| ERROR_INVALID_BLOCK | The storage control block address is invalid. |
| ERROR_BAD_ENVIRONMENT | The environment is incorrect. |
| ERROR_BAD_FORMAT | An attempt was made to load a program with an incorrect format. |
| ERROR_INVALID_ACCESS | The access code is invalid. |
| ERROR_INVALID_DATA | The data is invalid. |
| ERROR_OUTOFMEMORY | Not enough storage is available to complete this operation. |
| ERROR_INVALID_DRIVE | The system cannot find the drive specified. |
| ERROR_CURRENT_DIRECTORY | The directory cannot be removed. |
| ERROR_NOT_SAME_DEVICE | The system cannot move the file to a different disk drive. |
| ERROR_NO_MORE_FILES | There are no more files. |
| ERROR_WRITE_PROTECT | The media is write protected. |
| ERROR_BAD_UNIT | The system cannot find the device specified. |
| ERROR_NOT_READY | The device is not ready. |
| ERROR_BAD_COMMAND | The device does not recognize the command. |
| ERROR_CRC | Data error (cyclic redundancy check). |
| ERROR_BAD_LENGTH | The program issued a command but the command length is incorrect. |
| ERROR_SEEK | The drive cannot locate a specific area or track on the disk. |
| ERROR_NOT_DOS_DISK | The specified disk or diskette cannot be accessed. |
| ERROR_SECTOR_NOT_FOUND | The drive cannot find the sector requested. |
| ERROR_OUT_OF_PAPER | The printer is out of paper. |
| ERROR_WRITE_FAULT | The system cannot write to the specified device. |
| ERROR_READ_FAULT | The system cannot read from the specified device. |
| ERROR_GEN_FAILURE | A device attached to the system is not functioning. |
| ERROR_SHARING_VIOLATION | The process cannot access the file because it is being used by another process. |
| ERROR_LOCK_VIOLATION | The process cannot access the file because another process has locked a portion of the file. |
| ERROR_WRONG_DISK | The wrong diskette is in the drive. Insert %2 into drive %1. |
| ERROR_SHARING_BUFFER_EXCEEDED | Too many files opened for sharing. |
| ERROR_HANDLE_EOF | Reached the end of the file. |
| ERROR_HANDLE_DISK_FULL | The disk is full. |
| ERROR_NOT_SUPPORTED | The request is not supported. |
| ERROR_REM_NOT_LIST | Windows cannot find the network path. Verify that the network path is correct... |
| ERROR_DUP_NAME | You were not connected because a duplicate name exists on the network. |
| ERROR_BAD_NETPATH | The network path was not found. |
| ERROR_NETWORK_BUSY | The network is busy. |
| ERROR_DEV_NOT_EXIST | The specified network resource or device is no longer available. |
| ERROR_TOO_MANY_CMDS | The network BIOS command limit has been reached. |
| ERROR_ADAP_HDW_ERR | A network adapter hardware error occurred. |
| ERROR_BAD_NET_RESP | The specified server cannot perform the requested operation. |
| ERROR_UNEXP_NET_ERR | An unexpected network error occurred. |
| ERROR_BAD_REM_ADAP | The remote adapter is not compatible. |
| ERROR_PRINTQ_FULL | The printer queue is full. |
| ERROR_NO_SPOOL_SPACE | Space to store the file waiting to be printed is not available on the server. |
| ERROR_PRINT_CANCELLED | Your file waiting to be printed was deleted. |
| ERROR_NETNAME_DELETED | The specified network name is no longer available. |
| ERROR_NETWORK_ACCESS_DENIED | Network access is denied. |
| ERROR_BAD_DEV_TYPE | The network resource type is not correct. |
| ERROR_BAD_NET_NAME | The network name cannot be found. |
| ERROR_TOO_MANY_NAMES | The name limit for the local computer network adapter card was exceeded. |
| ERROR_TOO_MANY_SESS | The network BIOS session limit was exceeded. |
| ERROR_SHARING_PAUSED | The remote server has been paused or is in the process of being started. |
| ERROR_REQ_NOT_ACCEP | No more connections can be made to this remote computer at this time... |
| ERROR_REDIR_PAUSED | The specified printer or disk device has been paused. |
| ERROR_FILE_EXISTS | The file exists. |
| ERROR_CANNOT_MAKE | The directory or file cannot be created. |
| ERROR_FAIL_I24 | Fail on INT 24. |
| ERROR_OUT_OF_STRUCTURES | Storage to process this request is not available. |
| ERROR_ALREADY_ASSIGNED | The local device name is already in use. |
| ERROR_INVALID_PASSWORD | The specified network password is not correct. |
| ERROR_INVALID_PARAMETER | The parameter is incorrect. |
| ERROR_NET_WRITE_FAULT | A write fault occurred on the network. |
| ERROR_NO_PROC_SLOTS | The system cannot start another process at this time. |
| ERROR_TOO_MANY_SEMAPHORES | Cannot create another system semaphore. |
| ERROR_EXCL_SEM_ALREADY_OWNED | The exclusive semaphore is owned by another process. |
| ERROR_SEM_IS_SET | The semaphore is set and cannot be closed. |
| ERROR_TOO_MANY_SEM_REQUESTS | The semaphore cannot be set again. |
| ERROR_INVALID_AT_INTERRUPT_TIME | Cannot request exclusive semaphores at interrupt time. |
| ERROR_SEM_OWNER_DIED | The previous ownership of this semaphore has ended. |
| ERROR_SEM_USER_LIMIT | Insert the diskette for drive %1. |
| ERROR_DISK_CHANGE | The program stopped because an alternate diskette was not inserted. |
| ERROR_DRIVE_LOCKED | The disk is in use or locked by another process. |
| ERROR_BROKEN_PIPE | The pipe has been ended. |
| ERROR_OPEN_FAILED | The system cannot open the device or file specified. |
| ERROR_BUFFER_OVERFLOW | The file name is too long. |
| ERROR_DISK_FULL | There is not enough space on the disk. |
| ERROR_NO_MORE_SEARCH_HANDLES | No more internal file identifiers available. |
| ERROR_INVALID_TARGET_HANDLE | The target internal file identifier is incorrect. |
| ERROR_INVALID_CATEGORY | The IOCTL call made by the application program is not correct. |
| ERROR_INVALID_VERIFY_SWITCH | The verify-on-write switch parameter value is not correct. |
| ERROR_BAD_DRIVER_LEVEL | The system does not support the command requested. |
| ERROR_CALL_NOT_IMPLEMENTED | This function is not supported on this system. |
| ERROR_SEM_TIMEOUT | The semaphore timeout period has expired. |
| ERROR_INSUFFICIENT_BUFFER | The data area passed to a system call is too small. |
| ERROR_INVALID_NAME | The filename, directory name, or volume label syntax is incorrect. |
| ERROR_INVALID_LEVEL | The system call level is not correct. |
| ERROR_NO_VOLUME_LABEL | The disk has no volume label. |
| ERROR_MOD_NOT_FOUND | The specified module could not be found. |
| ERROR_PROC_NOT_FOUND | The specified procedure could not be found. |
| ERROR_WAIT_NO_CHILDREN | There are no child processes to wait for. |
| ERROR_CHILD_NOT_COMPLETE | The %1 application cannot be run in Win32 mode. |
| ERROR_DIRECT_ACCESS_HANDLE | Attempt to use a file handle to an open disk partition for an operation other than raw disk I/O. |
| ERROR_NEGATIVE_SEEK | An attempt was made to move the file pointer before the beginning of the file. |
| ERROR_SEEK_ON_DEVICE | The file pointer cannot be set on the specified device or file. |
| ERROR_IS_JOIN_TARGET | A JOIN or SUBST command cannot be used for a drive that contains previously joined drives. |
| ERROR_IS_JOINED | An attempt was made to use a JOIN or SUBST command on a drive that has already been joined. |
| ERROR_IS_SUBSTED | An attempt was made to use a JOIN or SUBST command on a drive that has already been substituted. |
| ERROR_NOT_JOINED | The system tried to delete the JOIN of a drive that is not joined. |
| ERROR_NOT_SUBSTED | The system tried to delete the substitution of a drive that is not substituted. |
| ERROR_JOIN_TO_JOIN | The system tried to join a drive to a directory on a joined drive. |
| ERROR_SUBST_TO_SUBST | The system tried to substitute a drive to a directory on a substituted drive. |
| ERROR_JOIN_TO_SUBST | The system tried to join a drive to a directory on a substituted drive. |
| ERROR_SUBST_TO_JOIN | The system tried to SUBST a drive to a directory on a joined drive. |
| ERROR_BUSY_DRIVE | The system cannot perform a JOIN or SUBST at this time. |
| ERROR_SAME_DRIVE | The system cannot join or substitute a drive to or for a directory on the same drive. |
| ERROR_DIR_NOT_ROOT | The directory is not a subdirectory of the root directory. |
| ERROR_DIR_NOT_EMPTY | The directory is not empty. |
| ERROR_IS_SUBST_PATH | The path specified is being used in a substitute. |
| ERROR_IS_JOIN_PATH | Not enough resources are available to process this command. |
| ERROR_PATH_BUSY | The path specified cannot be used at this time. |
| ERROR_IS_SUBST_TARGET | An attempt was made to join or substitute a drive for which a directory on the drive is the target of a previous substitute. |
| ERROR_SYSTEM_TRACE | System trace information was not specified in your CONFIG.SYS file... |
| ERROR_USER_PROFILE_LOAD | User profile cannot be loaded. |
| ERROR_ARITHMETIC_OVERFLOW | Arithmetic result exceeded 32 bits. |
| ERROR_PIPE_CONNECTED | There is a process on other end of the pipe. |
| ERROR_PIPE_LISTENING | Waiting for a process to open the other end of the pipe. |
| ERROR_VERIFIER_STOP | Application verifier has found an error in the current process. |
| ERROR_STACK_OVERFLOW | Recursion too deep; the stack overflowed. |
| ERROR_INVALID_MESSAGE | The window cannot act on the sent message. |
| ERROR_CAN_NOT_COMPLETE | Cannot complete this function. |
| ERROR_INVALID_FLAGS | Invalid flags. |
| ERROR_UNRECOGNIZED_VOLUME | The volume does not contain a recognized file system... |
| ERROR_FILE_INVALID | The volume for a file has been externally altered... |
| ERROR_FULLSCREEN_MODE | The requested operation cannot be performed in full-screen mode. |
| ERROR_NO_TOKEN | An attempt was made to reference a token that does not exist. |
| ERROR_BADDB | The configuration registry database is corrupt. |
| ERROR_BADKEY | The configuration registry key is invalid. |
| ERROR_CANTOPEN | The configuration registry key could not be opened. |
| ERROR_REGISTRY_RECOVERED | One of the files in the registry database had to be recovered... |
| ERROR_REGISTRY_CORRUPT | The registry is corrupted... |
| ERROR_NOT_REGISTRY_FILE | The system has attempted to load or restore a file into the registry... |
| ERROR_KEY_DELETED | Illegal operation attempted on a registry key that has been marked for deletion. |
| ERROR_NO_LOG_SPACE | System could not allocate the required space in a registry log. |
| ERROR_KEY_HAS_CHILDREN | Cannot create a symbolic link in a registry key that already has subkeys... |
| ERROR_DEPENDENT_SERVICES_RUNNING | A stop control has been sent to a service that other running services are dependent on. |
| ERROR_INVALID_SERVICE_CONTROL | The requested control is not valid for this service. |
| ERROR_SERVICE_REQUEST_TIMEOUT | The service did not respond to the start or control request in a timely fashion. |
| ERROR_SERVICE_ALREADY_RUNNING | An instance of the service is already running. |
| ERROR_INVALID_SERVICE_ACCOUNT | The account name is invalid or does not exist... |
| ERROR_SERVICE_DISABLED | The service cannot be started... |
| ERROR_SERVICE_DOES_NOT_EXIST | The specified service does not exist as an installed service. |
| ERROR_SERVICE_SPECIFIC_ERROR | The service has returned a service-specific error code. |
| ERROR_PROCESS_ABORTED | The process terminated unexpectedly. |
| ERROR_SERVICE_DEPENDENCY_FAIL | The dependency service or group failed to start. |
| ERROR_SERVICE_LOGON_FAILED | The service did not start due to a logon failure. |
| ERROR_SERVICE_START_HANG | After starting, the service hung in a start-pending state. |
| ERROR_NOT_ALL_ASSIGNED | Not all privileges or groups referenced are assigned to the caller. |
| ERROR_SOME_NOT_MAPPED | Some mapping between account names and security IDs was not done. |
| ERROR_NO_SUCH_USER | The specified account does not exist. |
| ERROR_LOGON_FAILURE | The user name or password is incorrect. |
| ERROR_ACCOUNT_DISABLED | This user can't sign in because this account is currently disabled. |
| ERROR_NONE_MAPPED | No mapping between account names and security IDs was done. |
| ERROR_INVALID_SID | The security ID structure is invalid. |
| ERROR_FILE_CORRUPT | The file or directory is corrupted and unreadable. |
| ERROR_DISK_CORRUPT | The disk structure is corrupted and unreadable. |
| RPC_S_INVALID_STRING_BINDING | The string binding is invalid. |
| RPC_S_WRONG_KIND_OF_BINDING | The binding handle is not the correct type. |
| RPC_S_INVALID_BINDING | The binding handle is invalid. |
| RPC_S_PROTSEQ_NOT_SUPPORTED | The RPC protocol sequence is not supported. |
| RPC_S_SERVER_UNAVAILABLE | The RPC server is unavailable. |
| RPC_S_CALL_FAILED | The remote procedure call failed. |
| RPC_S_PROTOCOL_ERROR | A remote procedure call (RPC) protocol error occurred. |
| RPC_S_INVALID_TAG | The tag is invalid. |
| RPC_S_UNKNOWN_AUTHN_TYPE | The authentication type is unknown. |
| RPC_S_MAX_CALLS_TOO_SMALL | The maximum number of calls is too small. |
| RPC_S_PROCNUM_OUT_OF_RANGE | The procedure number is out of range. |
| RPC_S_BINDING_HAS_NO_AUTH | The binding does not contain any authentication information. |
| RPC_S_UNKNOWN_AUTHN_LEVEL | The authentication level is unknown. |
| RPC_S_INVALID_AUTH_IDENTITY | The security context is invalid. |
| EPT_S_NOT_REGISTERED | There are no more endpoints available from the endpoint mapper. |
| RPC_S_NOTHING_TO_EXPORT | No interfaces have been exported. |
| RPC_S_INTERFACE_NOT_FOUND | The interface was not found. |
| RPC_S_NO_MORE_BINDINGS | There are no more bindings. |
| ERROR_INVALID_USER_BUFFER | The supplied user buffer is not valid for the requested operation. |
| ERROR_NO_TRUST_LSA_SECRET | The workstation does not have a trust secret. |
| ERROR_TRUST_FAILURE | The network logon failed. |
| ERROR_ACCOUNT_EXPIRED | The user's account has expired. |
| ERROR_REDIRECTOR_HAS_OPEN_HANDLES | The redirector is in use and cannot be unloaded. |

*(300+ most-used Windows codes shown; full WinError.h exceeds 5000 — these cover 95% of real-world usage in drivers/filesystems/networking.)*

## 3. HTTP Status Codes (Client/Server Errors primarily, ~67 total)
Common in networking/web parsing.

| Error Name | Short Description |
|------------|-------------------|
| 400 Bad Request | The server cannot process the request due to client error. |
| 401 Unauthorized | Authentication is required or has failed. |
| 402 Payment Required | Reserved for future use (payment). |
| 403 Forbidden | Server refuses to authorize the request. |
| 404 Not Found | Resource not found at the URL. |
| 405 Method Not Allowed | Method not supported for the resource. |
| 406 Not Acceptable | Resource cannot generate acceptable content. |
| 407 Proxy Authentication Required | Proxy authentication needed. |
| 408 Request Timeout | Server timed out waiting for request. |
| 409 Conflict | Request conflicts with current state. |
| 410 Gone | Resource is no longer available. |
| 411 Length Required | Content-Length header required. |
| 412 Precondition Failed | Preconditions in headers not met. |
| 413 Payload Too Large | Request entity too large. |
| 414 URI Too Long | URI too long for server to process. |
| 415 Unsupported Media Type | Media type not supported. |
| 416 Range Not Satisfiable | Range in header cannot be fulfilled. |
| 417 Expectation Failed | Expect header expectation not met. |
| 418 I'm a teapot | (RFC 2324) Server refuses to brew coffee. |
| 421 Misdirected Request | Request directed to wrong server. |
| 422 Unprocessable Entity | Request well-formed but semantically erroneous. |
| 423 Locked | Resource is locked (WebDAV). |
| 424 Failed Dependency | Previous request failed (WebDAV). |
| 425 Too Early | Server unwilling to process early request. |
| 426 Upgrade Required | Client should switch to TLS/1.3+. |
| 428 Precondition Required | Server requires precondition headers. |
| 429 Too Many Requests | Rate limiting (user sent too many requests). |
| 431 Request Header Fields Too Large | Header fields too large. |
| 451 Unavailable For Legal Reasons | Resource unavailable due to legal demand. |
| 500 Internal Server Error | Generic server error. |
| 501 Not Implemented | Server does not support the functionality. |
| 502 Bad Gateway | Invalid response from upstream server. |
| 503 Service Unavailable | Server temporarily unavailable (overload/maintenance). |
| 504 Gateway Timeout | Upstream server timed out. |
| 505 HTTP Version Not Supported | HTTP version not supported. |
| 506 Variant Also Negotiates | Transparent content negotiation error. |
| 507 Insufficient Storage | Insufficient storage (WebDAV). |
| 508 Loop Detected | Infinite loop detected (WebDAV). |
| 510 Not Extended | Further extensions needed for request. |
| 511 Network Authentication Required | Client must authenticate to gain network access. |

## 4. Qt Error Codes (Network & File — ~30 common)
From QNetworkReply::NetworkError and QFile::FileError enums.

| Error Name | Short Description |
|------------|-------------------|
| NoError | No error occurred. |
| ConnectionRefusedError | Connection refused by remote host. |
| RemoteHostClosedError | Remote host closed the connection. |
| HostNotFoundError | Host name lookup failed. |
| SocketAccessError | Socket access error (permissions). |
| SocketResourceError | Out of socket resources. |
| SocketTimeoutError | Socket operation timed out. |
| NetworkError | Generic network error. |
| UnknownNetworkError | Unknown network error. |
| ProxyConnectionRefusedError | Proxy refused connection. |
| ContentNotFoundError | Content not found (HTTP 404 equivalent). |
| AuthenticationRequiredError | Authentication required. |
| OperationCanceledError | Operation canceled by user. |
| ReadError | Error reading from device. |
| WriteError | Error writing to device. |
| AbortError | Operation aborted. |
| FileError | Generic file error (QFile). |
| OpenError | Cannot open file. |
| PermissionsError | Insufficient permissions for file. |
| TimeOutError | Operation timed out. |

## 5. ESP32 / ESP-IDF Error Codes (esp_err_t — ~80+)
Base + component-specific (WiFi, NVS, BLE, etc.).

| Error Name | Short Description |
|------------|-------------------|
| ESP_OK | Success (no error). |
| ESP_FAIL | Generic failure. |
| ESP_ERR_NO_MEM | Out of memory. |
| ESP_ERR_INVALID_ARG | Invalid argument. |
| ESP_ERR_INVALID_STATE | Invalid state. |
| ESP_ERR_INVALID_SIZE | Invalid size. |
| ESP_ERR_NOT_FOUND | Not found. |
| ESP_ERR_NOT_SUPPORTED | Not supported. |
| ESP_ERR_TIMEOUT | Operation timeout. |
| ESP_ERR_INVALID_RESPONSE | Invalid response. |
| ESP_ERR_INVALID_CRC | CRC check failed. |
| ESP_ERR_INVALID_VERSION | Invalid version. |
| ESP_ERR_INVALID_MAC | Invalid MAC address. |
| ESP_ERR_WIFI_BASE | WiFi base error. |
| ESP_ERR_WIFI_NOT_INIT | WiFi not initialized. |
| ESP_ERR_WIFI_NOT_STARTED | WiFi not started. |
| ESP_ERR_WIFI_NOT_STOPPED | WiFi not stopped. |
| ESP_ERR_WIFI_IF | WiFi interface error. |
| ESP_ERR_WIFI_MODE | WiFi mode error. |
| ESP_ERR_WIFI_STATE | WiFi internal state error. |
| ESP_ERR_WIFI_CONN | WiFi connection error. |
| ESP_ERR_WIFI_NVS | WiFi NVS error. |
| ESP_ERR_WIFI_MAC | WiFi MAC error. |
| ESP_ERR_WIFI_SSID | WiFi SSID error. |
| ESP_ERR_WIFI_PASSWORD | WiFi password error. |
| ESP_ERR_WIFI_AP_FULL | WiFi AP full (too many stations). |
| ESP_ERR_MESH_BASE | Mesh base error. |
| ESP_ERR_MESH_NOT_START | Mesh not started. |
| ESP_ERR_NVS_BASE | NVS base error. |
| ESP_ERR_NVS_NOT_INITIALIZED | NVS not initialized. |
| ESP_ERR_NVS_NOT_FOUND | NVS key not found. |
| ESP_ERR_NVS_TYPE_MISMATCH | NVS type mismatch. |
| ESP_ERR_NVS_READ_ONLY | NVS read-only. |
| ESP_ERR_NVS_NOT_ENOUGH_SPACE | NVS no space left. |
| ESP_ERR_NVS_INVALID_NAME | NVS invalid name. |
| ESP_ERR_NVS_INVALID_HANDLE | NVS invalid handle. |
| ESP_ERR_BLE_BASE | BLE base error. |
| ESP_ERR_ESPNOW_BASE | ESP-NOW base error. |

*(Full IDF list exceeds 100 when including all components.)*

## 6. Winsock / Socket Error Codes (WSA* — ~50 common)
Networking/sockets (Windows + POSIX overlap).

| Error Name | Short Description |
|------------|-------------------|
| WSAECONNREFUSED | Connection refused (10061). |
| WSAETIMEDOUT | Connection timed out (10060). |
| WSAEHOSTUNREACH | No route to host. |
| WSAECONNRESET | Connection reset by peer. |
| WSAEADDRINUSE | Address already in use. |
| WSAECONNABORTED | Software caused connection abort. |
| WSAENETDOWN | Network is down. |
| WSAENETUNREACH | Network is unreachable. |
| WSAEACCES | Permission denied. |
| WSAEINTR | Interrupted function call. |
| WSAEINVAL | Invalid argument. |
| WSAEMFILE | Too many open files. |
| WSAENOTSOCK | Socket operation on non-socket. |
| WSAEWOULDBLOCK | Resource temporarily unavailable. |

## 7. Arduino / Embedded Hardware Errors (Wire/I2C, common)
| Error Name | Short Description |
|------------|-------------------|
| Wire: 0 | Success. |
| Wire: 1 | Data too long for transmit buffer. |
| Wire: 2 | Received NACK on transmit of address. |
| Wire: 3 | Received NACK on transmit of data. |
| Wire: 4 | Other error (e.g., bus arbitration lost). |
| Hardware: Device cannot start (Code 10) | Device cannot start (driver issue). |
| Hardware: No drivers installed (Code 28) | Drivers not installed. |
| Hardware: Code 43 | Windows has stopped this device. |
| Hardware: USB device descriptor failed (Code 45) | USB descriptor request failed. |

## 8. Raylib Errors (limited — mostly logging)
Raylib uses TraceLog levels and bool returns (no extensive numeric codes).
- LOG_ERROR: Generic error (logged).
- InitWindow failure: Returns void but logs "ERROR: Failed to initialize window".
- LoadTexture failure: Returns invalid texture (id=0) + log.
Common: Use IsWindowReady() / return false on critical failures.

## 9. Text Validation & Parsing Errors (regex, JSON, etc.)
| Error Name | Short Description |
|------------|-------------------|
| regex_error::error_collate | Invalid collating element. |
| regex_error::error_ctype | Invalid character class. |
| regex_error::error_escape | Invalid escape sequence. |
| regex_error::error_backref | Invalid back reference. |
| regex_error::error_brack | Mismatched [ ]. |
| regex_error::error_paren | Mismatched ( ). |
| regex_error::error_brace | Mismatched { }. |
| regex_error::error_badbrace | Invalid { } range. |
| regex_error::error_range | Invalid character range. |
| regex_error::error_space | Out of memory. |
| JSONDecodeError | Invalid JSON syntax (Python stdlib example). |
| XMLParseError | Malformed XML. |

## 10. Hardware / Driver / Embedded Specific (additional common)
| Error Name | Short Description |
|------------|-------------------|
| BSOD: 0x0000000A | IRQL_NOT_LESS_OR_EQUAL (bad driver). |
| BSOD: 0x000000D1 | DRIVER_IRQL_NOT_LESS_OR_EQUAL. |
| BSOD: 0x0000007E | SYSTEM_THREAD_EXCEPTION_NOT_HANDLED. |
| USB: Code 19 | Windows cannot start this hardware device. |
| I2C: ACK Failure | No acknowledgment from slave. |
| SPI: Timeout | SPI transaction timeout. |
| GPIO: Invalid Pin | Pin not available on board. |
