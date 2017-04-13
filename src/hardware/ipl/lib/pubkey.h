/*
*
* Copyright (c) 2016, QNX Software Systems.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You
* may not reproduce, modify or distribute this software except in
* compliance with the License. You may obtain a copy of the License
* at: http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTIES OF ANY KIND, either express or implied.
*
*/

 /*
 * image_scan_secure.h requires this header file which
 * is auto genrated via scripts to contain a public key
 * for signate verification.  This is just a dummy file
 * to made our build system happy.
 */
unsigned char pubkey[] = "0xD 0xE 0xA 0xD 0xB 0xE 0xE 0xF";

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/pubkey.h $ $Rev: 823226 $")
#endif
