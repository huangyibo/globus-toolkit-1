/******************************************************************************
globus_callback.c

Description:

  A general polling infrastructure

CVS Information:

  $Source$
  $Date$
  $Revision$
  $State$
  $Author$
******************************************************************************/

/******************************************************************************
			     Include header files
******************************************************************************/
#include "globus_memory.h"
#include "globus_libc.h"
#include "globus_list.h"

/*
 * data structures hidden from user
 */
typedef struct globus_l_memory_header_s
{
    globus_byte_t *                             next;
} globus_l_memory_header_t;

struct globus_memory_s
{
    int                                         total_size;
    int                                         node_size;
    int                                         nodes_used;
    int                                         node_count;
    int                                         node_count_per_malloc;

    globus_bool_t                               destroyed;
    globus_mutex_t                              lock;

    globus_byte_t *                             first;
    globus_byte_t **                            free_ptrs;
    int                                         free_ptrs_size;
    int                                         free_ptrs_offset;
};

globus_mutex_t                                  globus_i_memory_mutex;

#define I_ALIGN_SIZE                            sizeof(long)

globus_bool_t
globus_l_memory_create_list(
    globus_memory_t *                           mem_info);


globus_bool_t
globus_i_memory_pre_activate(void)
{
    globus_mutex_init(
        &globus_i_memory_mutex,
        GLOBUS_NULL);
    return globus_i_list_pre_activate();
}

globus_bool_t
globus_memory_init(
    globus_memory_t *                           mem_info,
    int                                         node_size,
    int                                         node_count)
{
    int                                         pad;
    struct globus_memory_s *                    s_mem_info;

    pad = node_size % I_ALIGN_SIZE;
    
    assert(mem_info != GLOBUS_NULL);
    s_mem_info = (struct globus_memory_s *)globus_malloc(sizeof(struct globus_memory_s));
    *mem_info = s_mem_info;

    s_mem_info->total_size = node_count * (node_size + pad);

    s_mem_info->node_size = node_size + pad;
    s_mem_info->node_count = node_count;
    s_mem_info->nodes_used = 0;
    s_mem_info->node_count_per_malloc = node_count;
    s_mem_info->free_ptrs_size = DEFAULT_FREE_PTRS_SIZE;
    s_mem_info->free_ptrs = (globus_byte_t **)malloc(DEFAULT_FREE_PTRS_SIZE * 
                                 sizeof(globus_byte_t *));
    s_mem_info->free_ptrs_offset = -1;

    globus_mutex_init(
        &s_mem_info->lock,
        (globus_mutexattr_t *) GLOBUS_NULL);
    s_mem_info->destroyed = GLOBUS_FALSE;

    return(globus_l_memory_create_list(
				mem_info)); 
}

/*
 * this is called locked
 */
globus_bool_t
globus_l_memory_create_list(
    globus_memory_t *                           mem_info)
{
    int                                         ctr;
    globus_l_memory_header_t *                  header;
    globus_byte_t *                             buf;
    int                                         tmp_size;
    struct globus_memory_s *                    s_mem_info;

    assert(mem_info != GLOBUS_NULL);
    s_mem_info = *mem_info;

    s_mem_info->first = globus_malloc(
                            s_mem_info->node_size * 
                            s_mem_info->node_count_per_malloc);

    s_mem_info->free_ptrs_offset++;
    if(s_mem_info->free_ptrs_offset == s_mem_info->free_ptrs_size)
    {
        tmp_size = s_mem_info->free_ptrs_size + DEFAULT_FREE_PTRS_SIZE;
        s_mem_info->free_ptrs = realloc(s_mem_info->free_ptrs, 
                  tmp_size * sizeof(globus_byte_t *));
        s_mem_info->free_ptrs_size = tmp_size;
    }

    s_mem_info->free_ptrs[s_mem_info->free_ptrs_offset] = s_mem_info->first;
    if(s_mem_info->first == GLOBUS_NULL)
    {
	    return GLOBUS_FALSE;
    }

    buf = s_mem_info->first;
    for(ctr = 0; ctr < s_mem_info->node_count_per_malloc - 1; ctr++)
    {
        header = (globus_l_memory_header_t *) buf;
	    buf += s_mem_info->node_size;
        header->next = buf;
    }
    header = (globus_l_memory_header_t *) buf;
    header->next = GLOBUS_NULL;

    return GLOBUS_TRUE;
}

globus_byte_t *
globus_memory_pop_node(
    globus_memory_t *                           mem_info)
{
    struct globus_memory_s *                    s_mem_info;
    globus_l_memory_header_t *                  header;
    globus_byte_t *                             tmp_byte;
   
    assert(mem_info != GLOBUS_NULL);
    s_mem_info = *mem_info;
    assert(s_mem_info != GLOBUS_NULL);
    
    globus_mutex_lock(&s_mem_info->lock);
    { 
        if(s_mem_info->destroyed)
        {
            globus_mutex_unlock(&s_mem_info->lock);
            return GLOBUS_FALSE;
        } 
        /* 
         *  test to see if there is memory left.
         */
        if(s_mem_info->first == GLOBUS_NULL)
        {
	        s_mem_info->node_count += s_mem_info->node_count_per_malloc;
            globus_l_memory_create_list(mem_info);
        }
    
        header = (globus_l_memory_header_t *) s_mem_info->first;
        tmp_byte = s_mem_info->first;
        s_mem_info->first = header->next;
        s_mem_info->nodes_used++;
    }
    globus_mutex_unlock(&s_mem_info->lock);

    return tmp_byte;
}

globus_bool_t
globus_memory_push_node(
    globus_memory_t *                           mem_info,
    globus_byte_t *                             buf)
{
    globus_l_memory_header_t *                  header;
    struct globus_memory_s *                    s_mem_info;

    assert(mem_info != GLOBUS_NULL);
    s_mem_info = *mem_info;
    assert(s_mem_info != GLOBUS_NULL);
    
    globus_mutex_lock(&s_mem_info->lock);
    { 
        if(s_mem_info->destroyed)
        {
            globus_mutex_unlock(&s_mem_info->lock);
            return GLOBUS_FALSE;
        } 
        header = (globus_l_memory_header_t *) (buf);

        header->next = s_mem_info->first;
        s_mem_info->first = (globus_byte_t *)header;
        s_mem_info->nodes_used--;
    } 
    globus_mutex_unlock(&s_mem_info->lock);

    return GLOBUS_TRUE;
}

globus_bool_t
globus_memory_destroy(
    globus_memory_t *                           mem_info)
{
/* TODO: fail if memory not freed correctly */
    int                                         ctr;
    struct globus_memory_s *                    s_mem_info;

    assert(mem_info != GLOBUS_NULL);
    s_mem_info = *mem_info;
    assert(s_mem_info != GLOBUS_NULL);

    globus_mutex_lock(&s_mem_info->lock);
    {
        if(s_mem_info->nodes_used > 0)
        {
        }
        for(ctr = 0; ctr <= s_mem_info->free_ptrs_offset; ctr++)
        {
            free(s_mem_info->free_ptrs[ctr]);
        }
    }
    globus_mutex_unlock(&s_mem_info->lock);

    globus_mutex_destroy(&s_mem_info->lock);
    globus_free(s_mem_info);
    mem_info = GLOBUS_NULL;
    
    return GLOBUS_TRUE;
}


