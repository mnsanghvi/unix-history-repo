/*-
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Berkeley Software Design Inc's name may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN INC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      from nfs_lock.h,v 2.2 1998/04/28 19:38:41 don Exp
 * $FreeBSD$
 */

/*
 * lockd uses the nfssvc system call to get the unique kernel services it needs.
 * It passes in a request structure with a version number at the start.
 * This prevents libc from needing to change if the information passed 
 * between lockd and the kernel needs to change.
 *
 * If a structure changes, you must bump the version number.
 */

#include <nfs/nfsproto.h>


#define LOCKD_REQ_VERSION	1

struct lockd_req {
	int			vers;	/* keep in sync with kernel please */
	int 		op;		/* F_GETLK | F_SETLK | F_UNLCK */
	int		owner;		/* owner of lock, -1 to allocate one */
	int		owner_rel_ok;	/* release owner if no locks left ? */
	int		*owner_ret;	/* owner alloc/free result target */
	void 		*fh;		/* NFS file handle */
	size_t 		fh_len;		/* NFS file handle length */
	u_quad_t 	offset;		/* offset of where to start lock */
	u_quad_t 	len;		/* length of range to lock */
	int 		type;		/* F_RDLCK | F_WRLCK | F_UNLCK */
	struct ucred	cred;		/* user credentials to use for lock */
	struct sockaddr saddr;		/* XXX how about non AF_INET ?? */
	int		pid;		/* pid of lock requester */
};

/* 
 * The fifo where the kernel writes requests for locks on remote NFS files,
 * and where lockd reads these requests.
 *
 */
#define	_PATH_LCKFIFO	"/var/run/lock"

/*
 * This structure is used to uniquely identify the process which originated
 * a particular message to lockd.  A sequence number is used to differentiate
 * multiple messages from the same process.  A process start time is used to
 * detect the unlikely, but possible, event of the recycling of a pid.
 */
struct lockd_msg_ident {
	pid_t		pid;            /* The process ID. */
	struct timeval	pid_start;	/* Start time of process id */
	int		msg_seq;	/* Sequence number of message */
};

#define LOCKD_MSG_VERSION	1

/*
 * The structure that the kernel hands us for each lock request.
 */
typedef struct __lock_msg {
	int			lm_version;	/* which version is this */
	struct lockd_msg_ident	lm_msg_ident;	/* originator of the message */
	struct flock		lm_fl;             /* The lock request. */
	int			lm_wait;           /* The F_WAIT flag. */
	int			lm_getlk;		/* is this a F_GETLK request */
	struct sockaddr 	lm_addr;		/* The address. */
	int			lm_nfsv3;		/* If NFS version 3. */
	size_t			lm_fh_len;		/* The file handle length. */
	struct ucred		lm_cred;		/* user cred for lock req */
	u_int8_t		lm_fh[NFS_SMALLFH];/* The file handle. */
} LOCKD_MSG;

#define LOCKD_ANS_VERSION	1

struct lockd_ans {
	int		la_vers;
	struct lockd_msg_ident	la_msg_ident;	/* originator of the message */
	int		la_errno;
	int		la_set_getlk_pid;		/* use returned pid */
	int		la_getlk_pid;		/* returned pid for F_GETLK */
};

#ifdef _KERNEL
int	nfs_dolock(struct vop_advlock_args *ap);
int nfslockdans(struct proc *p, struct lockd_ans *ansp);
int nfslockdreq(struct proc *p, struct lockd_req *reqp);
#endif
