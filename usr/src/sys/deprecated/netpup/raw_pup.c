/*	raw_pup.c	4.4	82/03/03	*/

#include "../h/param.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/socketvar.h"
#include "../net/in.h"
#include "../net/in_systm.h"
#include "../net/pup.h"
#include "../net/raw_cb.h"
#include "/usr/include/errno.h"

/*
 * Raw PUP protocol interface.
 */

/*ARGSUSED*/
rpup_ctlinput(m)
	struct mbuf *m;
{
COUNT(RPUP_CTLINPUT);
}

/*
 * Encapsulate packet in PUP header which is supplied by the
 * user.  This is done to allow user to specify PUP identifier.
 */
rpup_output(m0, so)
	struct mbuf *m0;
	struct socket *so;
{
	register struct rawcb *rp = sotorawcb(so);
	register struct pup_header *pup;
	int len;
	struct mbuf *n;
	struct sockaddr_pup *spup;
	struct ifnet *ifp;

COUNT(RPUP_OUTPUT);
	/*
	 * Verify user has supplied necessary space
	 * for the header and check parameters in it.
	 */
	if ((m->m_off > MMAXOFF || m->m_len < sizeof(struct pup_header)) &&
	    (m = m_pullup(m, sizeof(struct pup_header)) == 0) {
		goto bad;
	pup = mtod(m, struct pup_header *);
	if (pup->pup_type == 0)
		goto bad;
	if (pup->pup_tcontrol && (pup->pup_tcontrol & ~PUP_TRACE))
		goto bad;
	for (len = 0, n = m; n; n = n->m_next)
		len += n->m_len;
	pup->pup_length = len;
	spup = (struct sockaddr_pup *)&rp->rcb_addr;
	pup->pup_dport = spup->spup_addr;

	/*
	 * Insure proper source address is included.
	 */
	spup = (struct sockadrr_pup *)rp->rcb_socket->so_addr;
	pup->pup_sport = spup->spup_addr;
	/* for now, assume user generates PUP checksum. */

	ifp = if_ifonnetof(&rp->rcb_addr);
	if (ifp == 0) {
		ifp = if_gatewayfor(&rp->rcb_addr);
		if (ifp == 0)
			goto bad;
	}
	return (enoutput((struct ifnet *)rp->rcb_pcb, m, PF_PUP));

bad:
	m_freem(m);
	return (0);
}
