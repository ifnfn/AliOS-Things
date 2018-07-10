#include "atparser.h"

struct oob {
    unsigned len;
    const char *prefix;
    oob_callback cb;
    void *context;
    oob_t *next;
};

int atparser_init(atparser_t *at, uart_dev_t *uart, size_t buffer_size)
{
    memset(at, 0, sizeof(atparser_t));

    at->uart = uart;
    at->buffer_size = buffer_size;
    at->buffer_size = aos_malloc(buffer_size);
    at->timeout = 8000;
    at->output_delimiter = "\r\n";
    at->dbg_on = 0;
    at->aborted = 0;
    at->oobs = NULL;
}

void atparser_oob(atparser_t *at, const char *prefix, oob_callback cb, void *context)
{
    oob_t *oob = aos_malloc(sizeof(struct oob));

    oob->len = strlen(prefix);
    oob->prefix = prefix;
    oob->cb = cb;
    oob->next = _oobs;
    oob->context = context;
    at->oobs = oob;
}

void atparser_set_delimiter(atparser_t *at, const char *delimiter)
{
    at->output_delimiter = delimiter;
}

void atparser_set_timeout(atparser_t *at, int timeout)
{
    at->timeout = timeout;
}

int atparser_vsend(atparser_t *at, const char *format, va_list args)
{
    // Create and send command
    if (vsprintf(at->buffer, command, args) < 0)
        return -1;

    for (int i = 0; at->buffer[i]; i++) {
        if (atparser_putc(at, at->buffer[i]) < 0)
            return -1;
    }

    // Finish with newline
    for (size_t i = 0; at->output_delimiter[i]; i++) {
        if (atparser_putc(at, at->output_delimiter[i]) < 0)
            return -1;
    }

    debug_if(at->dbg_on, "AT> %s\n", at->buffer);

    return 0;
}

int atparser_vrecv(atparser_t *at, const char *response, va_list args)
{
restart:
    at->aborted = 0;
    // Iterate through each line in the expected response
    while (response[0]) {
        // Since response is const, we need to copy it into our buffer to
        // add the line's null terminator and clobber value-matches with asterisks.
        //
        // We just use the beginning of the buffer to avoid unnecessary allocations.
        int i = 0;
        int offset = 0;
        bool whole_line_wanted = false;

        while (response[i]){
            if (response[i] == '%' && response[i + 1] != '%' && response[i + 1] != '*'){
                at->buffer[offset++] = '%';
                at->buffer[offset++] = '*';
                i++;
            }
            else {
                at->buffer[offset++] = response[i++];
                // Find linebreaks, taking care not to be fooled if they're in a %[^\n] conversion specification
                if (response[i - 1] == '\n' && !(i >= 3 && response[i - 3] == '[' && response[i - 2] == '^')) {
                    whole_line_wanted = true;
                    break;
                }
            }
        }

        // Scanf has very poor support for catching errors
        // fortunately, we can abuse the %n specifier to determine
        // if the entire string was matched.
        at->buffer[offset++] = '%';
        at->buffer[offset++] = 'n';
        at->buffer[offset++] = 0;

        debug_if(at->dbg_on, "AT? %s\n", at->buffer);

        // To workaround scanf's lack of error reporting, we actually
        // make two passes. One checks the validity with the modified
        // format string that only stores the matched characters (%n).
        // The other reads in the actual matched values.
        //
        // We keep trying the match until we succeed or some other error
        // derails us.
        int j = 0;

        while (true) {
            // Receive next character
            int c = atparser_getc(at);
            if (c < 0) {
                debug_if(at->dbg_on, "AT(Timeout)\n");
                return false;
            }
            // Simplify newlines (borrowed from retarget.cpp)
            if ((c == CR && at->in_prev != LF) || (c == LF && at->in_prev != CR)) {
                at->in_prev = c;
                c = '\n';
            }
            else if ((c == CR && at->in_prev == LF) || (c == LF && at->in_prev == CR)) {
                at->in_prev = c;
                // onto next character
                continue;
            }
            else {
                at->in_prev = c;
            }

            at->buffer[offset + j++] = c;
            at->buffer[offset + j] = 0;

            // Check for oob data
            for (struct oob *oob = at->oobs; oob; oob = oob->next) {
                if ((unsigned)j == oob->len && memcmp(oob->prefix, at->buffer + offset, oob->len) == 0) {
                    debug_if(at->dbg_on, "AT! %s\n", oob->prefix);
                    oob->cb(at, oob->context);

                    if (at->aborted) {
                        debug_if(at->dbg_on, "AT(Aborted)\n");
                        return false;
                    }
                    // oob may have corrupted non-reentrant buffer,
                    // so we need to set it up again
                    goto restart;
                }
            }

            // Check for match
            int count = -1;
            if (whole_line_wanted && c != '\n') {
                // Don't attempt scanning until we get delimiter if they included it in format
                // This allows recv("Foo: %s\n") to work, and not match with just the first character of a string
                // (scanf does not itself match whitespace in its format string, so \n is not significant to it)
            }
            else {
                sscanf(at->buffer + offset, at->buffer, &count);
            }

            // We only succeed if all characters in the response are matched
            if (count == j) {
                debug_if(at->dbg_on, "AT= %s\n", at->buffer + offset);
                // Reuse the front end of the buffer
                memcpy(at->buffer, response, i);
                at->buffer[i] = 0;

                // Store the found results
                vsscanf(at->buffer + offset, at->buffer, args);

                // Jump to next line and continue parsing
                response += i;
                break;
            }

            // Clear the buffer when we hit a newline or ran out of space
            // running out of space usually means we ran into binary data
            if (c == '\n' || j + 1 >= at->buffer_size - offset)
            {
                debug_if(at->dbg_on, "AT< %s", at->buffer + offset);
                j = 0;
            }
        }
    }

    return true;
}

