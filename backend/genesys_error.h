/* sane - Scanner Access Now Easy.

   Copyright (C) 2019 Povilas Kanapickas <povilas@radix.lt>

   This file is part of the SANE package.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

   As a special exception, the authors of SANE give permission for
   additional uses of the libraries contained in this release of SANE.

   The exception is that, if you link a SANE library with other files
   to produce an executable, this does not by itself cause the
   resulting executable to be covered by the GNU General Public
   License.  Your use of that executable is in no way restricted on
   account of linking the SANE library code into it.

   This exception does not, however, invalidate any other reasons why
   the executable file might be covered by the GNU General Public
   License.

   If you submit changes to SANE to the maintainers to be included in
   a subsequent release, you agree by submitting the changes that
   those changes may be distributed with this exception intact.

   If you write modifications of your own for SANE, it is your choice
   whether to permit this exception to apply to your modifications.
   If you do not wish that, delete this exception notice.
*/

#ifndef BACKEND_GENESYS_ERROR_H
#define BACKEND_GENESYS_ERROR_H

#include "../include/sane/config.h"
#include "../include/sane/sane.h"
#include "../include/sane/sanei_backend.h"

#include <stdexcept>
#include <cstring>
#include <string>

#define DBG_error0      0	/* errors/warnings printed even with devuglevel 0 */
#define DBG_error       1	/* fatal errors */
#define DBG_init        2	/* initialization and scanning time messages */
#define DBG_warn        3	/* warnings and non-fatal errors */
#define DBG_info        4	/* informational messages */
#define DBG_proc        5	/* starting/finishing functions */
#define DBG_io          6	/* io functions */
#define DBG_io2         7	/* io functions that are called very often */
#define DBG_data        8	/* log image data */

class SaneException : std::exception {
public:
    SaneException(SANE_Status status);
    SaneException(SANE_Status status, const char* msg);

    SaneException(const char* msg);


    SANE_Status status() const;
    const char* what() const noexcept override;

private:

    void set_msg(const char* msg);

    std::string msg_;
    SANE_Status status_;
};

/**
 * call a function and return on error
 */
#define RIE(function)                                   \
  do { status = function;                               \
    if (status != SANE_STATUS_GOOD) \
      { \
        DBG(DBG_error, "%s: %s\n", __func__, sane_strstatus (status)); \
        return status; \
      }	\
  } while (SANE_FALSE)

// call a function and throw an exception on error
#define TIE(function)                                                                              \
    do {                                                                                           \
        SANE_Status tmp_status = function;                                                         \
        if (tmp_status != SANE_STATUS_GOOD) {                                                      \
            throw SaneException(tmp_status);                                                       \
        }                                                                                          \
    } while (false)

class DebugMessageHelper {
public:
    static constexpr unsigned MAX_BUF_SIZE = 120;

    DebugMessageHelper(const char* func);
    DebugMessageHelper(const char* func, const char* format, ...)
    #ifdef __GNUC__
        __attribute__((format(printf, 3, 4)))
    #endif
    ;

    ~DebugMessageHelper();

    void status(const char* msg) { vstatus("%s", msg); }
    void vstatus(const char* format, ...)
    #ifdef __GNUC__
        __attribute__((format(printf, 2, 3)))
    #endif
    ;

    void clear() { msg_[0] = '\n'; }

private:
    const char* func_ = nullptr;
    char msg_[MAX_BUF_SIZE];
    unsigned num_exceptions_on_enter_ = 0;
};

#define DBG_HELPER(var) DebugMessageHelper var(__func__)
#define DBG_HELPER_ARGS(var, ...) DebugMessageHelper var(__func__, __VA_ARGS__)

template<class F>
SANE_Status wrap_exceptions_to_status_code(const char* func, F&& function)
{
    try {
        return function();
    } catch (const SaneException& exc) {
        return exc.status();
    } catch (const std::bad_alloc& exc) {
        return SANE_STATUS_NO_MEM;
    } catch (const std::exception& exc) {
        DBG(DBG_error, "%s: got uncaught exception: %s\n", func, exc.what());
        return SANE_STATUS_INVAL;
    } catch (...) {
        DBG(DBG_error, "%s: got unknown uncaught exception\n", func);
        return SANE_STATUS_INVAL;
    }
}

template<class F>
void catch_all_exceptions(const char* func, F&& function)
{
    try {
        function();
    } catch (const SaneException& exc) {
        DBG(DBG_error, "%s: got exception: %s\n", func, exc.what());
    } catch (const std::bad_alloc& exc) {
        DBG(DBG_error, "%s: got exception: could not allocate memory: %s\n", func, exc.what());
    } catch (const std::exception& exc) {
        DBG(DBG_error, "%s: got uncaught exception: %s\n", func, exc.what());
    } catch (...) {
        DBG(DBG_error, "%s: got unknown uncaught exception\n", func);
    }
}

inline void wrap_status_code_to_exception(SANE_Status status)
{
    if (status == SANE_STATUS_GOOD)
        return;
    throw SaneException(status);
}

#endif // BACKEND_GENESYS_ERROR_H
