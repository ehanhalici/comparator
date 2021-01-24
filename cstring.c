#include "cstring.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#undef cstring
#undef cstringList

cstring *String();
//cstringList

/*cstringListenin sonuna eleman ekler*/
static void listAdd(cstringList **root, char *string)
{
	if ((*root)->data == NULL)
	{
		(*root)->data = String(string);
		(*root)->next = NULL;
		(*root)->back = NULL;
		return;
	}

	cstringList *ptr = *root;

	while (ptr->next != NULL)
		ptr = ptr->next;

	ptr->next = StringList();
	ptr->next->back = ptr; 
	ptr = ptr->next;

	ptr->next = NULL;
	ptr->data = String(string);
}

/*cstringListenin eleman sayısını verir*/
static int listCount(cstringList *root)
{
	int counter = 0;
	cstringList *ptr = root;
	while (ptr != NULL)
	{ // ptr->next!=NULL koşulu olsaydı son düğüm sayılmazdı
		counter++;
		ptr = ptr->next;
	}
	return counter;
}

/*cstringListede ilgili elemanın nodunu döndürür*/
static cstringList *listFind(cstringList **root, char *key)
{
	if (*root == NULL)
		return NULL;

	cstringList *ptr = *root;

	while (ptr != NULL)
	{
		if (strcmp(ptr->data->string, key))
			ptr = ptr->next;
		else
			break;
	}

	return ptr;
}

/*cstringListedeki elemanı siler*/
static void listDeleteNode(cstringList **root, char *key)
{
	if (*root == NULL)
		return;

	if (!strcmp((*root)->data->string, key))
	{
		cstringList *temp = *root;
		*root = (*root)->next;
		free(temp);
		return;
	}

	cstringList *ptr = *root;
	while (ptr->next != NULL)
	{
		if (!strcmp(ptr->next->data->string, key))
		{
			cstringList *temp = ptr->next;
			ptr->next = temp->next;
			free(temp);
			return;
		}
		ptr = ptr->next;
	}
}

/*cstringListeyi siler*/
static void listDestroy(cstringList **root)
{
	if (*root == NULL)
		return;

	cstringList *temp;
	while (*root != NULL)
	{
		temp = *root;
		*root = (*root)->next;
		free(temp->data->string);
		free(temp);
	}
}

cstringList *StringList()
{
	cstringList *root = (cstringList *)malloc((sizeof(cstring)));

	root->data = NULL;
	root->next = NULL;

	root->listAdd = listAdd;
	root->listCount = listCount;
	root->listFind = listFind;
	root->listDeleteNode = listDeleteNode;
	root->listDestroy = listDestroy;

	return root;
}

//cstringList end

static int length(cstring *this)
{
	return strlen(this->string);
}

static void new (cstring *this, char *new_string)
{
	int length = strlen(new_string);
	if (this->string != NULL)
	{
		memset(this->string, 0, strlen(this->string));
		this->string = (char *)realloc(this->string, sizeof(char) * (length + 1));
	}
	else
		this->string = (char *)calloc(length + 1, sizeof(char));
	if (length)
		memcpy(this->string, new_string, length);
	this->string[length] = '\0';
}

static void slice(cstring *this, cstring *destination, int start, int end)
{
	if (start == end || start <= -1 || end <= -1)
	{
		destination->new (destination, "");
		return;
	}
	
	char temp[(end - start) + 1];
	memset(temp, 0, (end - start) + 1);
	for (int i = 0, j = start; i < (end - start); i++, j++)
		temp[i] = this->string[j];
	temp[(end - start)] = '\0';

	destination->new (destination, temp);
}

static void addHead(cstring *this, char *add_head)
{
	int length_head = strlen(add_head);
	int length_string = strlen(this->string);
	char temp[length_head + length_string + 1];
	for (int i = 0; i < length_head; i++)
		temp[i] = add_head[i];
	for (int i = length_head, j = 0; i < length_head + length_string; i++, j++)
		temp[i] = this->string[j];
	temp[length_head + length_string] = '\0';

	this->new (this, temp);
}

static void addEnd(cstring *this, char *add_end)
{
	int length_end = strlen(add_end);
	int length_string = strlen(this->string);
	char temp[length_end + length_string + 1];
	for (int i = 0; i < length_string; i++)
		temp[i] = this->string[i];
	for (int i = length_string, j = 0; i < length_string + length_end; i++, j++)
		temp[i] = add_end[j];
	temp[length_string + length_end] = '\0';

	this->new (this, temp);
}

void addEndNewLine(cstring *this)
{
	int length = this->length(this);
	char temp[length + 2];
	for (int i = 0; i < length; i++)
		temp[i] = this->string[i];
	
	temp[length] = '\n';
	temp[length+1] ='\0';

	this->new (this, temp);
}

static void addTo(cstring *this, int index, char *addTo)
{
	int length_to = strlen(addTo);
	int length_string = strlen(this->string);
	char temp[length_to + length_string + 1];
	for (int i = 0; i < index; i++)
		temp[i] = this->string[i];
	for (int i = index, j = 0; i < index + length_to; i++, j++)
		temp[i] = addTo[j];
	for (int i = index + length_to, j = index; i < length_to + length_string; i++, j++)
		temp[i] = this->string[j];
	temp[length_to + length_string] = '\0';

	this->new (this, temp);
}


