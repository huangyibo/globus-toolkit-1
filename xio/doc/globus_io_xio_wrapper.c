
/* XXX
 * need module/driver activate calls
 */
 
#undef MyName
#define MyName(func_name)                                                   \
    static char * myname = #func_name

#define GlobusLIOCheckHandle(handle, type)                                  \
    do                                                                      \
    {                                                                       \
        if(!(handle))                                                       \
        {                                                                   \
            return globus_error_put(                                        \
                globus_io_error_construct_null_parameter(                   \
                    GLOBUS_IO_MODULE,                                       \
                    GLOBUS_NULL,                                            \
                    #handle,                                                \
                    1,                                                      \
                    myname));                                               \
        }                                                                   \
                                                                            \
        if((*(handle))->type != type)                                       \
        {                                                                   \
            return globus_error_put(                                        \
                globus_io_error_construct_bad_pointer(                      \
                    GLOBUS_IO_MODULE,                                       \
                    GLOBUS_NULL,                                            \
                    #handle,                                                \
                    1,                                                      \
                    myname));                                               \
        }                                                                   \
    } while(0)

#define GlobusLIOCheckAttr(attr, types, need_gsi)                           \
    do                                                                      \
    {                                                                       \
        globus_result_t                 _result;                            \
                                                                            \
        _result = globus_l_io_attr_check(                                   \
            (attr),                                                         \
            (types),                                                        \
            (need_gsi),                                                     \
            myname);                                                        \
        if(_result != GLOBUS_SUCCESS)                                       \
        {                                                                   \
            return _result;                                                 \
        }                                                                   \
    } while(0)

#define GlobusLIOCheckNullParam(arg)                                        \
    if(!(arg))                                                              \
        return globus_error_put(                                            \
            globus_io_error_construct_null_parameter(                       \
                GLOBUS_IO_MODULE,                                           \
                GLOBUS_NULL,                                                \
                #arg,                                                       \
                1,                                                          \
                myname))

#define GlobusLIOMalloc(pointer, type)                                      \
    ((pointer = (type *) globus_malloc(sizeof(type)))                       \
        ? (GLOBUS_SUCCESS)                                                  \
        : (globus_error_put(                                                \
            globus_io_error_construct_system_failure(                       \
                GLOBUS_IO_MODULE,                                           \
                GLOBUS_NULL,                                                \
                GLOBUS_NULL,                                                \
                errno))))

typedef enum
{
    GLOBUS_I_IO_FILE_DRIVER_ATTR = 1,
    GLOBUS_I_IO_TCP_DRIVER_ATTR  = 2,
    GLOBUS_I_IO_UDP_DRIVER_ATTR  = 4
} globus_i_io_driver_attr_type_t;

typedef enum
{
    GLOBUS_I_IO_FILE_HANDLE = 1,
    GLOBUS_I_IO_TCP_HANDLE  = 2,
    GLOBUS_I_IO_UDP_HANDLE  = 4
} globus_i_io_handle_type_t;

typedef struct globus_i_io_attr_s
{
    globus_xio_handle_attr_t            xio_attr;

    globus_i_io_driver_attr_type_t      type;
    globus_xio_driver_handle_attr_t     driver_attr;

    globus_bool_t                       gsi_attr_used;
    globus_xio_driver_handle_attr_t     gsi_attr;
} globus_i_io_attr_t;

typedef struct
{
    globus_io_handle_t *                      io_handle;
    globus_io_secure_authorization_callback_t callback;
    void *                                    callback_arg;
} globus_l_gsi_authorization_callback_info_t;

typedef struct globus_i_io_handle_s
{
    globus_i_io_handle_type_t           type;
    globus_io_handle_t *                io_handle;
    globus_xio_handle_t                 xio_handle;
    globus_l_gsi_authorization_callback_info_t * gsi_auth_callback_info;
} globus_i_io_handle_t;

static
globus_result_t
globus_l_io_attr_check(
    globus_io_attr_t *                  attr,
    int                                 types,
    globus_bool_t                       need_gsi,
    const char *                        func_name)
{
    globus_i_io_attr_t *                iattr;
    
    if(!attr)
    {
        return globus_error_put(
            globus_io_error_construct_null_parameter(
                GLOBUS_IO_MODULE,
                GLOBUS_NULL,
                "attr",
                1,
                func_name));
    }
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    if(!iattr || !(iattr->type & types))
    {
        return globus_error_put(
            globus_io_error_construct_bad_pointer(
                GLOBUS_IO_MODULE,
                GLOBUS_NULL,
                "attr",
                1,
                func_name));
    }
    
    if(need_gsi && !iattr->gsi_attr_used && 
        (types & GLOBUS_I_IO_TCP_DRIVER_ATTR))
    {
        globus_result_t                 result;
        
        result = globus_xio_gsi_attr_init(&iattr->gsi_attr);
        if(result == GLOBUS_SUCCESS)
        {
            iattr->gsi_attr_used = GLOBUS_TRUE;
        }
        else
        {
            return result;
        }
    }
    
    return GLOBUS_SUCCESS;
}

static
globus_result_t
globus_l_io_iattr_copy(
    globus_io_attr_t *                  dest,
    globus_io_attr_t *                  source)
{
    globus_i_io_attr_t *                source_iattr;
    globus_i_io_attr_t *                dest_iattr;
    globus_result_t                     result;
    MyName(globus_io_fileattr_init);
    
    source_iattr = (globus_i_io_attr_t *) *source;
    
    result = GlobusLIOMalloc(dest_iattr, globus_i_io_attr_t);
    if(result != GLOBUS_SUCCESS)
    {
        return result;
    }
    
    dest_iattr->type = source_iattr->type;
    dest_iattr->gsi_attr_used = source_iattr->gsi_attr_used;
    result = globus_xio_handle_attr_copy(
        &dest_iattr->xio_attr, source_iattr->xio_attr);
    if(result != GLOBUS_SUCCESS)
    {
        goto free_dest;
    }
    
    if(source_iattr->gsi_attr_used)
    {
        result = globus_xio_gsi_attr_copy(
            &dest_iattr->gsi_attr, source_iattr->gsi_attr);
        if(result != GLOBUS_SUCCESS)
        {
            goto destroy_dest_xio_attr;
        }
    }
    
    switch(source_iattr->type)
    {
      case GLOBUS_I_IO_FILE_DRIVER_ATTR:
        result = globus_xio_file_attr_copy(
            &dest_iattr->driver_attr, source_iattr->driver_attr);
        break;
        
      case GLOBUS_I_IO_TCP_DRIVER_ATTR:
        result = globus_xio_tcp_attr_copy(
            &dest_iattr->driver_attr, source_iattr->driver_attr);
        break;
        
      case GLOBUS_I_IO_UDP_DRIVER_ATTR:
        result = globus_xio_udp_attr_copy(
            &dest_iattr->driver_attr, source_iattr->driver_attr);
        break;
      
      default:
        globus_assert(0 && "invalid type");
        break;
    }
    
    if(result != GLOBUS_SUCCESS)
    {
        goto destroy_dest_gsi_attr;
    }
    
    *dest = dest_iattr;
    
    return GLOBUS_SUCCESS;

destroy_dest_gsi_attr:
    if(source_iattr->gsi_attr_used)
    {
        globus_xio_gsi_attr_destroy(dest_iattr->gsi_attr);
    }
    
destroy_dest_xio_attr:
    globus_xio_handle_attr_destroy(dest_iattr->xio_attr);
    
free_dest:
    globus_free(dest_iattr);
    
    return result;
}

