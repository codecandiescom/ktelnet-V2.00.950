/*
 * Adapted for Windows 95/98/NT and Borland C++ 5.0
 * by Thomas Nyström, Stacken Computer Club
 * 1998-12-30 20:47:46
 */	 

/* 
  Copyright (C) 1989 by the Massachusetts Institute of Technology

   Export of this software from the United States of America is assumed
   to require a specific license from the United States Government.
   It is the responsibility of any person or organization contemplating
   export to obtain such a license before exporting.

WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
distribute this software and its documentation for any purpose and
without fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright notice and
this permission notice appear in supporting documentation, and that
the name of M.I.T. not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.  M.I.T. makes no representations about the suitability of
this software for any purpose.  It is provided "as is" without express
or implied warranty.

  */

#include "krb_locl.h"
#include <base64.h>


RCSID("$Id: send_to_kdc.c,v 1.59 1998/07/22 13:14:52 assar Exp $");

void krb_wdump(const unsigned char *b, int len);

struct host {
    struct sockaddr_in addr;
    enum krb_host_proto proto;
    char *host_name;
};

static const char *prog = "send_to_kdc";
static send_recv(KTEXT pkt, KTEXT rpkt, int f,
		 struct sockaddr_in *adr, struct host *addrs);

#define PROXY_VAR "krb4_proxy"

/*
 * send_to_kdc() sends a message to the Kerberos authentication
 * server(s) in the given realm and returns the reply message.
 * The "pkt" argument points to the message to be sent to Kerberos;
 * the "rpkt" argument will be filled in with Kerberos' reply.
 * The "realm" argument indicates the realm of the Kerberos server(s)
 * to transact with.  If the realm is null, the local realm is used.
 *
 * If more than one Kerberos server is known for a given realm,
 * different servers will be queried until one of them replies.
 * Several attempts (retries) are made for each server before
 * giving up entirely.
 *
 * If an answer was received from a Kerberos host, KSUCCESS is
 * returned.  The following errors can be returned:
 *
 * SKDC_CANT    - can't get local realm
 *              - can't find "kerberos" in /etc/services database
 *              - can't open socket
 *              - can't bind socket
 *              - all ports in use
 *              - couldn't find any Kerberos host
 *
 * SKDC_RETRY   - couldn't get an answer from any Kerberos server,
 *		  after several retries
 */

/* always use the admin server */
static int krb_use_admin_server_flag = 0;

int
KRB_LIB_FUNCTION
krb_use_admin_server(int flag)
{
    int old = krb_use_admin_server_flag;
    krb_use_admin_server_flag = flag;
    return old;
}

