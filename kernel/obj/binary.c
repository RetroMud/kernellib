# include <kernel/kernel.h>
# include <kernel/user.h>

inherit LIB_CONN;	/* basic connection object */


object driver;		/* driver object */
string buffer;		/* buffered input */

/*
 * NAME:	create()
 * DESCRIPTION:	initialize
 */
static void create(int clone)
{
    if (clone) {
	::create("binary");
	driver = find_object(DRIVER);
	buffer = "";
    }
}

/*
 * NAME:	open()
 * DESCRIPTION:	open the connection
 */
static int open()
{
    ::open(allocate(driver->query_tls_size()));
# ifdef SYS_DATAGRAMS
    return TRUE;
# else
    return FALSE;
# endif
}

/*
 * NAME:	close()
 * DESCRIPTION:	close the connection
 */
static void close(int dest)
{
    ::close(allocate(driver->query_tls_size()), dest);
}

/*
 * NAME:	receive_message()
 * DESCRIPTION:	forward a message to listeners
 */
static void receive_message(string str)
{
    int mode, len;
    string head, pre;
    mixed *tls;

    tls = allocate(driver->query_tls_size());
    buffer += str;
    while (this_object() &&
	   (mode=query_mode()) != MODE_BLOCK && mode != MODE_DISCONNECT) {
	if (mode != MODE_RAW) {
	    if (sscanf(buffer, "%s\r\n%s", str, buffer) != 0 ||
		sscanf(buffer, "%s\n%s", str, buffer) != 0) {
		while (sscanf(str, "%s\b%s", head, str) != 0) {
		    while (sscanf(head, "%s\x7f%s", pre, head) != 0) {
			len = strlen(pre);
			if (len != 0) {
			    head = pre[0 .. len - 2] + head;
			}
		    }
		    len = strlen(head);
		    if (len != 0) {
			str = head[0 .. len - 2] + str;
		    }
		}
		while (sscanf(str, "%s\x7f%s", head, str) != 0) {
		    len = strlen(head);
		    if (len != 0) {
			str = head[0 .. len - 2] + str;
		    }
		}

		::receive_message(tls, str);
	    } else {
		break;
	    }
	} else {
	    if (strlen(buffer) != 0) {
		str = buffer;
		buffer = "";
		::receive_message(tls, str);
	    }
	    break;
	}
    }
}

# ifdef SYS_DATAGRAMS
/*
 * NAME:	receive_datagram()
 * DESCRIPTION:	receive a datagram
 */
static void receive_datagram(string str)
{
    ::receive_datagram(allocate(driver->query_tls_size()), str);
}
# endif

/*
 * NAME:	set_mode()
 * DESCRIPTION:	set the connection mode
 */
void set_mode(int mode)
{
    string str;

    if (KERNEL() || SYSTEM()) {
	::set_mode(mode);
	if (mode == MODE_RAW && strlen(buffer) != 0) {
	    /* flush buffer */
	    str = buffer;
	    buffer = "";
	    ::receive_message(nil, str);
	}
    }
}

/*
 * NAME:	message()
 * DESCRIPTION:	send a message to the other side
 */
int message(string str)
{
    if (query_mode() < MODE_RAW) {
	str = implode(explode("\n" + str + "\n", "\n"), "\r\n");
    }
    return ::message(str);
}

/*
 * NAME:	message_done()
 * DESCRIPTION:	called when output is completed
 */
static void message_done()
{
    ::message_done(allocate(driver->query_tls_size()));
}
