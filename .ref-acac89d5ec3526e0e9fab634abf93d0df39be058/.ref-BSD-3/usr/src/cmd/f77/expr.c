#include "defs"

/* little routines to create constant blocks */

struct Constblock *mkconst(t)
register int t;
{
register struct Constblock *p;

p = ALLOC(Constblock);
p->tag = TCONST;
p->vtype = t;
return(p);
}


struct Constblock *mklogcon(l)
register int l;
{
register struct Constblock * p;

p = mkconst(TYLOGICAL);
p->const.ci = l;
return(p);
}



struct Constblock *mkintcon(l)
ftnint l;
{
register struct Constblock *p;

p = mkconst(TYLONG);
p->const.ci = l;
#ifdef MAXSHORT
	if(l >= -MAXSHORT   &&   l <= MAXSHORT)
		p->vtype = TYSHORT;
#endif
return(p);
}



struct Constblock *mkaddcon(l)
register int l;
{
register struct Constblock *p;

p = mkconst(TYADDR);
p->const.ci = l;
return(p);
}



struct Constblock *mkrealcon(t, d)
register int t;
double d;
{
register struct Constblock *p;

p = mkconst(t);
p->const.cd[0] = d;
return(p);
}


struct Constblock *mkbitcon(shift, leng, s)
int shift;
int leng;
char *s;
{
register struct Constblock *p;

p = mkconst(TYUNKNOWN);
p->const.ci = 0;
while(--leng >= 0)
	if(*s != ' ')
		p->const.ci = (p->const.ci << shift) | hextoi(*s++);
return(p);
}





struct Constblock *mkstrcon(l,v)
int l;
register char *v;
{
register struct Constblock *p;
register char *s;

p = mkconst(TYCHAR);
p->vleng = ICON(l);
p->const.ccp = s = (char *) ckalloc(l);
while(--l >= 0)
	*s++ = *v++;
return(p);
}


struct Constblock *mkcxcon(realp,imagp)
register expptr realp, imagp;
{
int rtype, itype;
register struct Constblock *p;

rtype = realp->headblock.vtype;
itype = imagp->headblock.vtype;

if( ISCONST(realp) && ISNUMERIC(rtype) && ISCONST(imagp) && ISNUMERIC(itype) )
	{
	p = mkconst( (rtype==TYDREAL||itype==TYDREAL) ? TYDCOMPLEX : TYCOMPLEX );
	if( ISINT(rtype) )
		p->const.cd[0] = realp->constblock.const.ci;
	else	p->const.cd[0] = realp->constblock.const.cd[0];
	if( ISINT(itype) )
		p->const.cd[1] = imagp->constblock.const.ci;
	else	p->const.cd[1] = imagp->constblock.const.cd[0];
	}
else
	{
	err("invalid complex constant");
	p = errnode();
	}

frexpr(realp);
frexpr(imagp);
return(p);
}


struct Errorblock *errnode()
{
struct Errorblock *p;
p = ALLOC(Errorblock);
p->tag = TERROR;
p->vtype = TYERROR;
return(p);
}





expptr mkconv(t, p)
register int t;
register expptr p;
{
register expptr q;
register int pt;
expptr opconv();

if(t==TYUNKNOWN || t==TYERROR)
	fatali("mkconv of impossible type %d", t);
pt = p->headblock.vtype;
if(t == pt)
	return(p);

else if( ISCONST(p) && pt!=TYADDR)
	{
	q = mkconst(t);
	consconv(t, &(q->constblock.const),
		p->constblock.vtype, &(p->constblock.const) );
	frexpr(p);
	}
#if TARGET == PDP11
	else if(ISINT(t) && pt==TYCHAR)
		{
		q = mkexpr(OPBITAND, opconv(p,TYSHORT), ICON(255));
		if(t == TYLONG)
			q = opconv(q, TYLONG);
		}
#endif
else
	q = opconv(p, t);

if(t == TYCHAR)
	q->constblock.vleng = ICON(1);
return(q);
}



expptr opconv(p, t)
expptr p;
int t;
{
register expptr q;

q = mkexpr(OPCONV, p, 0);
q->headblock.vtype = t;
return(q);
}



struct Exprblock *addrof(p)
expptr p;
{
return( mkexpr(OPADDR, p, NULL) );
}



tagptr cpexpr(p)
register tagptr p;
{
register tagptr e;
int tag;
register chainp ep, pp;
ptr cpblock();

static int blksize[ ] =
	{	0,
		sizeof(struct Nameblock),
		sizeof(struct Constblock),
		sizeof(struct Exprblock),
		sizeof(struct Addrblock),
		sizeof(struct Primblock),
		sizeof(struct Listblock),
		sizeof(struct Errorblock)
	};

if(p == NULL)
	return(NULL);

if( (tag = p->headblock.tag) == TNAME)
	return(p);

e = cpblock( blksize[p->headblock.tag] , p);

switch(tag)
	{
	case TCONST:
		if(e->constblock.vtype == TYCHAR)
			{
			e->constblock.const.ccp =
				copyn(1+strlen(e->constblock.const.ccp),
					e->constblock.const.ccp);
			e->constblock.vleng = cpexpr(e->constblock.vleng);
			}
	case TERROR:
		break;

	case TEXPR:
		e->exprblock.leftp = cpexpr(p->exprblock.leftp);
		e->exprblock.rightp = cpexpr(p->exprblock.rightp);
		break;

	case TLIST:
		if(pp = p->listblock.listp)
			{
			ep = e->listblock.listp = mkchain( cpexpr(pp->datap), NULL);
			for(pp = pp->nextp ; pp ; pp = pp->nextp)
				ep = ep->nextp = mkchain( cpexpr(pp->datap), NULL);
			}
		break;

	case TADDR:
		e->addrblock.vleng = cpexpr(e->addrblock.vleng);
		e->addrblock.memoffset = cpexpr(e->addrblock.memoffset);
		e->addrblock.istemp = NO;
		break;

	case TPRIM:
		e->primblock.argsp = cpexpr(e->primblock.argsp);
		e->primblock.fcharp = cpexpr(e->primblock.fcharp);
		e->primblock.lcharp = cpexpr(e->primblock.lcharp);
		break;

	default:
		fatali("cpexpr: impossible tag %d", tag);
	}

return(e);
}

frexpr(p)
register tagptr p;
{
register chainp q;

if(p == NULL)
	return;

switch(p->headblock.tag)
	{
	case TCONST:
		if( ISCHAR(p) )
			{
			free(p->constblock.const.ccp);
			frexpr(p->constblock.vleng);
			}
		break;

	case TADDR:
		if(p->addrblock.istemp)
			{
			frtemp(p);
			return;
			}
		frexpr(p->addrblock.vleng);
		frexpr(p->addrblock.memoffset);
		break;

	case TERROR:
		break;

	case TNAME:
		return;

	case TPRIM:
		frexpr(p->primblock.argsp);
		frexpr(p->primblock.fcharp);
		frexpr(p->primblock.lcharp);
		break;

	case TEXPR:
		frexpr(p->exprblock.leftp);
		if(p->exprblock.rightp)
			frexpr(p->exprblock.rightp);
		break;

	case TLIST:
		for(q = p->listblock.listp ; q ; q = q->nextp)
			frexpr(q->datap);
		frchain( &(p->listblock.listp) );
		break;

	default:
		fatali("frexpr: impossible tag %d", p->headblock.tag);
	}

free(p);
}

