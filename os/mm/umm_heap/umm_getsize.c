/****************************************************************************
 *
 * Copyright 2022 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/************************************************************************
 * Included Files
 ************************************************************************/

#include <tinyara/config.h>
#include <tinyara/mm/mm.h>

/************************************************************************
 * Public Functions
 ************************************************************************/

/****************************************************************************
 * Name: umm_get_largest_freenode_size
 *
 * Description:
 *   Returns the largest free node size of user heap.
 ****************************************************************************/

size_t umm_get_largest_freenode_size(void)
{
	return mm_get_largest_freenode_size();
}

/************************************************************************
 * Name: umm_get_heap_free_size
 *
 * Description:
 *   Returns the available free size of user heap.
 *
 ************************************************************************/
#ifdef CONFIG_DEBUG_MM_HEAPINFO
size_t umm_get_heap_free_size(void)
{
	return mm_get_heap_free_size();
}
#endif /* CONFIG_DEBUG_MM_HEAPINFO */