/* file attrs */

globus_result_t
globus_io_fileattr_init(
    globus_io_attr_t *                  attr)
{
    globus_i_io_attr_t *                iattr;
    globus_result_t                     result;
    MyName(globus_io_fileattr_init);
    
    GlobusLIOCheckNullParam(attr);
    
    result = GlobusLIOMalloc(iattr, globus_i_io_attr_t);
    if(result != GLOBUS_SUCCESS)
    {
        return result;
    }
    
    iattr->type = GLOBUS_I_IO_FILE_DRIVER_ATTR;
    iattr->gsi_attr_used = GLOBUS_FALSE;
    
    result = globus_xio_file_attr_init(&iattr->driver_attr);
    if(result != GLOBUS_SUCCESS)
    {
        goto exit_free;
    }
    
    result = globus_xio_handle_attr_init(&iattr->xio_attr);
    if(result != GLOBUS_SUCCESS)
    {
        goto exit_destroy;
    }
    
    *attr = iattr;
    
    return GLOBUS_SUCCESS;
    
exit_destroy:
    
    globus_xio_file_attr_destroy(iattr->driver_attr);
    
exit_free:

    globus_free(iattr);
    
    return result;
}

globus_result_t
globus_io_fileattr_destroy(
    globus_io_attr_t *                  attr)
{
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_fileattr_destroy);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_FILE_DRIVER_ATTR, GLOBUS_FALSE);
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    globus_xio_file_attr_destroy(iattr->driver_attr);
    globus_xio_handle_attr_destroy(iattr->xio_attr);
    
    globus_free(iattr);
    
    return GLOBUS_SUCCESS;
}

globus_result_t
globus_io_attr_set_file_type(
    globus_io_attr_t *                  attr,
    globus_io_file_type_t               file_type)
{
    MyName(globus_io_attr_set_file_type);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_FILE_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_file_attr_set_type((*attr)->driver_attr, file_type);
}

globus_result_t
globus_io_attr_get_file_type(
    globus_io_attr_t *                  attr,
    globus_io_file_type_t *             file_type)
{
    MyName(globus_io_attr_get_file_type);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_FILE_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_file_attr_get_type((*attr)->driver_attr, file_type);
}

/* udp attrs */

globus_result_t
globus_io_udpattr_init(
    globus_io_attr_t *                  attr)
{
    globus_i_io_attr_t *                iattr;
    globus_result_t                     result;
    MyName(globus_io_udpattr_init);
    
    GlobusLIOCheckNullParam(attr);
    
    result = GlobusLIOMalloc(iattr, globus_i_io_attr_t);
    if(result != GLOBUS_SUCCESS)
    {
        return result;
    }
    
    iattr->type = GLOBUS_I_IO_UDP_DRIVER_ATTR;
    iattr->gsi_attr_used = GLOBUS_FALSE;
    
    result = globus_xio_udp_attr_init(&iattr->driver_attr);
    if(result != GLOBUS_SUCCESS)
    {
        goto exit_free;
    }
    
    result = globus_xio_handle_attr_init(&iattr->xio_attr);
    if(result != GLOBUS_SUCCESS)
    {
        goto exit_destroy;
    }
    
    *attr = iattr;
    
    return GLOBUS_SUCCESS;
    
exit_destroy:
    
    globus_xio_udp_attr_destroy(iattr->driver_attr);
    
exit_free:

    globus_free(iattr);
    
    return result;
}

globus_result_t
globus_io_udpattr_destroy(
    globus_io_attr_t *                  attr)
{
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_udpattr_destroy);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    globus_xio_udp_attr_destroy(iattr->driver_attr);
    globus_xio_handle_attr_destroy(iattr->xio_attr);
    
    globus_free(iattr);
    
    return GLOBUS_SUCCESS;
}

globus_result_t
globus_io_attr_set_udp_restrict_port(
    globus_io_attr_t *                  attr,
    globus_bool_t                       restrict_port)
{
    MyName(globus_io_attr_set_udp_restrict_port);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_udp_attr_set_restrict_port(
        (*attr)->driver_attr, restrict_port);
}

globus_result_t
globus_io_attr_get_udp_restrict_port(
    globus_io_attr_t *                  attr,
    globus_bool_t *                     restrict_port)
{
    MyName(globus_io_attr_get_udp_restrict_port);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_udp_attr_get_restrict_port(
        (*attr)->driver_attr, restrict_port);
}

globus_result_t
globus_io_attr_set_udp_multicast_loop(
    globus_io_attr_t *                  attr,
    globus_bool_t                       enable_loopback)
{
    MyName(globus_io_attr_set_udp_multicast_loop);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_udp_attr_set_multicast_loop(
        (*attr)->driver_attr, enable_loopback);
}

globus_result_t
globus_io_attr_get_udp_multicast_loop(
    globus_io_attr_t *                  attr,
    globus_bool_t *                     enable_loopback)
{
    MyName(globus_io_attr_get_udp_multicast_loop);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_udp_attr_get_multicast_loop(
        (*attr)->driver_attr, enable_loopback);
}

globus_result_t
globus_io_attr_set_udp_multicast_membership(
    globus_io_attr_t *                  attr,
    char *                              address,
    char *                              interface_addr)
{
    MyName(globus_io_attr_set_udp_multicast_membership);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_udp_attr_set_multicast_membership(
        (*attr)->driver_attr, address, interface_addr);
}

globus_result_t
globus_io_attr_get_udp_multicast_membership(
    globus_io_attr_t *                  attr,
    char **                             address,
    char **                             interface_addr)
{
    MyName(globus_io_attr_get_udp_multicast_membership);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_udp_attr_get_multicast_membership(
        (*attr)->driver_attr, address, interface_addr);
}