int
KRB_LIB_FUNCTION
send_to_kdc(KTEXT pkt, KTEXT rpkt, char *realm)
{
    int i;
    int no_host; /* was a kerberos host found? */
    int retry;
    int n_hosts;
    int retval;
    struct hostent *host;
    char lrealm[REALM_SZ];
    struct krb_host *k_host;
    struct host *hosts = malloc(sizeof(*hosts));

    if (hosts == NULL)
	return SKDC_CANT;

    /*
     * If "realm" is non-null, use that, otherwise get the
     * local realm.
     */
    if (realm)
	strcpy_truncate(lrealm, realm, REALM_SZ);
    else
	if (krb_get_lrealm(lrealm,1)) {
	    if (krb_debug)
		krb_warning("%s: can't get local realm\n", prog);
	    return(SKDC_CANT);
	}
    if (krb_debug)
	krb_warning("lrealm is %s\n", lrealm);

    no_host = 1;
    /* get an initial allocation */
    n_hosts = 0;
    for (i = 1;
	 ((k_host = krb_get_host(i, lrealm, krb_use_admin_server_flag)) != 0);
	 ++i) {
	char *p;
	char **addr_list;

	if (krb_debug)
		krb_warning("Checking: %s\n", k_host->host);

	/* THN: If protocol is HTTP and proxy is activated we should not
	 * try to check the hostname, we might be in a private name-
	 * space and the address can't be validated */

	if ((k_host->proto != PROTO_HTTP) ||
	    (krb_get_config_string(PROXY_VAR) == NULL)) {
	  if (krb_debug)
	    krb_warning("Getting host entry for %s...", k_host->host);
	  host = gethostbyname(k_host->host);
	  if (krb_debug) {
	    krb_warning("%s.\n",
			host ? "Got it" : "Didn't get it");
	  }
	  if (!host)
            continue;

	  no_host = 0;    /* found at least one */
	  for (addr_list = host->h_addr_list;
	       ((p = *addr_list) != 0);
	       ++addr_list) {
	    hosts = realloc(hosts, sizeof(*hosts) * (n_hosts + 1));
	    if (hosts == NULL)
		return SKDC_CANT;
	    memset (&hosts[n_hosts].addr, 0, sizeof(hosts[n_hosts].addr));
	    hosts[n_hosts].addr.sin_family = host->h_addrtype;
	    hosts[n_hosts].addr.sin_port = htons((u_short)k_host->port);
	    hosts[n_hosts].proto = k_host->proto;
	    memcpy(&hosts[n_hosts].addr.sin_addr, p,
		   sizeof(hosts[n_hosts].addr.sin_addr));
	    ++n_hosts;
	    if (send_recv(pkt, rpkt, hosts[n_hosts-1].proto,
			  &hosts[n_hosts-1].addr, &hosts[n_hosts-1])) {
		retval = KSUCCESS;
		goto rtn;
	    }
	    if (krb_debug) {
		krb_warning("Timeout, error, or wrong descriptor\n");
	    }
	  }
	} else {	/* Using proxy. */
	    hosts = realloc(hosts, sizeof(*hosts) * (n_hosts + 1));
	    if (hosts == NULL)
		return SKDC_CANT;
	    memset (&hosts[n_hosts].addr, 0, sizeof(hosts[n_hosts].addr));
	    hosts[n_hosts].addr.sin_port = htons((u_short)k_host->port);
	    hosts[n_hosts].proto = k_host->proto;
	    hosts[n_hosts].host_name = k_host->host;
	    ++n_hosts;
	    if (send_recv(pkt, rpkt, hosts[n_hosts-1].proto,
			  &hosts[n_hosts-1].addr, &hosts[n_hosts-1])) {
		retval = KSUCCESS;
		goto rtn;
	    }
	    if (krb_debug) {
		krb_warning("Timeout, error, or wrong descriptor\n");
	    }
	}
    }
    if (no_host) {
	if (krb_debug)
	    krb_warning("%s: can't find any Kerberos host.\n",
			prog);
        retval = SKDC_CANT;
        goto rtn;
    }
    /* retry each host in sequence */
    for (retry = 0; retry < CLIENT_KRB_RETRY; ++retry) {
	for (i = 0; i < n_hosts; ++i) {
	    if (send_recv(pkt, rpkt,
			  hosts[i].proto,
			  &hosts[i].addr,
			  &hosts[i])) {
		retval = KSUCCESS;
		goto rtn;
	    }
        }
    }
    retval = SKDC_RETRY;
rtn:
    free(hosts);
    return(retval);
}

static int
udp_socket(void)
{
    return socket(AF_INET, SOCK_DGRAM, 0);
}

static int
udp_connect(int s, struct sockaddr_in *adr)
{
    if(krb_debug) {
	krb_warning("connecting to %s udp, port %d\n", 
		    inet_ntoa(adr->sin_addr), 
		    ntohs(adr->sin_port));
    }
    return connect(s, (struct sockaddr*)adr, sizeof(*adr));
}

static int
udp_send(int s, struct sockaddr_in* adr, KTEXT pkt, char *host_name)
{
    if(krb_debug) {
	krb_warning("sending %d bytes to %s, udp port %d\n", 
		    pkt->length,
		    inet_ntoa(adr->sin_addr), 
		    ntohs(adr->sin_port));
        krb_wdump(pkt->dat, pkt->length);	    
    }
    return send(s, (void *)pkt->dat, pkt->length, 0);
}

static int
tcp_socket(void)
{
    return socket(AF_INET, SOCK_STREAM, 0);
}

static int
tcp_connect(int s, struct sockaddr_in *adr)
{
    if(krb_debug) {
	krb_warning("connecting to %s, tcp port %d\n", 
		    inet_ntoa(adr->sin_addr), 
		    ntohs(adr->sin_port));
    }
    return connect(s, (struct sockaddr*)adr, sizeof(*adr));
}

