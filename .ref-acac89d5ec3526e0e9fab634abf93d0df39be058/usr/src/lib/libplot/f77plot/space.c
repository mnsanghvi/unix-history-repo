/*-
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.proprietary.c%
 */

#ifndef lint
static char sccsid[] = "@(#)space.c	8.1 (Berkeley) %G%";
#endif /* not lint */

space_(x0,y0,x1,y1)
int *x0, *y0, *x1, *y1;
{
	space(*x0,*y0,*x1,*y1);
}