globus_result_t
globus_io_attr_set_udp_multicast_ttl(
    globus_io_attr_t *                  attr,
    globus_byte_t                       ttl)
{
    MyName(globus_io_attr_set_udp_multicast_ttl);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_udp_attr_set_multicast_ttl((*attr)->driver_attr, ttl);
}

globus_result_t
globus_io_attr_get_udp_multicast_ttl(
    globus_io_attr_t *                  attr,
    globus_byte_t *                     ttl)
{
    MyName(globus_io_attr_get_udp_multicast_ttl);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_udp_attr_get_multicast_ttl((*attr)->driver_attr, ttl);
}

globus_result_t
globus_io_attr_set_udp_multicast_interface(
    globus_io_attr_t *                  attr,
    char *                              interface_addr)
{
    MyName(globus_io_attr_set_udp_multicast_interface);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_udp_attr_set_multicast_interface(
        (*attr)->driver_attr, interface_addr);
}

globus_result_t
globus_io_attr_get_udp_multicast_interface(
    globus_io_attr_t *                  attr,
    char **                             interface_addr)
{
    MyName(globus_io_attr_get_udp_multicast_interface);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_udp_attr_get_multicast_interface(
        (*attr)->driver_attr, interface_addr);
}

/* tcp attrs */
globus_result_t
globus_io_tcpattr_init(
    globus_io_attr_t *                  attr)
{
    globus_i_io_attr_t *                iattr;
    globus_result_t                     result;
    MyName(globus_io_tcpattr_init);
    
    GlobusLIOCheckNullParam(attr);
    
    result = GlobusLIOMalloc(iattr, globus_i_io_attr_t);
    if(result != GLOBUS_SUCCESS)
    {
        return result;
    }
    
    iattr->type = GLOBUS_I_IO_TCP_DRIVER_ATTR;
    iattr->gsi_attr_used = GLOBUS_FALSE;
    
    result = globus_xio_tcp_attr_init(&iattr->driver_attr);
    if(result != GLOBUS_SUCCESS)
    {
        goto exit_free;
    }
    
    result = globus_xio_handle_attr_init(&iattr->xio_attr);
    if(result != GLOBUS_SUCCESS)
    {
        goto exit_destroy;
    }
    
    *attr = iattr;
    
    return GLOBUS_SUCCESS;
    
exit_destroy:
    
    globus_xio_tcp_attr_destroy(iattr->driver_attr);
    
exit_free:

    globus_free(iattr);
    
    return result;
}

globus_result_t
globus_io_tcpattr_destroy(
    globus_io_attr_t *                  attr)
{
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_tcpattr_destroy);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_FALSE);
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    if(iattr->gsi_attr_used)
    {
        globus_xio_gsi_attr_destroy(iattr->gsi_attr);
    }
    
    globus_xio_tcp_attr_destroy(iattr->driver_attr);
    globus_xio_handle_attr_destroy(iattr->xio_attr);
    
    globus_free(iattr);
    
    return GLOBUS_SUCCESS;
}

globus_result_t
globus_io_attr_set_tcp_restrict_port(
    globus_io_attr_t *                  attr,
    globus_bool_t                       restrict_port)
{
    MyName(globus_io_attr_set_tcp_restrict_port);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_tcp_attr_set_restrict_port(
        (*attr)->driver_attr, restrict_port);
}

globus_result_t
globus_io_attr_get_tcp_restrict_port(
    globus_io_attr_t *                  attr,
    globus_bool_t *                     restrict_port)
{
    MyName(globus_io_attr_get_tcp_restrict_port);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_tcp_attr_get_restrict_port(
        (*attr)->driver_attr, restrict_port);
}

globus_result_t
globus_io_attr_set_tcp_nodelay(
    globus_io_attr_t *                  attr,
    globus_bool_t                       nodelay)
{
    MyName(globus_io_attr_set_tcp_nodelay);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_tcp_attr_set_nodelay((*attr)->driver_attr, nodelay);
}

globus_result_t
globus_io_attr_get_tcp_nodelay(
    globus_io_attr_t *                  attr,
    globus_bool_t *                     nodelay)
{
    MyName(globus_io_attr_get_tcp_nodelay);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_tcp_attr_get_nodelay((*attr)->driver_attr, nodelay);
}

globus_result_t
globus_io_attr_set_tcp_interface(
    globus_io_attr_t *                  attr,
    const char *                        interface_addr)
{
    MyName(globus_io_attr_set_tcp_interface);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_tcp_attr_set_interface(
        (*attr)->driver_attr, interface_addr);
}

globus_result_t
globus_io_attr_get_tcp_interface(
    globus_io_attr_t *                  attr,
    char **                             interface_addr)
{
    MyName(globus_io_attr_get_tcp_interface);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_tcp_attr_get_interface(
        (*attr)->driver_attr, interface_addr);
}

/* socket attrs */
globus_result_t
globus_io_attr_set_socket_reuseaddr(
    globus_io_attr_t *                  attr,
    globus_bool_t                       reuseaddr)
{
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_attr_set_socket_reuseaddr);
    
    GlobusLIOCheckAttr(
        attr, 
        GLOBUS_I_IO_TCP_DRIVER_ATTR | GLOBUS_I_IO_UDP_DRIVER_ATTR, 
        GLOBUS_FALSE);
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    if(iattr->type == GLOBUS_I_IO_TCP_DRIVER_ATTR)
    {
        return globus_xio_tcp_attr_set_reuseaddr(
            iattr->driver_attr, reuseaddr);
    }
    else
    {
        return globus_xio_udp_attr_set_reuseaddr(
            iattr->driver_attr, reuseaddr);
    }
}

globus_result_t
globus_io_attr_get_socket_reuseaddr(
    globus_io_attr_t *                  attr,
    globus_bool_t *                     reuseaddr)
{
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_attr_get_socket_reuseaddr);
    
    GlobusLIOCheckAttr(
        attr, 
        GLOBUS_I_IO_TCP_DRIVER_ATTR | GLOBUS_I_IO_UDP_DRIVER_ATTR, 
        GLOBUS_FALSE);
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    if(iattr->type == GLOBUS_I_IO_TCP_DRIVER_ATTR)
    {
        return globus_xio_tcp_attr_get_reuseaddr(
            iattr->driver_attr, reuseaddr);
    }
    else
    {
        return globus_xio_udp_attr_get_reuseaddr(
            iattr->driver_attr, reuseaddr);
    }
}

