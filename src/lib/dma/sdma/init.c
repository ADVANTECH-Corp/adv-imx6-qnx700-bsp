/*
 * $QNXLicenseC: 
 * Copyright 2009, QNX Software Systems.  
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
 * This file may contain contributions from others, either as  
 * contributors under the License or as licensors under other terms.   
 * Please review this entire file for other proprietary rights or license  
 * notices, as well as the QNX Development Suite License Guide at  
 * http://licensing.qnx.com/license-guide/ for other information. 
 * $
 */

#include "sdma.h" 

#define ARM_LPAE_4G_BOUNDARY     (0x0000000100000000ULL)

////////////////////////////////////////////////////////////////////////////////
//                            PRIVATE FUNCTIONS                               //
////////////////////////////////////////////////////////////////////////////////

static int shared_mem_create() {
    int fd;
    int status;
    sdma_shmem_t * shmem_ptr;
    uint64_t memsize = sizeof(sdma_shmem_t);
    off64_t paddr64;
    uint32_t paddr;
    pthread_mutexattr_t mutex_attr;

    fd = shm_open("/SDMA_MUTEX",O_RDWR | O_CREAT | O_EXCL, 0666);

    if (fd >= 0) {
        memsize = (memsize + __PAGESIZE - 1) / __PAGESIZE * __PAGESIZE;
        if (shm_ctl (fd, SHMCTL_PHYS|SHMCTL_ANON, 0, memsize) != 0)
        {
            status = errno;
            perror("shared_mem_create() shm_ctl failed\n");
            goto fail2;
        }

        shmem_ptr = mmap(    0,
                             sizeof(sdma_shmem_t),
                             PROT_READ|PROT_WRITE,
                             MAP_SHARED,
                             fd,
                             0          );
        if (shmem_ptr == MAP_FAILED) {
            status = errno;
            perror("shared_mem_create() Couldn't mmap shared memory\n");
            goto fail2;
        }

        //get the physical memory address
        status = mem_offset64((void*)shmem_ptr,
                             NOFD,
                             sizeof(sdma_shmem_t),
                             &paddr64,
                             0 );
        if (status != 0) {
            perror("shared_mem_create() Couldn't get physical address of shared memory\n");
            goto fail3;
        }

        /* We currently have no way to allocate anonymous memory from typed memory (below4G)
         * via shm_ctl(). So we cannot support LPAE systems with this library as we need
         * our below4G physical memory to persist in our shared memory object after the
         * process terminates.
         */
        if ( (paddr64 > (INT64_MAX - sizeof(sdma_shmem_t))) ||
             (paddr64 + sizeof(sdma_shmem_t) >= ARM_LPAE_4G_BOUNDARY) )
        {
            status = ENOTSUP;
            perror("LPAE not supported");
            goto fail3;
        }
        paddr = (uint32_t) paddr64;

        //initial sdma shmem structure
        memset((void*)shmem_ptr, 0, sizeof(sdma_shmem_t));

        // Initialized multi-process mutexes
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_setpshared(&mutex_attr,PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&(shmem_ptr->libinit_mutex),&mutex_attr);
        pthread_mutex_init(&(shmem_ptr->command_mutex),&mutex_attr);
        pthread_mutex_init(&(shmem_ptr->register_mutex),&mutex_attr);

        //save the physical memory address of the CCB structure
        shmem_ptr->ccb_paddr = paddr + offsetof(sdma_shmem_t, ccb_arr);

        // Save the physical memmory address of the Command channel buffer descriptor
        shmem_ptr->ccb_arr[SDMA_CMD_CH].base_bd_paddr = paddr + offsetof(sdma_shmem_t, cmd_chn_bd);
        shmem_ptr->ccb_arr[SDMA_CMD_CH].current_bd_paddr = shmem_ptr->ccb_arr[SDMA_CMD_CH].base_bd_paddr;

        /* We will re-map the shmem_ptr via the shared memory object in the sdmasync_init() call which
         * is common between first process library load and subsequent process library loads.
         */
        munmap((void*)shmem_ptr, sizeof(sdma_shmem_t));
    } else {
        // Couldn't create shared memory because it either already exists,
        // or for some other reason... it doesn't matter at this point, because
        // sdmasync_init() will try and open the shared memory object if it exists.
        status = errno;
        goto fail1;
    }

    close(fd);
    return EOK;

fail3:
    munmap((void*)shmem_ptr, sizeof(sdma_shmem_t));
fail2:
    close(fd);
fail1:
    return status;
}


////////////////////////////////////////////////////////////////////////////////
//                            PUBLIC FUNCTIONS                                //
////////////////////////////////////////////////////////////////////////////////

void ctor(void) __attribute__((__constructor__));
void dtor(void) __attribute__((__destructor__));

void ctor(void) {
     rsrc_alloc_t    ralloc;

    if (shared_mem_create() == EOK) {
        // seed the resource db manager
        memset(&ralloc, 0, sizeof(ralloc));
        ralloc.start    = SDMA_CH_LO;
        ralloc.end      = SDMA_CH_HI;    
        ralloc.flags    = RSRCDBMGR_DMA_CHANNEL | RSRCDBMGR_FLAG_NOREMOVE; 
        if ( rsrcdbmgr_create(&ralloc, 1) != EOK ) {
            perror("SDMA ctor() Unable to seed dma channels\n");
        }
    } 
}

void dtor(void)  {     
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/lib/dma/sdma/init.c $ $Rev: 816922 $")
#endif
