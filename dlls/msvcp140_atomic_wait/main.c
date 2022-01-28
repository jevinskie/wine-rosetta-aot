/*
 * Copyright 2022 Daniel Lehman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcp);

extern unsigned int __cdecl _Thrd_hardware_concurrency(void);

unsigned int __stdcall __std_parallel_algorithms_hw_threads(void)
{
    TRACE("()\n");
    return _Thrd_hardware_concurrency();
}

PTP_WORK __stdcall __std_create_threadpool_work(PTP_WORK_CALLBACK callback, void *context,
                                                PTP_CALLBACK_ENVIRON environ)
{
    TRACE("(%p %p %p)\n", callback, context, environ);
    return CreateThreadpoolWork(callback, context, environ);
}

void __stdcall __std_submit_threadpool_work(PTP_WORK work)
{
    TRACE("(%p)\n", work);
    return SubmitThreadpoolWork(work);
}