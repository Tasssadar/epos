//enum mclass { mc_f, mc_i, mc_t };

#define mclass FIT_IDX

/*
 *	More comments should eventually come here.  These markers are attached
 *	to units, in an unlimited number.  At present, they carry prosodic
 *	information (only intensity and pitch, not duration) relating either
 *	to the unit, or to a specified point within it.
 *
 *	This was added between 2.4 and 2.5.
 */

class marker
{
	friend class unit;

	mclass quant;
	bool extent;
	int par;
	float pos;
	marker *next;

   public:
	marker();
	~marker();
	marker(mclass mc, bool e, int p, marker *m, float po) { quant = mc; extent = e; par = p; next = m; pos = po; };
	marker *derived();
	void merge(marker *&into);
	inline bool listed(marker *ma);
	bool disjoint(marker *ma);

	bool operator < (marker &ma);
	
	int write_ssif(char *whither);

	void *operator new(size_t size);
	void operator delete(void *ptr);
};

marker::marker()
{
	next = NULL;
}

marker::~marker()
{
	if (next) delete next;
	next = NULL;
}

marker *marker::derived()
{
	if (!this) return NULL;
	marker *nm = new marker;
	nm->next = next->derived();
	nm->quant = quant;
	nm->extent = extent;
	nm->par = par;
	nm->pos = pos;
	return nm;
}

void
marker::merge(marker *&into)
{
	if (!disjoint(into)) shriek(861, "Cannot merge non-disjoints");
	if (!this)
		return;
	if (!into) {
		into = this;
		return;
	}
	if (*into < *this) {
		if (!into->next) into->next = this;
		else merge(into->next);
	} else {
		if (quant == into->quant && pos == into->pos && extent && into->extent) {
			into->par += par;		// merge compatible extent markers
			marker *nm = next;
			next = NULL;
			delete this;
			nm->merge(into);
		} else {
			marker *other = into;
			into = this;
			other->merge(next);
		}
	}
}

inline bool
marker::listed(marker *ma)
{
	if (!ma) return false;
	return this == ma || listed(ma->next);
}

bool
marker::disjoint(marker *ma)
{
	if (!this || !ma) return true;
	if (this == ma) return false;
	return !listed(ma->next) && next->disjoint(ma);
}

bool
marker::operator < (marker &ma)
{
	if (extent == ma.extent) return pos < ma.pos;
	else return !extent;
}


int
marker::write_ssif(char *whither)
{
	if (quant == Q_FREQ) return sprintf(whither, "(%d,%d) ", (int)(pos * 100),
		this_voice->init_f + par /*, extent?"yes":"no" */ );
	else return 0;
}