globus_result_t
globus_io_attr_set_socket_keepalive(
    globus_io_attr_t *                  attr,
    globus_bool_t                       keepalive)
{
    MyName(globus_io_attr_set_socket_keepalive);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_tcp_attr_set_keepalive((*attr)->driver_attr, keepalive);
}

globus_result_t
globus_io_attr_get_socket_keepalive(
    globus_io_attr_t *                  attr,
    globus_bool_t *                     keepalive)
{
    MyName(globus_io_attr_get_socket_keepalive);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_tcp_attr_get_keepalive((*attr)->driver_attr, keepalive);
}

globus_result_t
globus_io_attr_set_socket_linger(
    globus_io_attr_t *                  attr,
    globus_bool_t                       linger,
    int                                 linger_time)
{
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_attr_set_socket_linger);
    
    GlobusLIOCheckAttr(
        attr, 
        GLOBUS_I_IO_TCP_DRIVER_ATTR | GLOBUS_I_IO_UDP_DRIVER_ATTR, 
        GLOBUS_FALSE);
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    if(iattr->type == GLOBUS_I_IO_TCP_DRIVER_ATTR)
    {
        return globus_xio_tcp_attr_set_linger(
            iattr->driver_attr, linger, linger_time);
    }
    else
    {
        return globus_xio_udp_attr_set_linger(
            iattr->driver_attr, linger, linger_time);
    }
}

globus_result_t
globus_io_attr_get_socket_linger(
    globus_io_attr_t *                  attr,
    globus_bool_t *                     linger,
    int *                               linger_time)
{
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_attr_get_socket_linger);
    
    GlobusLIOCheckAttr(
        attr, 
        GLOBUS_I_IO_TCP_DRIVER_ATTR | GLOBUS_I_IO_UDP_DRIVER_ATTR, 
        GLOBUS_FALSE);
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    if(iattr->type == GLOBUS_I_IO_TCP_DRIVER_ATTR)
    {
        return globus_xio_tcp_attr_get_linger(
            iattr->driver_attr, linger, linger_time);
    }
    else
    {
        return globus_xio_udp_attr_get_linger(
            iattr->driver_attr, linger, linger_time);
    }
}

globus_result_t
globus_io_attr_set_socket_oobinline(
    globus_io_attr_t *                  attr,
    globus_bool_t                       oobinline)
{
    MyName(globus_io_attr_set_socket_oobinline);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_tcp_attr_set_oobinline((*attr)->driver_attr, oobinline);
}

globus_result_t
globus_io_attr_get_socket_oobinline(
    globus_io_attr_t *                  attr,
    globus_bool_t *                     oobinline)
{
    MyName(globus_io_attr_get_socket_oobinline);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_FALSE);
    
    return globus_xio_tcp_attr_get_oobinline((*attr)->driver_attr, oobinline);
}

globus_result_t
globus_io_attr_set_socket_sndbuf(
    globus_io_attr_t *                  attr,
    int                                 sndbuf)
{
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_attr_set_socket_sndbuf);
    
    GlobusLIOCheckAttr(
        attr, 
        GLOBUS_I_IO_TCP_DRIVER_ATTR | GLOBUS_I_IO_UDP_DRIVER_ATTR, 
        GLOBUS_FALSE);
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    if(iattr->type == GLOBUS_I_IO_TCP_DRIVER_ATTR)
    {
        return globus_xio_tcp_attr_set_sndbuf(iattr->driver_attr, sndbuf);
    }
    else
    {
        return globus_xio_udp_attr_set_sndbuf(iattr->driver_attr, sndbuf);
    }
}

globus_result_t
globus_io_attr_get_socket_sndbuf(
    globus_io_attr_t *                  attr,
    int *                               sndbuf)
{
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_attr_get_socket_sndbuf);
    
    GlobusLIOCheckAttr(
        attr, 
        GLOBUS_I_IO_TCP_DRIVER_ATTR | GLOBUS_I_IO_UDP_DRIVER_ATTR, 
        GLOBUS_FALSE);
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    if(iattr->type == GLOBUS_I_IO_TCP_DRIVER_ATTR)
    {
        return globus_xio_tcp_attr_get_sndbuf(iattr->driver_attr, sndbuf);
    }
    else
    {
        return globus_xio_udp_attr_get_sndbuf(iattr->driver_attr, sndbuf);
    }
}

globus_result_t
globus_io_attr_set_socket_rcvbuf(
    globus_io_attr_t *                  attr,
    int                                 rcvbuf)
{
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_attr_set_socket_rcvbuf);
    
    GlobusLIOCheckAttr(
        attr, 
        GLOBUS_I_IO_TCP_DRIVER_ATTR | GLOBUS_I_IO_UDP_DRIVER_ATTR, 
        GLOBUS_FALSE);
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    if(iattr->type == GLOBUS_I_IO_TCP_DRIVER_ATTR)
    {
        return globus_xio_tcp_attr_set_rcvbuf(iattr->driver_attr, rcvbuf);
    }
    else
    {
        return globus_xio_udp_attr_set_rcvbuf(iattr->driver_attr, rcvbuf);
    }
}

globus_result_t
globus_io_attr_get_socket_rcvbuf(
    globus_io_attr_t *                  attr,
    int *                               rcvbuf)
{
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_attr_get_socket_rcvbuf);
    
    GlobusLIOCheckAttr(
        attr, 
        GLOBUS_I_IO_TCP_DRIVER_ATTR | GLOBUS_I_IO_UDP_DRIVER_ATTR, 
        GLOBUS_FALSE);
    
    iattr = (globus_i_io_attr_t *) *attr;
    
    if(iattr->type == GLOBUS_I_IO_TCP_DRIVER_ATTR)
    {
        return globus_xio_tcp_attr_get_rcvbuf(iattr->driver_attr, rcvbuf);
    }
    else
    {
        return globus_xio_udp_attr_get_rcvbuf(iattr->driver_attr, rcvbuf);
    }
}