/* fix up types in expression; replace subtrees and convert
   names to address blocks */

expptr fixtype(p)
register tagptr p;
{

if(p == 0)
	return(0);

switch(p->headblock.tag)
	{
	case TCONST:
		if( ! ONEOF(p->constblock.vtype,MSKINT|MSKLOGICAL|MSKADDR) )
			p = putconst(p);
		return(p);

	case TADDR:
		p->addrblock.memoffset = fixtype(p->addrblock.memoffset);
		return(p);

	case TERROR:
		return(p);

	default:
		fatali("fixtype: impossible tag %d", p->headblock.tag);

	case TEXPR:
		return( fixexpr(p) );

	case TLIST:
		return( p );

	case TPRIM:
		if(p->primblock.argsp && p->primblock.namep->vclass!=CLVAR)
			return( mkfunct(p) );
		else	return( mklhs(p) );
	}
}





/* special case tree transformations and cleanups of expression trees */

expptr fixexpr(p)
register struct Exprblock *p;
{
expptr lp;
register expptr rp;
register expptr q;
int opcode, ltype, rtype, ptype, mtype;
expptr mkpower();

if(p->tag == TERROR)
	return(p);
else if(p->tag != TEXPR)
	fatali("fixexpr: invalid tag %d", p->tag);
opcode = p->opcode;
lp = p->leftp = fixtype(p->leftp);
ltype = lp->headblock.vtype;
if(opcode==OPASSIGN && lp->headblock.tag!=TADDR)
	{
	err("left side of assignment must be variable");
	frexpr(p);
	return( errnode() );
	}

if(p->rightp)
	{
	rp = p->rightp = fixtype(p->rightp);
	rtype = rp->headblock.vtype;
	}
else
	{
	rp = NULL;
	rtype = 0;
	}

/* force folding if possible */
if( ISCONST(lp) && (rp==NULL || ISCONST(rp)) )
	{
	q = mkexpr(opcode, lp, rp);
	if( ISCONST(q) )
		return(q);
	free(q);	/* constants did not fold */
	}

if( (ptype = cktype(opcode, ltype, rtype)) == TYERROR)
	{
	frexpr(p);
	return( errnode() );
	}

switch(opcode)
	{
	case OPCONCAT:
		if(p->vleng == NULL)
			p->vleng = mkexpr(OPPLUS,
				cpexpr(lp->headblock.vleng),
				cpexpr(rp->headblock.vleng) );
		break;

	case OPASSIGN:
	case OPPLUSEQ:
	case OPSTAREQ:
		if(ltype == rtype)
			break;
		if( ! ISCONST(rp) && ISREAL(ltype) && ISREAL(rtype) )
			break;
		if( ISCOMPLEX(ltype) || ISCOMPLEX(rtype) )
			break;
		if( ONEOF(ltype, MSKADDR|MSKINT) && ONEOF(rtype, MSKADDR|MSKINT)
#if FAMILY==PCC
		    && typesize[ltype]>=typesize[rtype] )
#else
		    && typesize[ltype]==typesize[rtype] )
#endif
			break;
		p->rightp = fixtype( mkconv(ptype, rp) );
		break;

	case OPSLASH:
		if( ISCOMPLEX(rtype) )
			{
			p = call2(ptype, ptype==TYCOMPLEX? "c_div" : "z_div",
				mkconv(ptype, lp), mkconv(ptype, rp) );
			break;
			}
	case OPPLUS:
	case OPMINUS:
	case OPSTAR:
	case OPMOD:
		if(ptype==TYDREAL && ( (ltype==TYREAL && ! ISCONST(lp) ) ||
		    (rtype==TYREAL && ! ISCONST(rp) ) ))
			break;
		if( ISCOMPLEX(ptype) )
			break;
		if(ltype != ptype)
			p->leftp = fixtype(mkconv(ptype,lp));
		if(rtype != ptype)
			p->rightp = fixtype(mkconv(ptype,rp));
		break;

	case OPPOWER:
		return( mkpower(p) );

	case OPLT:
	case OPLE:
	case OPGT:
	case OPGE:
	case OPEQ:
	case OPNE:
		if(ltype == rtype)
			break;
		mtype = cktype(OPMINUS, ltype, rtype);
		if(mtype==TYDREAL && ( (ltype==TYREAL && ! ISCONST(lp)) ||
		    (rtype==TYREAL && ! ISCONST(rp)) ))
			break;
		if( ISCOMPLEX(mtype) )
			break;
		if(ltype != mtype)
			p->leftp = fixtype(mkconv(mtype,lp));
		if(rtype != mtype)
			p->rightp = fixtype(mkconv(mtype,rp));
		break;


	case OPCONV:
		ptype = cktype(OPCONV, p->vtype, ltype);
		if(lp->headblock.tag==TEXPR && lp->exprblock.opcode==OPCOMMA)
			{
			lp->exprblock.rightp = fixtype( mkconv(ptype, lp->exprblock.rightp) );
			free(p);
			p = lp;
			}
		break;

	case OPADDR:
		if(lp->headblock.tag==TEXPR && lp->exprblock.opcode==OPADDR)
			fatal("addr of addr");
		break;

	case OPCOMMA:
	case OPQUEST:
	case OPCOLON:
		break;

	case OPMIN:
	case OPMAX:
		ptype = p->vtype;
		break;

	default:
		break;
	}

p->vtype = ptype;
return(p);
}

#if SZINT < SZLONG
/*
   for efficient subscripting, replace long ints by shorts
   in easy places
*/

expptr shorten(p)
register expptr p;
{
register expptr q;

if(p->headblock.vtype != TYLONG)
	return(p);

switch(p->headblock.tag)
	{
	case TERROR:
	case TLIST:
		return(p);

	case TCONST:
	case TADDR:
		return( mkconv(TYINT,p) );

	case TEXPR:
		break;

	default:
		fatali("shorten: invalid tag %d", p->headblock.tag);
	}

switch(p->exprblock.opcode)
	{
	case OPPLUS:
	case OPMINUS:
	case OPSTAR:
		q = shorten( cpexpr(p->exprblock.rightp) );
		if(q->headblock.vtype == TYINT)
			{
			p->exprblock.leftp = shorten(p->exprblock.leftp);
			if(p->exprblock.leftp->headblock.vtype == TYLONG)
				frexpr(q);
			else
				{
				frexpr(p->exprblock.rightp);
				p->exprblock.rightp = q;
				p->exprblock.vtype = TYINT;
				}
			}
		break;

	case OPNEG:
		p->exprblock.leftp = shorten(p->exprblock.leftp);
		if(p->exprblock.leftp->headblock.vtype == TYINT)
			p->exprblock.vtype = TYINT;
		break;

	case OPCALL:
	case OPCCALL:
		p = mkconv(TYINT,p);
		break;
	default:
		break;
	}

return(p);
}
#endif

