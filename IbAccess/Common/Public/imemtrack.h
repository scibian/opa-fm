/* BEGIN_ICS_COPYRIGHT3 ****************************************

Copyright (c) 2015, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

** END_ICS_COPYRIGHT3   ****************************************/

/* [ICS VERSION STRING: unknown] */

#ifndef _IBA_PUBLIC_IMEMTRACK_H_
#define _IBA_PUBLIC_IMEMTRACK_H_

#include "iba/public/ilist.h"
#include "iba/public/ispinlock.h"
#include "iba/public/imemory.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TRK_TAG		MAKE_MEM_TAG( m, t, r, k )


typedef struct MemoryTrackerFileName {
	struct MemoryTrackerFileName *next;
	int referenceCount;
	int filenameLen;
	char filename[1];
} MemoryTrackerFileName_t;

#define MEMORY_TRACKER_BUCKETS 53

/* Structure to track memory allocations. */
typedef struct _MEM_TRACKER
{
	/* List for tracking memory allocations. */
	QUICK_LIST	AllocList;
	/* Lock for synchronization. */
	SPIN_LOCK	Lock;
	/* List to manage free headers. */
	QUICK_LIST	FreeHrdList;

} MEM_TRACKER;

/* Footer for all memory allocations. */
typedef struct _MEM_ALLOC_FTR
{
	uint32				OutOfBound;
} MEM_ALLOC_FTR;

/* Header for all memory allocations. */
typedef struct _MEM_ALLOC_HDR
{
	LIST_ITEM			ListItem;
	MemoryTrackerFileName_t *trk;
	uint32				LineNum;
	uint32				Bytes;
	int32				reported;
	uint32				tick;
	volatile boolean displaying;
	volatile boolean deallocate;
	MEM_ALLOC_FTR		*pFtr;
} MEM_ALLOC_HDR;

#ifdef __cplusplus
};
#endif

#endif /* _IBA_PUBLIC_IMEMTRACK_H_ */
