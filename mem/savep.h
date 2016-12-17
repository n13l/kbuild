/*                                                                              
 * The MIT License (MIT)                       Memory Management debug facility
 *
 * Copyright (c) 2015 Daniel Kubec <niel@rtfm.cz> 
 *                                                                              
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"),to deal 
 * in the Software without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell    
 * copies of the Software, and to permit persons to whom the Software is        
 * furnished to do so, subject to the following conditions:                     
 *                                                                              
 * The above copyright notice and this permission notice shall be included in   
 * all copies or substantial portions of the Software.                          
 *                                                                              
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN    
 * THE SOFTWARE.
 */

#ifndef __MM_SAVEP_H__
#define __MM_SAVEP_H__

#include <sys/compiler.h>
#include <sys/cpu.h>
#include <sys/log.h>
#include <mem/debug.h>
#include <unix/list.h>

struct mm_savep {
	size_t avail[2];
	void  *final[2];
	struct snode node;
};

static inline void
mm_savep_dump(struct mm_savep *savep)
{
/*	
	mem_dbg("node %p next=%p avail=%u:%u final=%p:%p",
	        (void *)&savep->node, (void *)&savep->node.next, 
	        (unsigned int)savep->avail[0], (unsigned int)savep->avail[1],
	        savep->final[0], savep->final[1]);
*/		
}

#endif