fixargs(doput, p0)
int doput;
struct Listblock *p0;
{
register chainp p;
register tagptr q, t;
register int qtag;
int nargs;
struct Addrblock *mkscalar();

nargs = 0;
if(p0)
    for(p = p0->listp ; p ; p = p->nextp)
	{
	++nargs;
	q = p->datap;
	qtag = q->headblock.tag;
	if(qtag == TCONST)
		{
		if(q->constblock.vtype == TYSHORT)
			q = mkconv(tyint, q);
		if(doput)
			p->datap = putconst(q);
		else
			p->datap = q;
		}
	else if(qtag==TPRIM && q->primblock.argsp==0 && q->primblock.namep->vclass==CLPROC)
		p->datap = mkaddr(q->primblock.namep);
	else if(qtag==TPRIM && q->primblock.argsp==0 && q->primblock.namep->vdim!=NULL)
		p->datap = mkscalar(q->primblock.namep);
	else if(qtag==TPRIM && q->primblock.argsp==0 && q->primblock.namep->vdovar && 
		(t = memversion(q->primblock.namep)) )
			p->datap = fixtype(t);
	else	p->datap = fixtype(q);
	}
return(nargs);
}


struct Addrblock *mkscalar(np)
register struct Nameblock *np;
{
register struct Addrblock *ap;
register struct Dimblock *dp;

vardcl(np);
ap = mkaddr(np);

#if TARGET == VAX
	/* on the VAX, prolog causes array arguments
	   to point at the (0,...,0) element, except when
	   subscript checking is on
	*/
	if( !checksubs && np->vstg==STGARG)
		{
		dp = np->vdim;
		frexpr(ap->memoffset);
		ap->memoffset = mkexpr(OPSTAR,
				(np->vtype==TYCHAR ?
					cpexpr(np->vleng) :
					(tagptr)ICON(typesize[np->vtype]) ),
				cpexpr(dp->baseoffset) );
		}
#endif
return(ap);
}





expptr mkfunct(p)
register struct Primblock * p;
{
struct Entrypoint *ep;
struct Addrblock *ap;
struct Extsym *extp;
register struct Nameblock *np;
register struct Exprblock *q;
struct Exprblock *intrcall(), *stfcall();
int k, nargs;
int class;

np = p->namep;
class = np->vclass;

if(class == CLUNKNOWN)
	{
	np->vclass = class = CLPROC;
	if(np->vstg == STGUNKNOWN)
		{
		if(k = intrfunct(np->varname))
			{
			np->vstg = STGINTR;
			np->vardesc.varno = k;
			np->vprocclass = PINTRINSIC;
			}
		else
			{
			extp = mkext( varunder(VL,np->varname) );
			extp->extstg = STGEXT;
			np->vstg = STGEXT;
			np->vardesc.varno = extp - extsymtab;
			np->vprocclass = PEXTERNAL;
			}
		}
	else if(np->vstg==STGARG)
		{
		if(np->vtype!=TYCHAR && !ftn66flag)
		    warn("Dummy procedure not declared EXTERNAL. Code may be wrong.");
		np->vprocclass = PEXTERNAL;
		}
	}

if(class != CLPROC)
	fatali("invalid class code %d for function", class);
if(p->fcharp || p->lcharp)
	{
	err("no substring of function call");
	goto error;
	}
impldcl(np);
nargs = fixargs( np->vprocclass!=PINTRINSIC,  p->argsp);

switch(np->vprocclass)
	{
	case PEXTERNAL:
		ap = mkaddr(np);
	call:
		q = mkexpr(OPCALL, ap, p->argsp);
		if( (q->vtype = np->vtype) == TYUNKNOWN)
			{
			err("attempt to use untyped function");
			goto error;
			}
		if(np->vleng)
			q->vleng = cpexpr(np->vleng);
		break;

	case PINTRINSIC:
		q = intrcall(np, p->argsp, nargs);
		break;

	case PSTFUNCT:
		q = stfcall(np, p->argsp);
		break;

	case PTHISPROC:
		warn("recursive call");
		for(ep = entries ; ep ; ep = ep->nextp)
			if(ep->enamep == np)
				break;
		if(ep == NULL)
			fatal("mkfunct: impossible recursion");
		ap = builtin(np->vtype, varstr(XL, ep->entryname->extname) );
		goto call;

	default:
		fatali("mkfunct: impossible vprocclass %d", np->vprocclass);
	}
free(p);
return(q);

error:
	frexpr(p);
	return( errnode() );
}



LOCAL struct Exprblock *stfcall(np, actlist)
struct Nameblock *np;
struct Listblock *actlist;
{
register chainp actuals;
int nargs;
chainp oactp, formals;
int type;
struct Exprblock *q, *rhs;
expptr ap;
register struct Rplblock *rp;
struct Rplblock *tlist;

if(actlist)
	{
	actuals = actlist->listp;
	free(actlist);
	}
else
	actuals = NULL;
oactp = actuals;

nargs = 0;
tlist = NULL;
if( (type = np->vtype) == TYUNKNOWN)
	{
	err("attempt to use untyped statement function");
	q = errnode();
	goto ret;
	}
formals = np->vardesc.vstfdesc->datap;
rhs = np->vardesc.vstfdesc->nextp;

/* copy actual arguments into temporaries */
while(actuals!=NULL && formals!=NULL)
	{
	rp = ALLOC(Rplblock);
	rp->rplnp = q = formals->datap;
	ap = fixtype(actuals->datap);
	if(q->vtype==ap->headblock.vtype && q->vtype!=TYCHAR
	   && (ap->headblock.tag==TCONST || ap->headblock.tag==TADDR) )
		{
		rp->rplvp = ap;
		rp->rplxp = NULL;
		rp->rpltag = ap->headblock.tag;
		}
	else	{
		rp->rplvp = mktemp(q->vtype, q->vleng);
		rp->rplxp = fixtype( mkexpr(OPASSIGN, cpexpr(rp->rplvp), ap) );
		if( (rp->rpltag = rp->rplxp->tag) == TERROR)
			err("disagreement of argument types in statement function call");
		}
	rp->nextp = tlist;
	tlist = rp;
	actuals = actuals->nextp;
	formals = formals->nextp;
	++nargs;
	}

if(actuals!=NULL || formals!=NULL)
	err("statement function definition and argument list differ");

/*
   now push down names involved in formal argument list, then
   evaluate rhs of statement function definition in this environment
*/
rpllist = hookup(tlist, rpllist);
q = mkconv(type, fixtype(cpexpr(rhs)) );

/* now generate the tree ( t1=a1, (t2=a2,... , f))))) */
while(--nargs >= 0)
	{
	if(rpllist->rplxp)
		q = mkexpr(OPCOMMA, rpllist->rplxp, q);
	rp = rpllist->nextp;
	frexpr(rpllist->rplvp);
	free(rpllist);
	rpllist = rp;
	}

ret:
	frchain( &oactp );
	return(q);
}