/* secure socket attrs */
globus_result_t
globus_io_attr_set_secure_authentication_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_authentication_mode_t
                                        mode,
    gss_cred_id_t                       credential)
{
    MyName(globus_io_attr_set_secure_authentication_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_set_authentication_mode(
        (*attr)->gsi_attr, mode, credential);
}

globus_result_t
globus_io_attr_get_secure_authentication_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_authentication_mode_t *
                                        mode,
    gss_cred_id_t *                     credential)
{
    MyName(globus_io_attr_get_secure_authentication_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_get_authentication_mode(
        (*attr)->gsi_attr, mode, credential);
}

globus_result_t
globus_io_attr_set_secure_authorization_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_authorization_mode_t
                                        mode,
    globus_io_secure_authorization_data_t *
                                        data)
{
    MyName(globus_io_attr_set_secure_authorization_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_set_authorization_mode(
        (*attr)->gsi_attr, mode, *data);
}

globus_result_t
globus_io_attr_get_secure_authorization_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_authorization_mode_t *
                                        mode,
    globus_io_secure_authorization_data_t *
                                        data)
{
    MyName(globus_io_attr_get_secure_authorization_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_get_authorization_mode(
        (*attr)->gsi_attr, mode, data);
}

globus_result_t
globus_io_attr_set_secure_extension_oids(
    globus_io_attr_t *                  attr,
    gss_OID_set                         extension_oids)
{
    MyName(globus_io_attr_set_secure_extension_oids);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_set_extension_oids(
        (*attr)->gsi_attr, extension_oids);
}

globus_result_t
globus_io_attr_get_secure_extension_oids(
    globus_io_attr_t *                  attr,
    gss_OID_set *                       extension_oids)
{
    MyName(globus_io_attr_get_secure_extension_oids);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_get_extension_oids(
        (*attr)->gsi_attr, extension_oids);
}

globus_result_t
globus_io_attr_set_secure_channel_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_channel_mode_t     mode)
{
    MyName(globus_io_attr_set_secure_channel_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_set_channel_mode((*attr)->gsi_attr, mode);
}

globus_result_t
globus_io_attr_get_secure_channel_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_channel_mode_t *   mode)
{
    MyName(globus_io_attr_get_secure_channel_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_get_channel_mode((*attr)->gsi_attr, mode);
}

globus_result_t
globus_io_attr_set_secure_protection_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_protection_mode_t  mode)
{
    MyName(globus_io_attr_set_secure_protection_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_set_protection_mode((*attr)->gsi_attr, mode);
}

globus_result_t
globus_io_attr_get_secure_protection_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_protection_mode_t *mode)
{
    MyName(globus_io_attr_get_secure_protection_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_get_protection_mode((*attr)->gsi_attr, mode);
}

globus_result_t
globus_io_attr_set_secure_delegation_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_delegation_mode_t  mode)
{
    MyName(globus_io_attr_set_secure_delegation_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_set_delegation_mode((*attr)->gsi_attr, mode);
}

globus_result_t
globus_io_attr_get_secure_delegation_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_delegation_mode_t *
                                        mode)
{
    MyName(globus_io_attr_get_secure_delegation_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_get_delegation_mode((*attr)->gsi_attr, mode);
}
globus_result_t
globus_io_attr_set_secure_proxy_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_proxy_mode_t       mode)
{
    MyName(globus_io_attr_set_secure_proxy_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_set_proxy_mode((*attr)->gsi_attr, mode);
}

globus_result_t
globus_io_attr_get_secure_proxy_mode(
    globus_io_attr_t *                  attr,
    globus_io_secure_proxy_mode_t *     mode)
{
    MyName(globus_io_attr_get_secure_proxy_mode);
    
    GlobusLIOCheckAttr(attr, GLOBUS_I_IO_TCP_DRIVER_ATTR, GLOBUS_TRUE);
    
    return globus_xio_gsi_attr_get_proxy_mode((*attr)->gsi_attr, mode);
}

/* callback space attrs */
globus_result_t
globus_io_attr_set_callback_space(
    globus_io_attr_t *                  attr,
    globus_callback_space_t             space)
{
    MyName(globus_io_attr_set_callback_space);
    
    GlobusLIOCheckAttr(
        attr,
        GLOBUS_I_IO_TCP_DRIVER_ATTR | 
            GLOBUS_I_IO_UDP_DRIVER_ATTR | 
            GLOBUS_I_IO_FILE_DRIVER_ATTR,
        GLOBUS_FALSE);
    
    return globus_xio_handle_attr_set_callback_space((*attr)->xio_attr, space);
}

globus_result_t
globus_io_attr_get_callback_space(
    globus_io_attr_t *                  attr,
    globus_callback_space_t *           space)
{
    MyName(globus_io_attr_get_callback_space);
    
    GlobusLIOCheckAttr(
        attr,
        GLOBUS_I_IO_TCP_DRIVER_ATTR | 
            GLOBUS_I_IO_UDP_DRIVER_ATTR | 
            GLOBUS_I_IO_FILE_DRIVER_ATTR,
        GLOBUS_FALSE);

    return globus_xio_handle_attr_get_callback_space((*attr)->xio_attr, space);
}

/* secure data handling */

globus_result_t
globus_io_secure_authorization_data_set_callback(
    globus_io_secure_authorization_data_t *
                                        data,
    globus_io_secure_authorization_callback_t
                                        callback,
    void *                              callback_arg)
{
    MyName(globus_io_secure_authorization_data_set_callback);
    
    GlobusLIOCheckNullParam(data);
    
    /* will wrap users callback with proper type when they bind to handle */
    return globus_xio_gsi_authorization_data_set_callback(
        *data,
        (globus_xio_gsi_authorization_callback_t) callback,
        callback_arg);
}

globus_result_t
globus_io_secure_authorization_data_get_callback(
    globus_io_secure_authorization_data_t *
                                        data,
    globus_io_secure_authorization_callback_t *
                                        callback,
    void **                             callback_arg)
{
    globus_result_t                     result;
    void *                              callback_func;
    globus_l_gsi_authorization_callback_info_t * callback_info;
    MyName(globus_io_secure_authorization_data_set_callback);
    
    GlobusLIOCheckNullParam(data);
    GlobusLIOCheckNullParam(callback);
    GlobusLIOCheckNullParam(callback_arg);
    
    result = globus_xio_gsi_authorization_data_get_callback(
        *data,
        (globus_xio_gsi_authorization_callback_t *) &callback_func,
        (void **) &callback_info);
    if(result == GLOBUS_SUCCESS)
    {
        if(callback_func == (void *) globus_l_io_gsi_authorization_callback)
        {
            /* user arg is my wrapper, get its values */
            *callback = callback_info->callback;
            *callback_arg = callback_info->callback_arg;
        }
        else
        {
            *callback = callback_func;
            *callback_arg = callback_info;
        }
    }
    
    return result;
}

/* file operations */

static
globus_result_t
globus_l_io_file_open(
    globus_io_handle_t *                handle,
    globus_i_io_attr_t *                iattr,
    globus_xio_factory_attr_t           factory_attr)
{
    globus_xio_handle_t                 xio_handle;
    globus_i_io_handle_t *              ihandle;
    globus_xio_factory_t                factory;
    globus_result_t                     result;
    MyName(globus_l_io_file_open);
    
    result = globus_xio_handle_attr_add_driver_attr(
        iattr->xio_attr, iattr->driver_attr);
    if(result == GLOBUS_SUCCESS)
    {
        result = globus_xio_factory_push_driver(
            factory_attr, GLOBUS_XIO_FILE_DRIVER, GLOBUS_NULL);
        if(result == GLOBUS_SUCCESS)
        {
             result = globus_xio_factory_init(&factory, factory_attr);
        }
    }
    
    if(result != GLOBUS_SUCCESS)
    {
        return result;
    }
    
    result = GlobusLIOMalloc(ihandle, globus_i_io_handle_t);
    if(result != GLOBUS_SUCCESS)
    {
        goto destroy_factory;
    }
    
    result = globus_xio_open(factory, &xio_handle, iattr->xio_attr);
    if(result != GLOBUS_SUCCESS)
    {
        globus_free(ihandle);
        goto destroy_factory;
    }
    
    ihandle->io_handle = handle;
    ihandle->xio_handle = xio_handle;
    ihandle->type = GLOBUS_I_IO_FILE_HANDLE;
    *handle = ihandle;

destroy_factory:
    globus_xio_factory_destroy(factory);
    
    return result;
}

globus_result_t
globus_io_file_open(
    char *                              path,
    int                                 flags,
    int                                 mode,
    globus_io_attr_t *                  attr,
    globus_io_handle_t *                handle)
{
    globus_xio_factory_attr_t           factory_attr;
    globus_result_t                     result;
    globus_io_attr_t                    myattr;
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_file_open);
    
    GlobusLIOCheckNullParam(handle);
    GlobusLIOCheckNullParam(path);
    
    if(attr)
    {
        GlobusLIOCheckAttr(attr, GLOBUS_I_IO_FILE_DRIVER_ATTR, GLOBUS_FALSE);
        result = globus_l_io_iattr_copy(&myattr, attr);
    }
    else
    {
        result = globus_io_fileattr_init(&myattr);
    }
    
    if(result != GLOBUS_SUCCESS)
    {
        return result;
    }
    
    iattr = (globus_i_io_attr_t *) myattr;

    result = globus_xio_file_attr_set_path(iattr->driver_attr, path);
    if(result == GLOBUS_SUCCESS)
    {
        result = globus_xio_file_attr_set_mode(iattr->driver_attr, mode);
        if(result == GLOBUS_SUCCESS)
        {
            result = globus_xio_file_attr_set_flags(iattr->driver_attr, flags);
            if(result == GLOBUS_SUCCESS)
            {
                result = globus_xio_factory_attr_init(&factory_attr);
            }
        }
    }
    
    if(result != GLOBUS_SUCCESS)
    {
        goto destroy_myattr;
    }
    
    result = globus_l_io_file_open(handle, iattr, factory_attr);

destroy_factory_attr:
    globus_xio_factory_attr_destroy(factory_attr);
    
destroy_myattr:
    globus_io_fileattr_destroy(&myattr);
    
    return result;
}

