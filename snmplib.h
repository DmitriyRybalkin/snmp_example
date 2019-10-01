

#ifndef SNMPLIB_H
#define SNMPLIB_H

#ifdef __cplusplus
    extern "C" {
#endif
  
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
    
namespace SNMPLib {

  int request_state = 5;
  /*
   * a list of hosts to query
   */
  struct host {
    const char *name;
    const char *community;
  } hosts[] = {
    { "192.168.1.4", "public" },
    { NULL }
  };
  
  /*
   * a list of variables to query for
   */
  struct oid_struct {
    const char *Name;
    oid Oid[MAX_OID_LEN];
    int OidLen;
  } oids[] = {
    { "sysDescr.0" },
    { NULL }
  };

  /* initialize */
  void initialize (void)
  {
    struct oid_struct *op = oids;
    
    /* initialize library */
    init_snmp("asynchapp");

    /* parse the oids */
    while (op->Name) {
      op->OidLen = sizeof(op->Oid)/sizeof(op->Oid[0]);
      if (!get_node(op->Name, op->Oid, (unsigned int *) &op->OidLen)) {
	snmp_perror("get_node");
	exit(1);
      }
      op++;
    }
  }

  /* simple printing of returned data */
  int print_result (int status, struct snmp_session *sp, struct snmp_pdu *pdu)
  {
    char buf[1024];
    struct variable_list *vp;
    int ix;
    struct timeval now;
    struct timezone tz;
    struct tm *tm;

    gettimeofday(&now, &tz);
    tm = localtime(&now.tv_sec);
    fprintf(stdout, "%.2d:%.2d:%.2d.%.6d ", tm->tm_hour, tm->tm_min, tm->tm_sec,
	    now.tv_usec);
    switch (status) {
    case STAT_SUCCESS:
      vp = pdu->variables;
      if (pdu->errstat == SNMP_ERR_NOERROR) {
	while (vp) {
	  snprint_variable(buf, sizeof(buf), vp->name, vp->name_length, vp);
	  fprintf(stdout, "%s: %s\n", sp->peername, buf);
	  vp = vp->next_variable;
	}
      }
      else {
	for (ix = 1; vp && ix != pdu->errindex; vp = vp->next_variable, ix++)
	  ;
	if (vp) snprint_objid(buf, sizeof(buf), vp->name, vp->name_length);
	else strcpy(buf, "(none)");
	fprintf(stdout, "%s: %s: %s\n",
	  sp->peername, buf, snmp_errstring(pdu->errstat));
      }
      return 1;
    case STAT_TIMEOUT:
      fprintf(stdout, "%s: Timeout\n", sp->peername);
      return 0;
    case STAT_ERROR:
      snmp_perror(sp->peername);
      return 0;
    }
    return 0;
  }

  /* poll all hosts in parallel */
  struct session {
    struct snmp_session *sess;		/* SNMP session data */
    struct oid_struct *current_oid;		/* How far in our poll are we */
  } sessions[sizeof(hosts)/sizeof(hosts[0])];

  /* hosts that we have not completed */
  int active_hosts;
    
  /* callback function, to be called whenever a response is received */
  int asynch_response(int operation, struct snmp_session *sp, int reqid,
		      struct snmp_pdu *pdu, void *magic)
  {
    struct session *host = (struct session *)magic;
    struct snmp_pdu *req;
    
    /* whenever a response is received, we will print it out, and send the next request out */
    if (operation == NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE) {
      if (print_result(STAT_SUCCESS, host->sess, pdu)) {
	request_state = STAT_SUCCESS;
	host->current_oid++;                      /* send next GET (if any) */
	if (host->current_oid->Name) {
	  req = snmp_pdu_create(SNMP_MSG_GET);
	  snmp_add_null_var(req, host->current_oid->Oid, host->current_oid->OidLen);
	  if (snmp_send(host->sess, req))
	    return 1;
	  else {
	    snmp_perror("snmp_send");
	    snmp_free_pdu(req);
	  }
	}
      }
    } else {
      request_state = STAT_TIMEOUT;
      print_result(STAT_TIMEOUT, host->sess, pdu);  
    }
    
    /* something went wrong (or end of variables)
    * this host not active any more
    */
    active_hosts--;
    return 1;
  }

  void asynchronous(void)
  {
    struct session *hs;
    struct host *hp;

    /* startup all hosts 
      * loop through all the hosts, opening a session for each and sending out the first request
      */
    for (hs = sessions, hp = hosts; hp->name; hs++, hp++) {
      struct snmp_pdu *req;
      struct snmp_session sess;
      
      /* initialize session */
      snmp_sess_init(&sess);
      sess.version = SNMP_VERSION_2c;
      sess.peername = (char *)hp->name;
      sess.community = (unsigned char*)hp->community;
      sess.community_len = strlen((const char*)sess.community);
      
	/* default callback */
      sess.callback = asynch_response;
      sess.callback_magic = hs;
      if (!(hs->sess = snmp_open(&sess))) {
	snmp_perror("snmp_open");
	continue;
      }
      hs->current_oid = oids;
      /* send the first GET */
      req = snmp_pdu_create(SNMP_MSG_GET);
      snmp_add_null_var(req, hs->current_oid->Oid, hs->current_oid->OidLen);
      if (snmp_send(hs->sess, req))
	active_hosts++;
      else {
	snmp_perror("snmp_send");
	snmp_free_pdu(req);
      }
    }
    
    /* loop while any active hosts */
    while (active_hosts) {
      int fds = 0, block = 1;
      fd_set fdset;
      struct timeval timeout;
      
      /* snmp_select_info is the function that gets us all we need to be able to call select */
      FD_ZERO(&fdset);
      snmp_select_info(&fds, &fdset, &timeout, &block);
      fds = select(fds, &fdset, NULL, NULL, block ? NULL : &timeout);
      
      /* snmp_read will read all sockets with pending data, and process them  */
      if (fds) snmp_read(&fdset);
      else snmp_timeout();
    }
    
    /* cleanup */
    for (hp = hosts, hs = sessions; hp->name; hs++, hp++) {
      if (hs->sess) snmp_close(hs->sess);
    }
  }
};

#ifdef __cplusplus
    }
#endif
    
#endif