struct Addrblock *mklhs(p)
register struct Primblock * p;
{
register struct Addrblock *s;
expptr suboffset();
struct Nameblock *np;
register struct Rplblock *rp;
int regn;

/* first fixup name */

if(p->tag != TPRIM)
	return(p);
np = p->namep;

/* is name on the replace list? */

for(rp = rpllist ; rp ; rp = rp->nextp)
	{
	if(np == rp->rplnp)
		{
		if(rp->rpltag == TNAME)
			{
			np = p->namep = rp->rplvp;
			break;
			}
		else	return( cpexpr(rp->rplvp) );
		}
	}

/* is variable a DO index in a register ? */

if(np->vdovar && ( (regn = inregister(np)) >= 0) )
	if(np->vtype == TYERROR)
		return( errnode() );
	else
		{
		s = ALLOC(Addrblock);
		s->tag = TADDR;
		s->vstg = STGREG;
		s->vtype = TYIREG;
		s->memno = regn;
		s->memoffset = ICON(0);
		return(s);
		}

vardcl(np);
s = mkaddr(np);
s->memoffset = mkexpr(OPPLUS, s->memoffset, suboffset(p) );
frexpr(p->argsp);
p->argsp = NULL;

/* now do substring part */

if(p->fcharp || p->lcharp)
	{
	if(np->vtype != TYCHAR)
		errstr("substring of noncharacter %s", varstr(VL,np->varname));
	else	{
		if(p->lcharp == NULL)
			p->lcharp = cpexpr(s->vleng);
		if(p->fcharp)
			s->vleng = mkexpr(OPMINUS, p->lcharp,
				mkexpr(OPMINUS, p->fcharp, ICON(1) ));
		else	{
			frexpr(s->vleng);
			s->vleng = p->lcharp;
			}
		}
	}

s->vleng = fixtype( s->vleng );
s->memoffset = fixtype( s->memoffset );
free(p);
return(s);
}





deregister(np)
struct Nameblock *np;
{
if(nregvar>0 && regnamep[nregvar-1]==np)
	{
	--nregvar;
#if FAMILY == DMR
	putnreg();
#endif
	}
}




struct Addrblock *memversion(np)
register struct Nameblock *np;
{
register struct Addrblock *s;

if(np->vdovar==NO || (inregister(np)<0) )
	return(NULL);
np->vdovar = NO;
s = mklhs( mkprim(np, 0,0,0) );
np->vdovar = YES;
return(s);
}



inregister(np)
register struct Nameblock *np;
{
register int i;

for(i = 0 ; i < nregvar ; ++i)
	if(regnamep[i] == np)
		return( regnum[i] );
return(-1);
}




enregister(np)
struct Nameblock *np;
{
if( inregister(np) >= 0)
	return(YES);
if(nregvar >= maxregvar)
	return(NO);
vardcl(np);
if( ONEOF(np->vtype, MSKIREG) )
	{
	regnamep[nregvar++] = np;
	if(nregvar > highregvar)
		highregvar = nregvar;
#if FAMILY == DMR
	putnreg();
#endif
	return(YES);
	}
else
	return(NO);
}




expptr suboffset(p)
register struct Primblock *p;
{
int n;
expptr size;
chainp cp;
expptr offp, prod;
expptr subcheck();
struct Dimblock *dimp;
expptr sub[MAXDIM+1];
register struct Nameblock *np;

np = p->namep;
offp = ICON(0);
n = 0;
if(p->argsp)
	for(cp = p->argsp->listp ; cp ; cp = cp->nextp)
		{
		sub[n++] = fixtype(cpexpr(cp->datap));
		if(n > maxdim)
			{
			erri("more than %d subscripts", maxdim);
			break;
			}
		}

dimp = np->vdim;
if(n>0 && dimp==NULL)
	err("subscripts on scalar variable");
else if(dimp && dimp->ndim!=n)
	errstr("wrong number of subscripts on %s",
		varstr(VL, np->varname) );
else if(n > 0)
	{
	prod = sub[--n];
	while( --n >= 0)
		prod = mkexpr(OPPLUS, sub[n],
			mkexpr(OPSTAR, prod, cpexpr(dimp->dims[n].dimsize)) );
#if TARGET == VAX
	if(checksubs || np->vstg!=STGARG)
		prod = mkexpr(OPMINUS, prod, cpexpr(dimp->baseoffset));
#else
	prod = mkexpr(OPMINUS, prod, cpexpr(dimp->baseoffset));
#endif
	if(checksubs)
		prod = subcheck(np, prod);
	if(np->vtype == TYCHAR)
		size = cpexpr(np->vleng);
	else	size = ICON( typesize[np->vtype] );
	prod = mkexpr(OPSTAR, prod, size);
	offp = mkexpr(OPPLUS, offp, prod);
	}

if(p->fcharp && np->vtype==TYCHAR)
	offp = mkexpr(OPPLUS, offp, mkexpr(OPMINUS, cpexpr(p->fcharp), ICON(1) ));

return(offp);
}




expptr subcheck(np, p)
struct Nameblock *np;
register expptr p;
{
struct Dimblock *dimp;
expptr t, checkvar, checkcond, badcall;

dimp = np->vdim;
if(dimp->nelt == NULL)
	return(p);	/* don't check arrays with * bounds */
checkvar = NULL;
checkcond = NULL;
if( ISICON(p) )
	{
	if(p->constblock.const.ci < 0)
		goto badsub;
	if( ISICON(dimp->nelt) )
		if(p->constblock.const.ci < dimp->nelt->constblock.const.ci)
			return(p);
		else
			goto badsub;
	}
if(p->headblock.tag==TADDR && p->addrblock.vstg==STGREG)
	{
	checkvar = cpexpr(p);
	t = p;
	}
else	{
	checkvar = mktemp(p->headblock.vtype, NULL);
	t = mkexpr(OPASSIGN, cpexpr(checkvar), p);
	}
checkcond = mkexpr(OPLT, t, cpexpr(dimp->nelt) );
if( ! ISICON(p) )
	checkcond = mkexpr(OPAND, checkcond,
			mkexpr(OPLE, ICON(0), cpexpr(checkvar)) );

badcall = call4(p->headblock.vtype, "s_rnge", mkstrcon(VL, np->varname),
		mkconv(TYLONG,  cpexpr(checkvar)),
		mkstrcon(XL, procname), ICON(lineno));
badcall->exprblock.opcode = OPCCALL;
p = mkexpr(OPQUEST, checkcond,
	mkexpr(OPCOLON, checkvar, badcall));

return(p);

badsub:
	frexpr(p);
	errstr("subscript on variable %s out of range", varstr(VL,np->varname));
	return ( ICON(0) );
}