static int
tcp_send(int s, struct sockaddr_in* adr, KTEXT pkt, char *host_name)
{
    unsigned char len[4];
    if(krb_debug) {
	krb_warning("sending %d bytes to %s, tcp port %d\n", 
		    pkt->length,
		    inet_ntoa(adr->sin_addr), 
		    ntohs(adr->sin_port));
    }
    krb_put_int(pkt->length, len, sizeof(len), 4);
    if(send(s, (void *)len, sizeof(len), 0) != sizeof(len))
	return -1;
    return send(s, (void *)pkt->dat, pkt->length, 0);
}

static int
udptcp_recv(void *buf, size_t len, KTEXT rpkt)
{
    int pktlen = min(len, MAX_KTXT_LEN);

    if(krb_debug) {
	krb_warning("recieved %d bytes on udp/tcp socket\n", len);
        krb_wdump(buf, pktlen);	    
	}
    memcpy(rpkt->dat, buf, pktlen);
    rpkt->length = pktlen;
    return 0;
}

static int
url_parse(const char *url, char *host, size_t len, short *port)
{
    const char *p;
    size_t n;

    if(strncmp(url, "http://", 7))
        return -1;
    url += 7;
    p = strchr(url, ':');
    if(p) {
        char *end;

        *port = htons((short)strtol(p + 1, &end, 0));
        if (end == p + 1)
            return -1;
        n = p - url;
    } else {
        *port = (short)k_getportbyname ("http", "tcp", htons(80));
        p = strchr(url, '/');
        if (p)
            n = p - url;
        else
            n = strlen(url);
    }
    if (n >= len)
        return -1;
    memcpy(host, url, n);
    host[n] = '\0';
    return 0;
}

static int
http_connect(int s, struct sockaddr_in *adr)
{
    const char *proxy = krb_get_config_string(PROXY_VAR);
    char host[MaxHostNameLen];
    short port;
    struct hostent *hp;
    struct sockaddr_in sin;
    if(proxy == NULL) {
	if(krb_debug)
	    krb_warning("Not using proxy.\n");
	return tcp_connect(s, adr);
    }
    krb_warning("Try proxy: %s\n", proxy);	
    if(url_parse(proxy, host, sizeof(host), &port) < 0)
	return -1;
    krb_warning("Host: %s, port: %d\n", host, ntohs(port));
    hp = gethostbyname(host);
    if(hp == NULL) {
	krb_warning("Bad hostname\n");
	return -1;
	}
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    memcpy(&sin.sin_addr, hp->h_addr, sizeof(sin.sin_addr));
    sin.sin_port = port;
    if(krb_debug) {
	krb_warning("connecting to proxy on %s (%s) port %d\n", 
		    host, inet_ntoa(sin.sin_addr), ntohs(port));
    }
    return connect(s, (struct sockaddr*)&sin, sizeof(sin));
}

static int
http_send(int s, struct sockaddr_in* adr, KTEXT pkt, char *host_name)
{
    char *str;
    char *msg;

    if (base64_encode(pkt->dat, pkt->length, &str) < 0)
	return -1;
    if(krb_get_config_string(PROXY_VAR)){
      if (host_name == NULL) {
	if (krb_debug)
	  krb_warning("http_send: Null host_name\n");
	return -1;
      }

      if(krb_debug) {
	krb_warning("sending %d bytes to %s, tcp port %d (via proxy)\n", 
		    pkt->length,
		    host_name,	/*inet_ntoa(adr->sin_addr), */
		    ntohs(adr->sin_port));
        krb_wdump(pkt->dat, pkt->length);	    
        krb_wdump((unsigned char *)str, strlen(str));
      }
      asprintf(&msg, "GET http://%s:%d/%s HTTP/1.0\r\n\r\n",
	       host_name,	/*inet_ntoa(adr->sin_addr),*/
	       ntohs(adr->sin_port),
	       str);
    } else {
      if(krb_debug) {
	krb_warning("sending %d bytes to %s, http port %d\n", 
		    pkt->length,
		    inet_ntoa(adr->sin_addr), 
		    ntohs(adr->sin_port));
        krb_wdump(pkt->dat, pkt->length);
        krb_wdump((unsigned char *)str, strlen(str));
      }
      asprintf(&msg, "GET %s HTTP/1.0\r\n\r\n", str);
    }
    free(str);

    if (msg == NULL)
	return -1;
	
    if(send(s, msg, strlen(msg), 0) != strlen(msg)){
	free(msg);
	return -1;
    }
    free(msg);
    return 0;
}