globus_result_t
globus_io_file_seek(
    globus_io_handle_t *                handle,
    globus_off_t                        offset,
    globus_io_whence_t                  whence)
{
    globus_i_io_handle_t *              ihandle;
    globus_xio_driver_handle_attr_t     attr;
    globus_result_t                     result;
    MyName(globus_io_file_seek);
    
    GlobusLIOCheckHandle(handle, GLOBUS_I_IO_FILE_HANDLE);
    
    ihandle = (globus_i_io_handle_t *) handle;
    
    return  globus_xio_handle_driver_fcntl(
        ihandle->xio_handle,
        GLOBUS_XIO_FILE_DRIVER,
        GLOBUS_XIO_FILE_SET_SEEK,
        offset,
        whence);
}

globus_result_t
globus_io_file_posix_convert(
    int                                 fd,
    globus_io_attr_t *                  attr,
    globus_io_handle_t *                handle)
{
    globus_xio_factory_attr_t           factory_attr;
    globus_result_t                     result;
    globus_io_attr_t                    myattr;
    globus_i_io_attr_t *                iattr;
    MyName(globus_io_file_posix_convert);
    
    GlobusLIOCheckNullParam(handle);
    GlobusLIOCheckNullParam(path);
    
    if(attr)
    {
        GlobusLIOCheckAttr(attr, GLOBUS_I_IO_FILE_DRIVER_ATTR, GLOBUS_FALSE);
        result = globus_l_io_iattr_copy(&myattr, attr);
    }
    else
    {
        result = globus_io_fileattr_init(&myattr);
    }
    
    if(result != GLOBUS_SUCCESS)
    {
        return result;
    }
    
    iattr = (globus_i_io_attr_t *) myattr;

    result = globus_xio_file_attr_set_fd(iattr->driver_attr, fd);
    if(result == GLOBUS_SUCCESS)
    {
        result = globus_xio_factory_attr_init(&factory_attr);
    }
    
    if(result != GLOBUS_SUCCESS)
    {
        goto destroy_myattr;
    }
    
    result = globus_l_io_file_open(handle, iattr, factory_attr);

    globus_xio_factory_attr_destroy(factory_attr);
    
destroy_myattr:
    globus_io_fileattr_destroy(&myattr);
    
    return result;
}

/* udp operations */

