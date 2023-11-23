#ifndef __PARAMLIST_H__
#define __PARAMLIST_H__

#include <cstdio>
#include <stdlib.h>
#include <string.h>

class paramlistelm {
public:
	paramlistelm *next;

	char left,right;
	float lower,upper,gain,gain2;
	int sortindex;

	paramlistelm(void) {
		left = right = 1;
		lower = upper = gain = 0;
		next = nullptr;
	};

	~paramlistelm() {
		delete next;
		next = nullptr;
	};

	char *getString(void) {
		static char str[64];
		sprintf(str,"%gHz to %gHz, %gdB %c%c",
			(double)lower,(double)upper,(double)gain,left?'L':' ',right?'R':' ');
		return str;
	}
};

class paramlist {
public:
	paramlistelm *elm;

	paramlist(void) {
		elm = nullptr;
	}

	~paramlist() {
		delete elm;
		elm = nullptr;
	}

	void copy(paramlist &src)
	{
		delete elm;
		elm = nullptr;

		paramlistelm **p,*q;
		for(p=&elm,q=src.elm;q != nullptr;q = q->next,p = &(*p)->next)
		{
			*p = new paramlistelm;
			(*p)->left  = q->left;
			(*p)->right = q->right;
			(*p)->lower = q->lower;
			(*p)->upper = q->upper;
			(*p)->gain  = q->gain;
		}
	}

	paramlistelm *newelm(void)
	{
		paramlistelm **e;
		for(e = &elm;*e != nullptr;e = &(*e)->next) ;
		*e = new paramlistelm;

		return *e;
	}

	int getnelm(void)
	{
		int i;
		paramlistelm *e;

		for(e = elm,i = 0;e != nullptr;e = e->next,i++) ;

		return i;
	}

	void delelm(paramlistelm *p)
	{
		paramlistelm **e;
		for(e = &elm;*e != nullptr && p != *e;e = &(*e)->next) ;
		if (*e == nullptr) return;
		*e = (*e)->next;
		p->next = nullptr;
		delete p;
	}

	void sortelm(void)
	{
		int i=0;

		if (elm == nullptr) return;

		for(paramlistelm *r = elm;r	!= nullptr;r = r->next) r->sortindex = i++;

		paramlistelm **p,**q;

		for(p=&elm->next;*p != nullptr;)
		{
			for(q=&elm;*q != *p;q = &(*q)->next)
				if ((*p)->lower < (*q)->lower ||
					((*p)->lower == (*q)->lower && (*p)->sortindex < (*q)->sortindex)) break;

			if (p == q) {p = &(*p)->next; continue;}

			paramlistelm **pn = p;
			paramlistelm *pp = *p;
			*p = (*p)->next;
			pp->next = *q;
			*q = pp;

			p = pn;
	    }
	}
};
#endif // __PARAMLIST_H__