struct Addrblock *mkaddr(p)
register struct Nameblock *p;
{
struct Extsym *extp;
register struct Addrblock *t;
struct Addrblock *intraddr();

switch( p->vstg)
	{
	case STGUNKNOWN:
		if(p->vclass != CLPROC)
			break;
		extp = mkext( varunder(VL, p->varname) );
		extp->extstg = STGEXT;
		p->vstg = STGEXT;
		p->vardesc.varno = extp - extsymtab;
		p->vprocclass = PEXTERNAL;

	case STGCOMMON:
	case STGEXT:
	case STGBSS:
	case STGINIT:
	case STGEQUIV:
	case STGARG:
	case STGLENG:
	case STGAUTO:
		t = ALLOC(Addrblock);
		t->tag = TADDR;
		if(p->vclass==CLPROC && p->vprocclass==PTHISPROC)
			t->vclass = CLVAR;
		else
			t->vclass = p->vclass;
		t->vtype = p->vtype;
		t->vstg = p->vstg;
		t->memno = p->vardesc.varno;
		t->memoffset = ICON(p->voffset);
		if(p->vleng)
			t->vleng = cpexpr(p->vleng);
		return(t);

	case STGINTR:
		return( intraddr(p) );

	}
/*debug*/ fprintf(diagfile, "mkaddr. vtype=%d, vclass=%d\n", p->vtype, p->vclass);
fatali("mkaddr: impossible storage tag %d", p->vstg);
/* NOTREACHED */
}




mkarg(type, argno)
int type, argno;
{
register struct Addrblock *p;

p = ALLOC(Addrblock);
p->tag = TADDR;
p->vtype = type;
p->vclass = CLVAR;
p->vstg = (type==TYLENG ? STGLENG : STGARG);
p->memno = argno;
return(p);
}




tagptr mkprim(v, args, lstr, rstr)
register union
	{
	struct Paramblock paramblock;
	struct Nameblock nameblock;
	struct Headblock headblock;
	} *v;
struct Listblock *args;
expptr lstr, rstr;
{
register struct Primblock *p;

if(v->headblock.vclass == CLPARAM)
	{
	if(args || lstr || rstr)
		{
		errstr("no qualifiers on parameter name %s",
			varstr(VL,v->paramblock.varname));
		frexpr(args);
		frexpr(lstr);
		frexpr(rstr);
		frexpr(v);
		return( errnode() );
		}
	return( cpexpr(v->paramblock.paramval) );
	}

p = ALLOC(Primblock);
p->tag = TPRIM;
p->vtype = v->nameblock.vtype;
p->namep = v;
p->argsp = args;
p->fcharp = lstr;
p->lcharp = rstr;
return(p);
}



vardcl(v)
register struct Nameblock *v;
{
int nelt;
struct Dimblock *t;
struct Addrblock *p;
expptr neltp;

if(v->vdcldone) return;

if(v->vtype == TYUNKNOWN)
	impldcl(v);
if(v->vclass == CLUNKNOWN)
	v->vclass = CLVAR;
else if(v->vclass!=CLVAR && v->vprocclass!=PTHISPROC)
	{
	dclerr("used as variable", v);
	return;
	}
if(v->vstg==STGUNKNOWN)
	v->vstg = implstg[ letter(v->varname[0]) ];

switch(v->vstg)
	{
	case STGBSS:
		v->vardesc.varno = ++lastvarno;
		break;
	case STGAUTO:
		if(v->vclass==CLPROC && v->vprocclass==PTHISPROC)
			break;
		nelt = 1;
		if(t = v->vdim)
			if( (neltp = t->nelt) && ISCONST(neltp) )
				nelt = neltp->constblock.const.ci;
			else
				dclerr("adjustable automatic array", v);
		p = autovar(nelt, v->vtype, v->vleng);
		v->voffset = p->memoffset->constblock.const.ci;
		frexpr(p);
		break;

	default:
		break;
	}
v->vdcldone = YES;
}




impldcl(p)
register struct Nameblock *p;
{
register int k;
int type, leng;

if(p->vdcldone || (p->vclass==CLPROC && p->vprocclass==PINTRINSIC) )
	return;
if(p->vtype == TYUNKNOWN)
	{
	k = letter(p->varname[0]);
	type = impltype[ k ];
	leng = implleng[ k ];
	if(type == TYUNKNOWN)
		{
		if(p->vclass == CLPROC)
			return;
		dclerr("attempt to use undefined variable", p);
		type = TYERROR;
		leng = 1;
		}
	settype(p, type, leng);
	}
}




LOCAL letter(c)
register int c;
{
if( isupper(c) )
	c = tolower(c);
return(c - 'a');
}

#define ICONEQ(z, c)  (ISICON(z) && z->constblock.const.ci==c)
#define COMMUTE	{ e = lp;  lp = rp;  rp = e; }