/* XXX net to set port back */
globus_result_t
globus_io_udp_bind(
    unsigned short *                    port,
    globus_io_attr_t *                  attr,
    globus_io_handle_t *                handle)
{
    globus_xio_factory_attr_t           factory_attr;
    globus_xio_handle_t                 xio_handle;
    globus_io_attr_t                    myattr;
    globus_i_io_attr_t *                iattr;
    globus_i_io_handle_t *              ihandle;
    globus_xio_factory_t                factory;
    globus_result_t                     result;
    MyName(globus_io_udp_bind);
    
    GlobusLIOCheckNullParam(handle);
    GlobusLIOCheckNullParam(port);
    
    if(attr)
    {
        GlobusLIOCheckAttr(attr, GLOBUS_I_IO_UDP_DRIVER_ATTR, GLOBUS_FALSE);
        result = globus_l_io_iattr_copy(&myattr, attr);
        if(result != GLOBUS_SUCCESS)
        {
            return result;
        }
        
        iattr = (globus_i_io_attr_t *) myattr;
        
        result = globus_xio_handle_attr_add_driver_attr(
            iattr->xio_attr, iattr->driver_attr);
        if(result != GLOBUS_SUCCESS)
        {
            goto destroy_myattr;
        }
    }
    
    result = globus_xio_factory_attr_init(&factory_attr);
    if(result != GLOBUS_SUCCESS)
    {
        goto destroy_myattr;
    }
    
    result = globus_xio_factory_attr_set_port(factory_attr, *port);
    if(result == GLOBUS_SUCCESS)
    {
        result = globus_xio_factory_push_driver(
            factory_attr, GLOBUS_XIO_UDP_DRIVER, GLOBUS_NULL);
        if(result == GLOBUS_SUCCESS)
        {
             result = globus_xio_factory_init(&factory, factory_attr);
        }
    }
    
    if(result != GLOBUS_SUCCESS)
    {
        goto destroy_factory_attr;
    }
    
    result = GlobusLIOMalloc(ihandle, globus_i_io_handle_t);
    if(result != GLOBUS_SUCCESS)
    {
        goto destroy_factory;
    }
    
    result = globus_xio_open(
        factory, &xio_handle, (attr ? iattr->xio_attr : GLOBUS_NULL));
    if(result != GLOBUS_SUCCESS)
    {
        globus_free(ihandle);
        goto destroy_factory;
    }
    
    ihandle->io_handle = handle;
    ihandle->xio_handle = xio_handle;
    ihandle->type = GLOBUS_I_IO_UDP_HANDLE;
    *handle = ihandle;
    
destroy_factory:
    globus_xio_factory_destroy(factory);
    
destroy_factory_attr:
    globus_xio_factory_attr_destroy(factory_attr);

destroy_myattr:
    if(attr)
    {
        globus_io_udpattr_destroy(&myattr);
    }
    
    return result;
}

/* need hash table */
globus_result_t
globus_io_udp_sendto(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    int                                 flags,
    globus_size_t                       nbytes,
    const char *                        host,
    unsigned short                      port,
    globus_size_t *                     bytes_sent)
{
    globus_i_io_handle_t *              ihandle;
    globus_xio_data_descriptor_t        dd;
    
    globus_result_t                     result;
    MyName(globus_io_udp_sendto);
    
    GlobusLIOCheckHandle(handle, GLOBUS_I_IO_UDP_HANDLE);
    
    ihandle = (globus_i_io_handle_t *) handle;
    
    result = globus_xio_data_descriptor_init(&dd, ihandle->xio_handle);
    if(result != GLOBUS_SUCCESS)
    {
        return result;
    }
    
    result = globus_xio_udp_data_descriptor_set_destination(dd, host, port);
    if(result == GLOBUS_SUCCESS)
    {
        result = globus_xio_udp_data_descriptor_set_flags(dd, flags);
    }
    
    if(result != GLOBUS_SUCCESS)
    {
        goto destroy_dd;
    }
    
    result = globus_xio_write(
        ihandle->xio_handle, buf, nbytes, dd, bytes_sent);
    if(result != GLOBUS_SUCCESS)
    {
        goto destroy_dd;
    }
    
    return GLOBUS_SUCCESS;
    
destroy_dd:
    globus_xio_data_descriptor_destroy(dd);
    
    return result;
}

globus_result_t
globus_io_udp_register_recvfrom(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    globus_size_t                       nbytes,
    int                                 flags,
    globus_io_udp_recvfrom_callback_t   recvfrom_callback,
    void *                              callback_arg)
{
    MyName(globus_io_udp_register_recvfrom);

}

globus_result_t
globus_io_udp_recvfrom(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    int                                 flags,
    globus_size_t                       nbytes,
    char **                             host,
    unsigned short *                    port,
    globus_size_t *                     nbytes_received)
{
    MyName(globus_io_udp_recvfrom);

}

/* tcp operations */

static
globus_bool_t
globus_l_io_gsi_authorization_callback(
    globus_xio_handle_t                         handle, 
    globus_result_t                             result,
    const char *                                identity,
    gss_ctx_id_t *                              context_handle,
    void *                                      user_arg)
{
    globus_l_gsi_authorization_callback_info_t * callback_info;
    
    callback_info = (globus_l_gsi_authorization_callback_info_t *) user_arg;
    
    return callback_info->callback(
        callback_info->callback_arg,
        &handle,
        result,
        identity,
        context_handle);
}

globus_result_t
globus_io_tcp_register_connect(
    char *                              host,
    unsigned short                      port,
    globus_io_attr_t *                  attr,
    globus_io_callback_t                callback,
    void *                              callback_arg,
    globus_io_handle_t *                handle)
{
    MyName(globus_io_tcp_register_connect);

}

globus_result_t
globus_io_tcp_connect(
    char *                              host,
    unsigned short                      port,
    globus_io_attr_t *                  attr,
    globus_io_handle_t *                handle)
{
    MyName(globus_io_tcp_connect);

}

globus_result_t
globus_io_tcp_create_listener(
    unsigned short *                    port,
    int                                 backlog,
    globus_io_attr_t *                  attr,
    globus_io_handle_t *                handle)
{
    MyName(globus_io_tcp_create_listener);

}

globus_result_t
globus_io_tcp_register_listen(
    globus_io_handle_t *                handle,
    globus_io_callback_t                callback,
    void *                              callback_arg)
{
    MyName(globus_io_tcp_register_listen);

}

globus_result_t
globus_io_tcp_listen(
    globus_io_handle_t *                handle)
{
    MyName(globus_io_tcp_listen);

}

globus_result_t
globus_io_tcp_register_accept(
    globus_io_handle_t *                listener_handle,
    globus_io_attr_t *                  attr,
    globus_io_handle_t *                new_handle,
    globus_io_callback_t                callback,
    void *                              callback_arg)
{
    MyName(globus_io_tcp_register_accept);

}

globus_result_t
globus_io_tcp_accept(
    globus_io_handle_t *                listener_handle,
    globus_io_attr_t *                  attr,
    globus_io_handle_t *                handle)
{
    MyName(globus_io_tcp_accept);

}

globus_result_t
globus_io_tcp_get_local_address(
    globus_io_handle_t *                handle,
    int *                               host,
    unsigned short *                    port)
{
    MyName(globus_io_tcp_get_local_address);

}

globus_result_t
globus_io_tcp_get_remote_address(
    globus_io_handle_t *                handle,
    int *                               host,
    unsigned short *                    port)
{
    MyName(globus_io_tcp_get_remote_address);

}

globus_result_t
globus_io_tcp_get_attr(
    globus_io_handle_t *                handle,
    globus_io_attr_t *                  attr)
{
    MyName(globus_io_tcp_get_attr);

}

globus_result_t
globus_io_tcp_set_attr(
    globus_io_handle_t *                handle,
    globus_io_attr_t *                  attr)
{
    MyName(globus_io_tcp_set_attr);

}