static int
http_recv(void *buf, size_t len, KTEXT rpkt)
{
    char *p;
    char *tmp = malloc(len + 1);

    if (krb_debug) {
	    krb_warning("http_recv:\n");
	    krb_wdump(buf, len);
	    }

    if (tmp == NULL)
	return -1;
    memcpy(tmp, buf, len);
    tmp[len] = 0;
    p = strstr(tmp, "\r\n\r\n");
    if(p == NULL){
	free(tmp);
	return -1;
    }
    p += 4;
    if(krb_debug)
	krb_warning("recieved %d bytes on http socket\n", (tmp + len) - p);
    if((tmp + len) - p > MAX_KTXT_LEN) {
	free(tmp);
	return -1;
    }
    memcpy(rpkt->dat, p, (tmp + len) - p);
    rpkt->length = (tmp + len) - p;
    free(tmp);
    return 0;
}

static struct proto_descr {
    int proto;
    int stream_flag;
    int (*socket)(void);
    int (*connect)(int, struct sockaddr_in*);
    int (*send)(int, struct sockaddr_in*, KTEXT, char *host_name);
    int (*recv)(void*, size_t, KTEXT);
} protos[] = {
    { PROTO_UDP, 0, udp_socket, udp_connect, udp_send, udptcp_recv },
    { PROTO_TCP, 1, tcp_socket, tcp_connect, tcp_send, udptcp_recv },
    { PROTO_HTTP, 1, tcp_socket, http_connect, http_send, http_recv }
};

static int
send_recv(KTEXT pkt, KTEXT rpkt, int proto, struct sockaddr_in *adr,
	  struct host *addrs)
{
    int i;
    int s;
    unsigned char buf[MAX_KTXT_LEN];
    int offset = 0;
    
    for(i = 0; i < sizeof(protos) / sizeof(protos[0]); i++){
	if(protos[i].proto == proto)
	    break;
    }
    if(i == sizeof(protos) / sizeof(protos[0]))
	return FALSE;
    if((s = (*protos[i].socket)()) < 0)
	return FALSE;
    if((*protos[i].connect)(s, adr) < 0){
	close(s);
	return FALSE;
    }
    if((*protos[i].send)(s, adr, pkt, addrs->host_name) < 0){
	close(s);
	return FALSE;
    }
    do{
	fd_set readfds;
	struct timeval timeout;
	int len;
	timeout.tv_sec = CLIENT_KRB_TIMEOUT;
	timeout.tv_usec = 0;
	FD_ZERO(&readfds);
	FD_SET(s, &readfds);
	
	/* select - either recv is ready, or timeout */
	/* see if timeout or error or wrong descriptor */
	if(select(s + 1, &readfds, 0, 0, &timeout) < 1 
	   || !FD_ISSET(s, &readfds)) {
	    if (krb_debug)
		krb_warning("select failed: errno = %d\n", errno);
	    close(s);
	    return FALSE;
	}
	len = recv(s, (void *)(buf + offset), sizeof(buf) - offset, 0);
	if (len < 0) {
	    close(s);
	    return FALSE;
	}
	if(len == 0)
	    break;
	offset += len;
    } while(protos[i].stream_flag);
    close(s);
    if((*protos[i].recv)(buf, offset, rpkt) < 0)
	return FALSE;
    return TRUE;
}

/* The configuration line "hosts: dns files" in /etc/nsswitch.conf is
 * rumored to avoid triggering this bug. */
#if defined(linux) && defined(HAVE__DNS_GETHOSTBYNAME) && 0
/* Linux libc 5.3 is broken probably somewhere in nsw_hosts.o,
 * for now keep this kludge. */
static
struct hostent *gethostbyname(const char *name)
{
  return (void *)_dns_gethostbyname(name);
}
#endif