expptr mkexpr(opcode, lp, rp)
int opcode;
register expptr lp, rp;
{
register struct Exprblock *e, *e1;
int etype;
int ltype, rtype;
int ltag, rtag;
expptr fold();

ltype = lp->headblock.vtype;
ltag = lp->headblock.tag;
if(rp && opcode!=OPCALL && opcode!=OPCCALL)
	{
	rtype = rp->headblock.vtype;
	rtag = rp->headblock.tag;
	}
else  rtype = 0;

etype = cktype(opcode, ltype, rtype);
if(etype == TYERROR)
	goto error;

switch(opcode)
	{
	/* check for multiplication by 0 and 1 and addition to 0 */

	case OPSTAR:
		if( ISCONST(lp) )
			COMMUTE

		if( ISICON(rp) )
			{
			if(rp->constblock.const.ci == 0)
				goto retright;
			goto mulop;
			}
		break;

	case OPSLASH:
	case OPMOD:
		if( ICONEQ(rp, 0) )
			{
			err("attempted division by zero");
			rp = ICON(1);
			break;
			}
		if(opcode == OPMOD)
			break;


	mulop:
		if( ISICON(rp) )
			{
			if(rp->constblock.const.ci == 1)
				goto retleft;

			if(rp->constblock.const.ci == -1)
				{
				frexpr(rp);
				return( mkexpr(OPNEG, lp, 0) );
				}
			}

		if( ISSTAROP(lp) && ISICON(lp->exprblock.rightp) )
			{
			if(opcode == OPSTAR)
				e = mkexpr(OPSTAR, lp->exprblock.rightp, rp);
			else  if(ISICON(rp) &&
				(lp->exprblock.rightp->constblock.const.ci %
					rp->constblock.const.ci) == 0)
				e = mkexpr(OPSLASH, lp->exprblock.rightp, rp);
			else	break;

			e1 = lp->exprblock.leftp;
			free(lp);
			return( mkexpr(OPSTAR, e1, e) );
			}
		break;


	case OPPLUS:
		if( ISCONST(lp) )
			COMMUTE
		goto addop;

	case OPMINUS:
		if( ICONEQ(lp, 0) )
			{
			frexpr(lp);
			return( mkexpr(OPNEG, rp, 0) );
			}

		if( ISCONST(rp) )
			{
			opcode = OPPLUS;
			consnegop(rp);
			}

	addop:
		if( ISICON(rp) )
			{
			if(rp->constblock.const.ci == 0)
				goto retleft;
			if( ISPLUSOP(lp) && ISICON(lp->exprblock.rightp) )
				{
				e = mkexpr(OPPLUS, lp->exprblock.rightp, rp);
				e1 = lp->exprblock.leftp;
				free(lp);
				return( mkexpr(OPPLUS, e1, e) );
				}
			}
		break;


	case OPPOWER:
		break;

	case OPNEG:
		if(ltag==TEXPR && lp->exprblock.opcode==OPNEG)
			{
			e = lp->exprblock.leftp;
			free(lp);
			return(e);
			}
		break;

	case OPNOT:
		if(ltag==TEXPR && lp->exprblock.opcode==OPNOT)
			{
			e = lp->exprblock.leftp;
			free(lp);
			return(e);
			}
		break;

	case OPCALL:
	case OPCCALL:
		etype = ltype;
		if(rp!=NULL && rp->listblock.listp==NULL)
			{
			free(rp);
			rp = NULL;
			}
		break;

	case OPAND:
	case OPOR:
		if( ISCONST(lp) )
			COMMUTE

		if( ISCONST(rp) )
			{
			if(rp->constblock.const.ci == 0)
				if(opcode == OPOR)
					goto retleft;
				else
					goto retright;
			else if(opcode == OPOR)
				goto retright;
			else
				goto retleft;
			}
	case OPEQV:
	case OPNEQV:

	case OPBITAND:
	case OPBITOR:
	case OPBITXOR:
	case OPBITNOT:
	case OPLSHIFT:
	case OPRSHIFT:

	case OPLT:
	case OPGT:
	case OPLE:
	case OPGE:
	case OPEQ:
	case OPNE:

	case OPCONCAT:
		break;
	case OPMIN:
	case OPMAX:

	case OPASSIGN:
	case OPPLUSEQ:
	case OPSTAREQ:

	case OPCONV:
	case OPADDR:

	case OPCOMMA:
	case OPQUEST:
	case OPCOLON:
		break;

	default:
		fatali("mkexpr: impossible opcode %d", opcode);
	}

e = ALLOC(Exprblock);
e->tag = TEXPR;
e->opcode = opcode;
e->vtype = etype;
e->leftp = lp;
e->rightp = rp;
if(ltag==TCONST && (rp==0 || rtag==TCONST) )
	e = fold(e);
return(e);

retleft:
	frexpr(rp);
	return(lp);

retright:
	frexpr(lp);
	return(rp);

error:
	frexpr(lp);
	if(rp && opcode!=OPCALL && opcode!=OPCCALL)
		frexpr(rp);
	return( errnode() );
}

#define ERR(s)   { errs = s; goto error; }

cktype(op, lt, rt)
register int op, lt, rt;
{
char *errs;

if(lt==TYERROR || rt==TYERROR)
	goto error1;

if(lt==TYUNKNOWN)
	return(TYUNKNOWN);
if(rt==TYUNKNOWN)
	if(op!=OPNOT && op!=OPBITNOT && op!=OPNEG && op!=OPCALL && op!=OPCCALL && op!=OPADDR)
		return(TYUNKNOWN);

switch(op)
	{
	case OPPLUS:
	case OPMINUS:
	case OPSTAR:
	case OPSLASH:
	case OPPOWER:
	case OPMOD:
		if( ISNUMERIC(lt) && ISNUMERIC(rt) )
			return( maxtype(lt, rt) );
		ERR("nonarithmetic operand of arithmetic operator")

	case OPNEG:
		if( ISNUMERIC(lt) )
			return(lt);
		ERR("nonarithmetic operand of negation")

	case OPNOT:
		if(lt == TYLOGICAL)
			return(TYLOGICAL);
		ERR("NOT of nonlogical")

	case OPAND:
	case OPOR:
	case OPEQV:
	case OPNEQV:
		if(lt==TYLOGICAL && rt==TYLOGICAL)
			return(TYLOGICAL);
		ERR("nonlogical operand of logical operator")

	case OPLT:
	case OPGT:
	case OPLE:
	case OPGE:
	case OPEQ:
	case OPNE:
		if(lt==TYCHAR || rt==TYCHAR || lt==TYLOGICAL || rt==TYLOGICAL)
			{
			if(lt != rt)
				ERR("illegal comparison")
			}

		else if( ISCOMPLEX(lt) || ISCOMPLEX(rt) )
			{
			if(op!=OPEQ && op!=OPNE)
				ERR("order comparison of complex data")
			}

		else if( ! ISNUMERIC(lt) || ! ISNUMERIC(rt) )
			ERR("comparison of nonarithmetic data")
		return(TYLOGICAL);

	case OPCONCAT:
		if(lt==TYCHAR && rt==TYCHAR)
			return(TYCHAR);
		ERR("concatenation of nonchar data")

	case OPCALL:
	case OPCCALL:
		return(lt);

	case OPADDR:
		return(TYADDR);

	case OPCONV:
		if(rt == 0)
			return(0);
		if(lt==TYCHAR && ISINT(rt) )
			return(TYCHAR);
	case OPASSIGN:
	case OPPLUSEQ:
	case OPSTAREQ:
		if( ISINT(lt) && rt==TYCHAR)
			return(lt);
		if(lt==TYCHAR || rt==TYCHAR || lt==TYLOGICAL || rt==TYLOGICAL)
			if(op!=OPASSIGN || lt!=rt)
				{
/* debug fprintf(diagfile, " lt=%d, rt=%d, op=%d\n", lt, rt, op); */
/* debug fatal("impossible conversion.  possible compiler bug"); */
				ERR("impossible conversion")
				}
		return(lt);

	case OPMIN:
	case OPMAX:
	case OPBITOR:
	case OPBITAND:
	case OPBITXOR:
	case OPBITNOT:
	case OPLSHIFT:
	case OPRSHIFT:
		return(lt);

	case OPCOMMA:
	case OPQUEST:
	case OPCOLON:
		return(rt);

	default:
		fatali("cktype: impossible opcode %d", op);
	}
error:	err(errs);
error1:	return(TYERROR);
}

