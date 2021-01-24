#ifndef _CSTRING
#define _CSTRING

typedef struct cstring cstring;
typedef struct cstringList cstringList;

struct cstringList
{
	cstring* data;
	cstringList *next;
	cstringList *back;
	void (*listAdd)(cstringList **root, char* key);
	int (*listCount)(cstringList *root);
	cstringList* (*listFind)(cstringList **root, char *key);
	void (*listDeleteNode) (cstringList **root, char* key);
	void (*listDestroy)(cstringList **root);
};

struct cstring
{
	char *string;
	int (*length)(cstring *this);
	void (*new)(cstring *this, char *new_string);
	void (*slice)(cstring *this,cstring *destination,int start, int end);
	void (*addHead)(cstring *this,char *add_head);
	void (*addEnd)(cstring *this,char *add_end);
	void (*addEndNewLine)(cstring *this);
	void (*addTo)(cstring *this,int index,char *addTo);
	void (*replaceThisIndex)(cstring *this, int start, int end, char *textToReplace);
	int (*find)(cstring *this,int index,char *find);
	int (*howManyAreIn)(cstring *this,char *text);
	void (*replaceThisString)(cstring *this,char *old,char *new);
	void (*join)(cstring *this,cstring *destination,int count,...);
	void (*upper)(cstring *this);
	void (*lower)(cstring *this);
	cstring* (*parser)(cstring *this, char* parseString);
	cstringList* (*getListFromParse)(cstring *this, char* parseString);
	void (*deleteNtoN)(cstring *this, int start, int end);
	int (*isSpace)(cstring *this);
};




cstring* String();
cstringList* StringList();

#define cstring cstring*
#define cstringList cstringList*
#endif