globus_result_t
globus_io_tcp_get_security_context(
    globus_io_handle_t *                handle,
    gss_ctx_id_t *                      context)
{
    MyName(globus_io_tcp_get_security_context);

}

globus_result_t
globus_io_tcp_get_delegated_credential(
    globus_io_handle_t *                handle,
    gss_cred_id_t *                     cred)
{
    MyName(globus_io_tcp_get_delegated_credential);

}

globus_result_t
globus_io_tcp_posix_convert(
    int                                 socket,
    globus_io_attr_t *                  attributes,
    globus_io_handle_t *                handle)
{
    MyName(globus_io_tcp_posix_convert);

}

globus_result_t
globus_io_tcp_posix_convert_listener(
    int                                 socket,
    globus_io_attr_t *                  attributes,
    globus_io_handle_t *                handle)
{
    MyName(globus_io_tcp_posix_convert_listener);

}

/* secure operations */

globus_result_t
globus_io_register_init_delegation(
    globus_io_handle_t *                handle,
    const gss_cred_id_t                 cred_handle,
    const gss_OID_set                   restriction_oids,
    const gss_buffer_set_t              restriction_buffers,
    OM_uint32                           time_req,
    globus_io_delegation_callback_t     callback,
    void *                              callback_arg)
{
    MyName(globus_io_register_init_delegation);

}

globus_result_t
globus_io_init_delegation(
    globus_io_handle_t *                handle,
    const gss_cred_id_t                 cred_handle,
    const gss_OID_set                   restriction_oids,
    const gss_buffer_set_t              restriction_buffers,
    OM_uint32                           time_req)
{
    MyName(globus_io_init_delegation);

}

globus_result_t
globus_io_register_accept_delegation(
    globus_io_handle_t *                handle,
    const gss_OID_set                   restriction_oids,
    const gss_buffer_set_t              restriction_buffers,
    OM_uint32                           time_req,
    globus_io_delegation_callback_t     callback,
    void *                              callback_arg)
{
    MyName(globus_io_register_accept_delegation);

}

globus_result_t
globus_io_accept_delegation(
    globus_io_handle_t *                handle,
    gss_cred_id_t *                     delegated_cred,
    const gss_OID_set                   restriction_oids,
    const gss_buffer_set_t              restriction_buffers,
    OM_uint32                           time_req,
    OM_uint32 *                         time_rec)
{
    MyName(globus_io_accept_delegation);

}

/* read operations */

globus_result_t
globus_io_register_read(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    globus_size_t                       max_nbytes,
    globus_size_t                       wait_for_nbytes,
    globus_io_read_callback_t           callback,
    void *                              callback_arg)
{
    MyName(globus_io_register_read);

}

globus_result_t
globus_io_try_read(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    globus_size_t                       max_nbytes,
    globus_size_t *                     nbytes_read)
{
    MyName(globus_io_try_read);

}

globus_result_t
globus_io_read(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    globus_size_t                       max_nbytes,
    globus_size_t                       wait_for_nbytes,
    globus_size_t *                     nbytes_read)
{
    MyName(globus_io_read);

}

/* write operations */

globus_result_t
globus_io_register_write(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    globus_size_t                       nbytes,
    globus_io_write_callback_t          write_callback,
    void *                              callback_arg)
{
    MyName(globus_io_register_write);

}

globus_result_t
globus_io_register_send(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    globus_size_t                       nbytes,
    int                                 flags,
    globus_io_write_callback_t          write_callback,
    void *                              callback_arg)
{
    MyName(globus_io_register_send);

}

globus_result_t
globus_io_register_writev(
    globus_io_handle_t *                handle,
    struct iovec *                      iov,
    globus_size_t                       iovcnt,
    globus_io_writev_callback_t         writev_callback,
    void *                              callback_arg)
{
    MyName(globus_io_register_writev);

}

globus_result_t
globus_io_try_write(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    globus_size_t                       max_nbytes,
    globus_size_t *                     nbytes_written)
{
    MyName(globus_io_try_write);

}

globus_result_t
globus_io_try_send(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    globus_size_t                       nbytes,
    int                                 flags,
    globus_size_t *                     nbytes_sent)
{
    MyName(globus_io_try_send);

}

globus_result_t
globus_io_write(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    globus_size_t                       nbytes,
    globus_size_t *                     nbytes_written)
{
    MyName(globus_io_write);

}

globus_result_t
globus_io_send(
    globus_io_handle_t *                handle,
    globus_byte_t *                     buf,
    globus_size_t                       nbytes,
    int                                 flags,
    globus_size_t *                     nbytes_sent)
{
    MyName(globus_io_send);

}

globus_result_t
globus_io_writev(
    globus_io_handle_t *                handle,
    struct iovec *                      iov,
    globus_size_t                       iovcnt,
    globus_size_t *                     bytes_written)
{
    MyName(globus_io_writev);

}

/* miscelaneous */

globus_result_t
globus_io_register_close(
    globus_io_handle_t *                handle,
    globus_io_callback_t                callback,
    void *                              callback_arg)
{
    MyName(globus_io_register_close);

}

globus_result_t
globus_io_close(
    globus_io_handle_t *                handle)
{
    MyName(globus_io_close);

}

globus_result_t
globus_io_register_cancel(
    globus_io_handle_t *                handle,
    globus_bool_t                       perform_callbacks,
    globus_io_callback_t                cancel_callback,
    void *                              cancel_arg)
{
    MyName(globus_io_register_cancel);

}

globus_result_t
globus_io_cancel(
    globus_io_handle_t *                handle,
    globus_bool_t                       perform_callbacks)
{
    MyName(globus_io_cancel);

}

globus_result_t
globus_io_register_select(
    globus_io_handle_t *                handle,
    globus_io_callback_t                read_callback_func,
    void *                              read_callback_arg,
    globus_io_callback_t                write_callback_func,
    void *                              write_callback_arg,
    globus_io_callback_t                except_callback_func,
    void *                              except_callback_arg)
{
    MyName(globus_io_register_select);

}

globus_io_handle_type_t
globus_io_get_handle_type(
    globus_io_handle_t *                handle)
{
    MyName(globus_io_get_handle_type);

}

globus_result_t
globus_io_handle_get_user_pointer(
    globus_io_handle_t *                handle,
    void **                             user_pointer)
{
    MyName(globus_io_handle_get_user_pointer);

}

globus_result_t
globus_io_handle_set_user_pointer(
    globus_io_handle_t *                handle,
    void *                              user_pointer)
{
    MyName(globus_io_handle_set_user_pointer);

}

globus_bool_t
globus_io_eof(
    globus_object_t *                   eof)
{
    MyName(globus_io_eof);

}