LOCAL expptr fold(e)
register struct Exprblock *e;
{
struct Constblock *p;
#ifdef VERSION6
	expptr lp, rp;
#else
	register expptr lp, rp;
#endif
int etype, mtype, ltype, rtype, opcode;
int i, ll, lr;
char *q, *s;
union Constant lcon, rcon;

opcode = e->opcode;
etype = e->vtype;

lp = e->leftp;
ltype = lp->headblock.vtype;
rp = e->rightp;

if(rp == 0)
	switch(opcode)
		{
		case OPNOT:
			lp->constblock.const.ci = ! lp->constblock.const.ci;
			return(lp);

		case OPBITNOT:
			lp->constblock.const.ci = ~ lp->constblock.const.ci;
			return(lp);

		case OPNEG:
			consnegop(lp);
			return(lp);

		case OPCONV:
		case OPADDR:
			return(e);

		default:
			fatali("fold: invalid unary operator %d", opcode);
		}

rtype = rp->headblock.vtype;

p = ALLOC(Constblock);
p->tag = TCONST;
p->vtype = etype;
p->vleng = e->vleng;

switch(opcode)
	{
	case OPCOMMA:
	case OPQUEST:
	case OPCOLON:
		return(e);

	case OPAND:
		p->const.ci = lp->constblock.const.ci &&
				rp->constblock.const.ci;
		break;

	case OPOR:
		p->const.ci = lp->constblock.const.ci ||
				rp->constblock.const.ci;
		break;

	case OPEQV:
		p->const.ci = lp->constblock.const.ci ==
				rp->constblock.const.ci;
		break;

	case OPNEQV:
		p->const.ci = lp->constblock.const.ci !=
				rp->constblock.const.ci;
		break;

	case OPBITAND:
		p->const.ci = lp->constblock.const.ci &
				rp->constblock.const.ci;
		break;

	case OPBITOR:
		p->const.ci = lp->constblock.const.ci |
				rp->constblock.const.ci;
		break;

	case OPBITXOR:
		p->const.ci = lp->constblock.const.ci ^
				rp->constblock.const.ci;
		break;

	case OPLSHIFT:
		p->const.ci = lp->constblock.const.ci <<
				rp->constblock.const.ci;
		break;

	case OPRSHIFT:
		p->const.ci = lp->constblock.const.ci >>
				rp->constblock.const.ci;
		break;

	case OPCONCAT:
		ll = lp->constblock.vleng->constblock.const.ci;
		lr = rp->constblock.vleng->constblock.const.ci;
		p->const.ccp = q = (char *) ckalloc(ll+lr);
		p->vleng = ICON(ll+lr);
		s = lp->constblock.const.ccp;
		for(i = 0 ; i < ll ; ++i)
			*q++ = *s++;
		s = rp->constblock.const.ccp;
		for(i = 0; i < lr; ++i)
			*q++ = *s++;
		break;


	case OPPOWER:
		if( ! ISINT(rtype) )
			return(e);
		conspower(&(p->const), lp, rp->constblock.const.ci);
		break;


	default:
		if(ltype == TYCHAR)
			{
			lcon.ci = cmpstr(lp->constblock.const.ccp,
					rp->constblock.const.ccp,
					lp->constblock.vleng->constblock.const.ci,
					rp->constblock.vleng->constblock.const.ci);
			rcon.ci = 0;
			mtype = tyint;
			}
		else	{
			mtype = maxtype(ltype, rtype);
			consconv(mtype, &lcon, ltype, &(lp->constblock.const) );
			consconv(mtype, &rcon, rtype, &(rp->constblock.const) );
			}
		consbinop(opcode, mtype, &(p->const), &lcon, &rcon);
		break;
	}

frexpr(e);
return(p);
}



/* assign constant l = r , doing coercion */

consconv(lt, lv, rt, rv)
int lt, rt;
register union Constant *lv, *rv;
{
switch(lt)
	{
	case TYCHAR:
		*(lv->ccp = ckalloc(1)) = rv->ci;
		break;

	case TYSHORT:
	case TYLONG:
		if(rt == TYCHAR)
			lv->ci = rv->ccp[0];
		else if( ISINT(rt) )
			lv->ci = rv->ci;
		else	lv->ci = rv->cd[0];
		break;

	case TYCOMPLEX:
	case TYDCOMPLEX:
		switch(rt)
			{
			case TYSHORT:
			case TYLONG:
				/* fall through and do real assignment of
				   first element
				*/
			case TYREAL:
			case TYDREAL:
				lv->cd[1] = 0; break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				lv->cd[1] = rv->cd[1]; break;
			}

	case TYREAL:
	case TYDREAL:
		if( ISINT(rt) )
			lv->cd[0] = rv->ci;
		else	lv->cd[0] = rv->cd[0];
		break;

	case TYLOGICAL:
		lv->ci = rv->ci;
		break;
	}
}



consnegop(p)
register struct Constblock *p;
{
switch(p->vtype)
	{
	case TYSHORT:
	case TYLONG:
		p->const.ci = - p->const.ci;
		break;

	case TYCOMPLEX:
	case TYDCOMPLEX:
		p->const.cd[1] = - p->const.cd[1];
		/* fall through and do the real parts */
	case TYREAL:
	case TYDREAL:
		p->const.cd[0] = - p->const.cd[0];
		break;
	default:
		fatali("consnegop: impossible type %d", p->vtype);
	}
}



LOCAL conspower(powp, ap, n)
register union Constant *powp;
struct Constblock *ap;
ftnint n;
{
register int type;
union Constant x;

switch(type = ap->vtype)	/* pow = 1 */ 
	{
	case TYSHORT:
	case TYLONG:
		powp->ci = 1;
		break;
	case TYCOMPLEX:
	case TYDCOMPLEX:
		powp->cd[1] = 0;
	case TYREAL:
	case TYDREAL:
		powp->cd[0] = 1;
		break;
	default:
		fatali("conspower: invalid type %d", type);
	}

if(n == 0)
	return;
if(n < 0)
	{
	if( ISINT(type) )
		{
		err("integer ** negative power ");
		return;
		}
	n = - n;
	consbinop(OPSLASH, type, &x, powp, &(ap->const));
	}
else
	consbinop(OPSTAR, type, &x, powp, &(ap->const));

for( ; ; )
	{
	if(n & 01)
		consbinop(OPSTAR, type, powp, powp, &x);
	if(n >>= 1)
		consbinop(OPSTAR, type, &x, &x, &x);
	else
		break;
	}
}



/* do constant operation cp = a op b */