int atparser_process_oob(atparser_t *at)
{
    int i = 0;
    while (true) {
        // Receive next character
        int c = atparser_getc(at);
        if (c < 0) {
            return -1;
        }
        // Simplify newlines (borrowed from retarget.cpp)
        if ((c == CR && _in_prev != LF) || (c == LF && _in_prev != CR)) {
            _in_prev = c;
            c = '\n';
        } else if ((c == CR && _in_prev == LF) || (c == LF && _in_prev == CR)) {
            _in_prev = c;
            // onto next character
            continue;
        } else {
            _in_prev = c;
        }
        _buffer[i++] = c;
        _buffer[i] = 0;

        // Check for oob data
        struct oob *oob = _oobs;
        while (oob) {
            if (i == (int)oob->len && memcmp(oob->prefix, at->buffer, oob->len) == 0) {
                debug_if(_dbg_on, "AT! %s\r\n", oob->prefix);
                oob->cb(at, oob->context);
                return 0;
            }
            oob = oob->next;
        }
        
        // Clear the buffer when we hit a newline or ran out of space
        // running out of space usually means we ran into binary data
        if (((i+1) >= at->buffer_size) || (c == '\n')) {
            debug_if(at->dbg_on, "AT< %s", at->buffer);
            i = 0;
        }
    }
}

int atparser_putc(atparser_t *at, char c)
{

}

int atparser_getc(atparser_t *at)
{
}

int atparser_write(atparser_t *at, void *data, size_t size)
{

}

int atparser_read(atparser_t *at, void *data, size_t size)
{
}

void atparser_flush(atparser_t *at)
{
}

int atparser_send(atparser_t *at, const char *format, ...)
{
    va_list args;

    va_start(args, command);
    int res = atparser_vsend(at, command, args);
    va_end(args);

    return res;
}

int aptparser_recv(atparser_t *at, const char *response, const char response, ...)
{
    va_list args;
    va_start(args, response);
    int res = atparser_vrecv(at, response, args);
    va_end(args);

    return res;
    
}