static void replaceThisIndex(cstring *this, int start, int end, char *textToReplace)
{
	if(start >= end)
		return;
	int newTextLength = strlen(textToReplace);
	int textLength = strlen(this->string);
	char temp[ textLength + newTextLength + 1 - (end- start)];
	for (int i = 0; i < start; i++)
		temp[i] = this->string[i];
	for (int i = start, j = 0; i < newTextLength + start; i++, j++)
		temp[i] = textToReplace[j];
	for (int i = end,j =  newTextLength + start; i < textLength ; i++,j++)
		temp[j] = this->string[i];
	
	temp[textLength + newTextLength  - (end- start)] = '\0';
	this->new (this, temp);
}

static int find(cstring *this, int index, char *find)
{

	char *address = strstr(&(this->string[index]), find);
	if (address == NULL)
		return -1;
	char *p = this->string;
	int findIndex = 0;
	while (p++ != address)
		findIndex++;
	return findIndex;
}

static int howManyAreIn(cstring *this, char *text)
{
	int count = -1;
	int index = -1;
	do
	{
		count++;
		index = find(this, index + 1, text);
	} while (index != -1);

	return count;
}

static void deleteNtoN(cstring *this, int start, int end)
{
	int length = this->length(this);
	char temp[length - (end - start) + 1];
	memset(temp, 0, length - (end - start) + 1);
	for (int i = 0; i < start; i++)
		temp[i] = this->string[i];
	for (int i = end, j = start; i <= length; i++, j++)
		temp[j] = this->string[i];
	temp[length - (end - start)] = '\0';

	this->new (this, temp);
}

static void replaceThisString(cstring *this, char *old, char *new)
{
	int oldCount = this->howManyAreIn(this, old);
	if (oldCount == 0)
		return;

	int sizeOld = oldCount * strlen(old);
	int sizeNew = oldCount * strlen(new);
	int tempLength = strlen(this->string) + (sizeNew - sizeOld);
	char temp[tempLength + 1];
	int index = -1;
	int oldIndex = 0;
	for (int i = 0, l = 0; i < tempLength;)
	{
		index = find(this, index + 1, old);
		if (index == -1)
		{
			for (int j = l; j < strlen(this->string); j++, i++, l++)
				temp[i] = this->string[l];
			break;
		}
		for (int j = oldIndex; j < index; j++, i++, l++)
			temp[i] = this->string[l];

		oldIndex = index;

		for (int j = 0; j < strlen(new); j++, i++)
			temp[i] = new[j];

		l += strlen(old);
		oldIndex += strlen(old);
	}
	temp[tempLength] = '\0';

	this->new (this, temp);
}

//BUG \n geçirilemiyor
static void join(cstring *this, cstring *destination, int count, ...)
{
	if (destination != this)
		destination->new (destination, this->string);
	va_list L;
	count += 3;
	va_start(L, count);
	for (int i = 3; i < count; i++)
		destination->addEnd(destination, va_arg(L, char *));

	va_end(L);
}

static void upper(cstring *this)
{
	int length = strlen(this->string);
	int beUpper = 'A' - 'a';
	for (int i = 0; i < length; i++)
	{
		if (this->string[i] <= 'z' && this->string[i] >= 'a')
			this->string[i] += beUpper;
	}
}

static void lower(cstring *this)
{
	int length = strlen(this->string);
	int beLower = 'A' - 'a';
	for (int i = 0; i < length; i++)
	{
		if (this->string[i] <= 'Z' && this->string[i] >= 'A')
			this->string[i] -= beLower;
	}
}

static cstring *parser(cstring *this, char *parseString)
{
	int length = strlen(parseString);
	int index = find(this, 0, parseString);
	if (index == -1)
		return NULL;
	cstring *new = String("");
	this->slice(this, new, 0, index);
	cstring *temp = String("");
	this->slice(this, temp, index + length, strlen(this->string));
	this->new (this, temp->string);
	return new;
}

int isSpace(cstring *this)
{
	int length = this->length(this);
	for (int i = 0; i < length; i++)
		if (this->string[i] != ' ')
			return -1;

	return 0;
}


static cstringList *getListFromParse(cstring *this, char *parseString)
{
	cstring *temp;
	cstringList *root = StringList();
	while (1 == 1)
	{
		temp = this->parser(this, parseString);
		if (temp == NULL)
		{
			root->listAdd(&root, this->string);
			break;
		}
		root->listAdd(&root, temp->string);
		free(temp);
	}
	return root;
}

cstring *String(char *s)
{
	cstring *this = (cstring *)malloc(sizeof(cstring));

	this->string = (char *)calloc((strlen(s) + 1), sizeof(char));
	memcpy(this->string, s, strlen(s));
	this->string[strlen(s)] = '\0';

	this->length = length;
	this->new = new;
	this->slice = slice;
	this->addHead = addHead;
	this->addEnd = addEnd;
	this->addEndNewLine=addEndNewLine;
	this->addTo = addTo;
	this->replaceThisIndex = replaceThisIndex;
	this->find = find;
	this->howManyAreIn = howManyAreIn;
	this->replaceThisString = replaceThisString;
	this->join = join;
	this->upper = upper;
	this->lower = lower;
	this->parser = parser;
	this->getListFromParse = getListFromParse;
	this->deleteNtoN = deleteNtoN;
	this->isSpace = isSpace;

	return this;
}