LOCAL consbinop(opcode, type, cp, ap, bp)
int opcode, type;
register union Constant *ap, *bp, *cp;
{
int k;
double temp;

switch(opcode)
	{
	case OPPLUS:
		switch(type)
			{
			case TYSHORT:
			case TYLONG:
				cp->ci = ap->ci + bp->ci;
				break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				cp->cd[1] = ap->cd[1] + bp->cd[1];
			case TYREAL:
			case TYDREAL:
				cp->cd[0] = ap->cd[0] + bp->cd[0];
				break;
			}
		break;

	case OPMINUS:
		switch(type)
			{
			case TYSHORT:
			case TYLONG:
				cp->ci = ap->ci - bp->ci;
				break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				cp->cd[1] = ap->cd[1] - bp->cd[1];
			case TYREAL:
			case TYDREAL:
				cp->cd[0] = ap->cd[0] - bp->cd[0];
				break;
			}
		break;

	case OPSTAR:
		switch(type)
			{
			case TYSHORT:
			case TYLONG:
				cp->ci = ap->ci * bp->ci;
				break;
			case TYREAL:
			case TYDREAL:
				cp->cd[0] = ap->cd[0] * bp->cd[0];
				break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				temp = ap->cd[0] * bp->cd[0] -
					    ap->cd[1] * bp->cd[1] ;
				cp->cd[1] = ap->cd[0] * bp->cd[1] +
					    ap->cd[1] * bp->cd[0] ;
				cp->cd[0] = temp;
				break;
			}
		break;
	case OPSLASH:
		switch(type)
			{
			case TYSHORT:
			case TYLONG:
				cp->ci = ap->ci / bp->ci;
				break;
			case TYREAL:
			case TYDREAL:
				cp->cd[0] = ap->cd[0] / bp->cd[0];
				break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				zdiv(cp,ap,bp);
				break;
			}
		break;

	case OPMOD:
		if( ISINT(type) )
			{
			cp->ci = ap->ci % bp->ci;
			break;
			}
		else
			fatal("inline mod of noninteger");

	default:	  /* relational ops */
		switch(type)
			{
			case TYSHORT:
			case TYLONG:
				if(ap->ci < bp->ci)
					k = -1;
				else if(ap->ci == bp->ci)
					k = 0;
				else	k = 1;
				break;
			case TYREAL:
			case TYDREAL:
				if(ap->cd[0] < bp->cd[0])
					k = -1;
				else if(ap->cd[0] == bp->cd[0])
					k = 0;
				else	k = 1;
				break;
			case TYCOMPLEX:
			case TYDCOMPLEX:
				if(ap->cd[0] == bp->cd[0] &&
				   ap->cd[1] == bp->cd[1] )
					k = 0;
				else	k = 1;
				break;
			}

		switch(opcode)
			{
			case OPEQ:
				cp->ci = (k == 0);
				break;
			case OPNE:
				cp->ci = (k != 0);
				break;
			case OPGT:
				cp->ci = (k == 1);
				break;
			case OPLT:
				cp->ci = (k == -1);
				break;
			case OPGE:
				cp->ci = (k >= 0);
				break;
			case OPLE:
				cp->ci = (k <= 0);
				break;
			}
		break;
	}
}




conssgn(p)
register expptr p;
{
if( ! ISCONST(p) )
	fatal( "sgn(nonconstant)" );

switch(p->headblock.vtype)
	{
	case TYSHORT:
	case TYLONG:
		if(p->constblock.const.ci > 0) return(1);
		if(p->constblock.const.ci < 0) return(-1);
		return(0);

	case TYREAL:
	case TYDREAL:
		if(p->constblock.const.cd[0] > 0) return(1);
		if(p->constblock.const.cd[0] < 0) return(-1);
		return(0);

	case TYCOMPLEX:
	case TYDCOMPLEX:
		return(p->constblock.const.cd[0]!=0 || p->constblock.const.cd[1]!=0);

	default:
		fatali( "conssgn(type %d)", p->constblock.vtype);
	}
/* NOTREACHED */
}

char *powint[ ] = { "pow_ii", "pow_ri", "pow_di", "pow_ci", "pow_zi" };


LOCAL expptr mkpower(p)
register struct Exprblock *p;
{
register expptr q, lp, rp;
int ltype, rtype, mtype;

lp = p->leftp;
rp = p->rightp;
ltype = lp->headblock.vtype;
rtype = rp->headblock.vtype;

if(ISICON(rp))
	{
	if(rp->constblock.const.ci == 0)
		{
		frexpr(p);
		if( ISINT(ltype) )
			return( ICON(1) );
		else
			return( putconst( mkconv(ltype, ICON(1))) );
		}
	if(rp->constblock.const.ci < 0)
		{
		if( ISINT(ltype) )
			{
			frexpr(p);
			err("integer**negative");
			return( errnode() );
			}
		rp->constblock.const.ci = - rp->constblock.const.ci;
		p->leftp = lp = fixexpr(mkexpr(OPSLASH, ICON(1), lp));
		}
	if(rp->constblock.const.ci == 1)
		{
		frexpr(rp);
		free(p);
		return(lp);
		}

	if( ONEOF(ltype, MSKINT|MSKREAL) )
		{
		p->vtype = ltype;
		return(p);
		}
	}
if( ISINT(rtype) )
	{
	if(ltype==TYSHORT && rtype==TYSHORT && (!ISCONST(lp) || tyint==TYSHORT) )
		q = call2(TYSHORT, "pow_hh", lp, rp);
	else	{
		if(ltype == TYSHORT)
			{
			ltype = TYLONG;
			lp = mkconv(TYLONG,lp);
			}
		q = call2(ltype, powint[ltype-TYLONG], lp, mkconv(TYLONG, rp));
		}
	}
else if( ISREAL( (mtype = maxtype(ltype,rtype)) ))
	q = call2(mtype, "pow_dd",
		mkconv(TYDREAL,lp), mkconv(TYDREAL,rp));
else	{
	q = call2(TYDCOMPLEX, "pow_zz",
		mkconv(TYDCOMPLEX,lp), mkconv(TYDCOMPLEX,rp));
	if(mtype == TYCOMPLEX)
		q = mkconv(TYCOMPLEX, q);
	}
free(p);
return(q);
}



/* Complex Division.  Same code as in Runtime Library
*/

struct dcomplex { double dreal, dimag; };


LOCAL zdiv(c, a, b)
register struct dcomplex *a, *b, *c;
{
double ratio, den;
double abr, abi;

if( (abr = b->dreal) < 0.)
	abr = - abr;
if( (abi = b->dimag) < 0.)
	abi = - abi;
if( abr <= abi )
	{
	if(abi == 0)
		fatal("complex division by zero");
	ratio = b->dreal / b->dimag ;
	den = b->dimag * (1 + ratio*ratio);
	c->dreal = (a->dreal*ratio + a->dimag) / den;
	c->dimag = (a->dimag*ratio - a->dreal) / den;
	}

else
	{
	ratio = b->dimag / b->dreal ;
	den = b->dreal * (1 + ratio*ratio);
	c->dreal = (a->dreal + a->dimag*ratio) / den;
	c->dimag = (a->dimag - a->dreal*ratio) / den;
	}

}
