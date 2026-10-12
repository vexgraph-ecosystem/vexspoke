#ifndef CLI_CONSOLE_H
#define CLI_CONSOLE_H
#include <stdint.h>
// Bare synchronous cold stdout output, not a managed terminal API. Control bytes
// are escaped, so each call produces exactly one line. No allocation or queue.
// init/drain/shutdown remain compatibility no-ops for System callers.
void Console_init(void);
void Console_log(const char *message);
// Consumes a live string block, even if stdout fails. Null reports rejection.
void Console_logString(uint8_t *message);
void Console_drain(void);
void Console_shutdown(void);
#endif
