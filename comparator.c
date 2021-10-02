#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <panel.h>
#include <locale.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "cstring.h"

#define ALT_BACKSPACE 127
#define ALT_ENTER 10
#define ALT_DELETE 330
#define ALT_TAB 9
#define ALT_CTRL_Q 17
#define ALT_CTRL_S 19
#define ALT_CTRL_T 20
//not used
#define ALT_CTRL_C 3
#define ALT_CTRL_V 22
#define ALT_CTRL_A 1
//not used end
#define wchar unsigned int
#define input_device_stdin 0
#define input_device_file 1
#define input_device_cstring 2

#define c_code 2
#define asm_code 3

typedef struct Row Row;
typedef struct Cursor Cursor;
typedef struct Page Page;

WINDOW *my_wins[2];
PANEL *my_panels[2];
PANEL *top;

char *asm_file_name = "data/asm_code.asm";
FILE *asm_file_ptr;

char *c_file_name = "data/c_code.c";
FILE *c_file_ptr;

char *c_temp_file_name = "data/c_code_temp.c";

cstringList sourceList;
cstringList assemblyList;
cstringList source_list_ptr;
cstringList asm_list_ptr;
int cstr_i = 0;

struct Page
{
	Cursor *cursor;
	Row *row;
	int row_count;
	int max_y;
	int max_x;
	int screen_x;
	int screen_y;
	int start_color;
	int end_color;
	int start_loop_color;
	int end_loop_color;
};
Page c_page;
Page asm_page;

struct Cursor
{
	int y;
	int x;
	int lastX;
};

struct Row
{
	wchar *line;
	int length;
	Row *next;
};

void add_row(int source_code)
{
	Page *proxy;
	if (source_code == c_code)
		proxy = &c_page;
	else if (source_code == asm_code)
		proxy = &asm_page;

	Row *ptr = proxy->row;
	for (int i = 0; i < proxy->cursor->y; i++)
		ptr = ptr->next;
	//en sona ekle
	if (ptr->next == NULL)
	{
		ptr->next = (Row *)malloc(sizeof(Row));
		ptr = ptr->next;

		ptr->line = NULL;
		ptr->length = 0;
		ptr->next = NULL;
	} //en başa ekle
	else if (ptr == proxy->row && proxy->cursor->x == 0)
	{
		Row *temp = (Row *)malloc(sizeof(Row));
		temp->next = ptr;
		ptr = temp;
		ptr->line = NULL;
		ptr->length = 0;
	} //araya ekle
	else
	{
		Row *temp = (Row *)malloc(sizeof(Row));
		temp->next = ptr->next;
		ptr->next = temp;
		ptr = ptr->next;
		ptr->line = NULL;
		ptr->length = 0;
	}
	proxy->row_count += 1;
}

void add_ch(int ch, int source_code)
{
	Page *proxy;
	if (source_code == c_code)
		proxy = &c_page;
	else if (source_code == asm_code)
		proxy = &asm_page;

	Row *ptr = proxy->row;
	for (int i = 0; i < proxy->cursor->y; i++)
		ptr = ptr->next;

	if (ptr->line == NULL)
	{
		ptr->line = (wchar *)malloc(sizeof(wchar) * 2);
		ptr->length = 1;
	}
	else
	{
		ptr->line = (wchar *)realloc(ptr->line, sizeof(wchar) * (ptr->length + 2));
		ptr->length += 1;
	}
	ptr->line[ptr->length] = '\0';
	if (proxy->cursor->x == ptr->length - 1)
		ptr->line[ptr->length - 1] = ch;

	else
	{
		for (int i = ptr->length - 1; i > proxy->cursor->x; i--)
			ptr->line[i] = ptr->line[i - 1];

		ptr->line[proxy->cursor->x] = ch;
	}
}

void new_line(char ch, int source_code)
{
	Page *proxy;
	if (source_code == c_code)
		proxy = &c_page;
	else if (source_code == asm_code)
		proxy = &asm_page;

	Row *ptr = proxy->row;
	int i = 0;
	for (; i < proxy->cursor->y; i++)
		ptr = ptr->next;

	//satırın ortasında bi yerde enter a basıldıysa
	if (ptr->length > proxy->cursor->x)
	{
		add_row(source_code);
		int x = proxy->cursor->x;
		proxy->cursor->y += 1;
		proxy->cursor->x = 0;
		for (int i = x; i < ptr->length; i++)
		{
			move(proxy->cursor->y, proxy->cursor->x);
			add_ch(ptr->line[i], source_code);
			proxy->cursor->x += 1;
		}
		ptr->line[x] = '\0';
		ptr->line = (wchar *)realloc(ptr->line, sizeof(wchar) * (x + 1));
		ptr->length = x;
		proxy->cursor->x = 0;
	}
	else
	{
		add_row(source_code);
		proxy->cursor->y += 1;
		proxy->cursor->x = 0;
	}
}

void backspace()
{
	if (c_page.cursor->x > 0)
	{
		Row *ptr = c_page.row;
		for (int i = 0; i < c_page.cursor->y; i++)
			ptr = ptr->next;

		for (int i = c_page.cursor->x; i <= ptr->length - 1; i++)
			ptr->line[i - 1] = ptr->line[i];

		ptr->line = (wchar *)realloc(ptr->line, sizeof(wchar) * (ptr->length));
		ptr->length -= 1;
		c_page.cursor->x -= 1;
	}
	// bu satırı yok et ve bu satırın tüm elemanlarını bir üst satıra ekle
	else if (c_page.cursor->y != 0)
	{
		Row *ptr = c_page.row;
		for (int i = 0; i < c_page.cursor->y - 1; i++)
			ptr = ptr->next;

		Row *temp = ptr->next;
		ptr->next = temp->next;
		c_page.cursor->y -= 1;
		int x = ptr->length;
		for (int i = 0; i < temp->length; i++)
		{
			c_page.cursor->x = ptr->length;
			add_ch(temp->line[i], c_code);
		}
		c_page.cursor->x = x;
		free(temp->line);
		free(temp);
	}
}

void delete ()
{
	Row *ptr = c_page.row;
	for (int i = 0; i < c_page.cursor->y; i++)
		ptr = ptr->next;
	//arada bir yerde ise
	if (c_page.cursor->x < ptr->length)
	{
		for (int i = c_page.cursor->x + 1; i <= ptr->length - 1; i++)
			ptr->line[i - 1] = ptr->line[i];

		ptr->line = (wchar *)realloc(ptr->line, sizeof(wchar) * (ptr->length));
		ptr->length -= 1;
	}
	//sonda ise, alt satırı getir, null ise getirme
	else if (ptr->next != NULL)
	{
		Row *temp = ptr->next;
		ptr->next = temp->next;
		int x = ptr->length;
		for (int i = 0; i < temp->length; i++)
		{
			c_page.cursor->x = ptr->length;
			add_ch(temp->line[i], c_code);
		}
		c_page.cursor->x = x;
		free(temp->line);
		free(temp);
	}
}

int byte_number(unsigned int ch)
{
	if (ch <= 127)
		return 1;
	else if (ch <= 223)
		return 2;
	else if (ch <= 239)
		return 3;
	else if (ch <= 247)
		return 4;
	else
		return -1;
}

unsigned int unicode_to_utf8(unsigned int ch, int input_device, int source_code)
{
	int bytes = byte_number(ch);
	unsigned int c = ch;
	for (int i = 1; i < bytes; i++)
	{
		if (input_device == input_device_stdin)
			ch = getch();
		else if (input_device == input_device_file && source_code == c_code)
			ch = fgetc(c_file_ptr);
		else if (input_device == input_device_file && source_code == asm_code)
			ch = fgetc(asm_file_ptr);
		else if (input_device == input_device_cstring)
		{
			ch = asm_list_ptr->data->string[cstr_i];
			asm_list_ptr = asm_list_ptr->next;
			cstr_i += 1;
		}

		c += ch << (8 * i);
	}
	return c;
}

void colorant()
{
	source_list_ptr = sourceList;
	asm_list_ptr = assemblyList;
	int i, j, k = 0;
	cstringList old_source_list_ptr = NULL;
	cstring loop = String("");

	//ilk fonksiyona konuşlan
	for (;; asm_list_ptr = asm_list_ptr->next)
	{
		int count = asm_list_ptr->data->howManyAreIn(asm_list_ptr->data, ">:");
		if (count > 0)
			break;
	}
	//sonrakine geç
	asm_list_ptr = asm_list_ptr->next;

	for (i = 0, j = -1; i < asm_page.cursor->y && source_list_ptr != NULL && asm_list_ptr != NULL;)
	{ //veriler aynıysa ikiside ilerlesin
		if (strcmp(asm_list_ptr->data->string, source_list_ptr->data->string) == 0)
		{
			asm_list_ptr = asm_list_ptr->next;
			old_source_list_ptr = source_list_ptr;
			source_list_ptr = source_list_ptr->next;
			j++;
			k = i;
			loop->new (loop, "NULL");
		}
		//aynı değilse asm dir sadece asm ilerlesin
		else
		{
			if ((asm_list_ptr->data->howManyAreIn(asm_list_ptr->data, "for") > 0 || asm_list_ptr->data->howManyAreIn(asm_list_ptr->data, "while") > 0))
			{
				loop->new (loop, asm_list_ptr->data->string);
				k = i;
			}
			else
			{
				i++;
			}
			asm_list_ptr = asm_list_ptr->next;
		}
	}
	//k asm nin başını belirtir bundan dolayı sadece eşitlik durumunda yada loop un ikincisinden sonra attı, en sonunda ise ya eşitlikte yada loopun 2.sinde kaldığı için bir tane ileri alınmalı
	k++;

	// renklendirme sonu ya bir sonraki kod a kadar, yada loop a kadar
	while (source_list_ptr != NULL && asm_list_ptr != NULL)
	{
		if (strcmp(asm_list_ptr->data->string, source_list_ptr->data->string) == 0 || asm_list_ptr->data->howManyAreIn(asm_list_ptr->data, "for") > 0 || asm_list_ptr->data->howManyAreIn(asm_list_ptr->data, "while") > 0)
			break;
		asm_list_ptr = asm_list_ptr->next;
		i++;
	}
	// 2. loopda j yi geri çek ama source ptr yi değiştirme
	if (loop->howManyAreIn(loop, "NULL") == 0 && strcmp(loop->string, "\0") != 0)
	{
		cstringList temp = source_list_ptr;
		while (temp->data->howManyAreIn(temp->data, loop->string) == 0)
		{
			j--;
			temp = temp->back;
		}
		j++;
	}
	c_page.start_color = j;
	c_page.end_color = j + 1;

	//asm de hareket ederken source de hareket etsin
	// c_page.cursor->y = c_page.start_color;
	// if (c_page.cursor->y < c_page.screen_y)
		// c_page.screen_y -= 1;
	// else if (c_page.cursor->y >= c_page.screen_y + c_page.max_y)
		// c_page.screen_y += 1;

	asm_page.start_color = k;
	asm_page.end_color = i;

	//loop un 2. bölümünü bulmak için
	if (old_source_list_ptr != NULL)
	{
		if (old_source_list_ptr->data->howManyAreIn(old_source_list_ptr->data, "for") > 0 || old_source_list_ptr->data->howManyAreIn(old_source_list_ptr->data, "while") > 0)
		{
			i += 1; //asm_list_ptr jmp nin bir fazlasında
			while (source_list_ptr != NULL && asm_list_ptr != NULL)
			{
				if (strcmp(asm_list_ptr->data->string, old_source_list_ptr->data->string) == 0)
					break;

				if (strcmp(asm_list_ptr->data->string, source_list_ptr->data->string) != 0)
					i++;
				else
					source_list_ptr = source_list_ptr->next;
				asm_list_ptr = asm_list_ptr->next;
			}
			asm_page.start_loop_color = i;

			asm_list_ptr = asm_list_ptr->next;
			while (source_list_ptr != NULL && asm_list_ptr != NULL)
			{
				if (strcmp(asm_list_ptr->data->string, source_list_ptr->data->string) == 0)
					break;
				asm_list_ptr = asm_list_ptr->next;
				i++;
			}
			asm_page.end_loop_color = --i;
		}
	}

	// ileri değil geri gitmeliyiz
	if (loop->howManyAreIn(loop, "NULL") == 0 && strcmp(loop->string, "\0") != 0)
	{
		//bir tur geri alıyoruz
		while (strcmp(asm_list_ptr->data->string, loop->string) != 0)
		{
			if (strcmp(source_list_ptr->data->string, asm_list_ptr->data->string) == 0)
			{
				source_list_ptr = source_list_ptr->back;
			}
			else
				asm_list_ptr = asm_list_ptr->back;
		}
		asm_list_ptr = asm_list_ptr->back;
		int l = 0;

		while (strcmp(asm_list_ptr->data->string, loop->string) != 0)
		{
			if (strcmp(source_list_ptr->data->string, asm_list_ptr->data->string) == 0)
			{
				source_list_ptr = source_list_ptr->back;
			}
			else
				l--;
			asm_list_ptr = asm_list_ptr->back;
		}
		asm_page.start_loop_color = k + l;
		// renklendirme sonu ya bir sonraki kod a kadar, yada loop a kadar

		asm_list_ptr = asm_list_ptr->next;
		source_list_ptr = source_list_ptr->next;
		l = -1;
		while (source_list_ptr != NULL && asm_list_ptr != NULL)
		{
			if (strcmp(asm_list_ptr->data->string, source_list_ptr->data->string) == 0 || asm_list_ptr->data->howManyAreIn(asm_list_ptr->data, "for") > 0 || asm_list_ptr->data->howManyAreIn(asm_list_ptr->data, "while") > 0)
			{
				break;
			}
			asm_list_ptr = asm_list_ptr->next;
			l++;
		}
		asm_page.end_loop_color = asm_page.start_loop_color + l;
	}
}

void save_file();
int process_key(unsigned int ch, int input_device, int source_code)
{
	switch (ch)
	{
	case ALT_CTRL_Q:
	{
		endwin();
		exit(0);
	}
	case ALT_CTRL_S:
	{
		save_file();
		break;
	}
	case ALT_CTRL_T:
	{
		if (top == my_panels[0])
			top = my_panels[1];
		else if (top == my_panels[1])
			top = my_panels[0];
		top_panel(top);
	}
	case KEY_RIGHT:
	{
		Page *proxy;
		WINDOW *win;
		if (source_code == c_code)
		{
			proxy = &c_page;
			win = my_wins[0];
		}
		else if (source_code == asm_code)
		{
			proxy = &asm_page;
			win = my_wins[1];
		}

		Row *ptr = proxy->row;
		for (int i = 0; i < proxy->cursor->y; i++)
			ptr = ptr->next;

		if (proxy->cursor->x != ptr->length)
			proxy->cursor->x += 1;

		else if (ptr->next != NULL)
		{
			proxy->cursor->x = 0;
			proxy->cursor->y += 1;
		}
		wmove(win, proxy->cursor->y, proxy->cursor->x);
		proxy->cursor->lastX = -1;
		if (source_code == asm_code)
		{
			c_page.start_loop_color = -1;
			c_page.end_loop_color = -1;
			asm_page.start_loop_color = -1;
			asm_page.end_loop_color = -1;
			colorant();
		}
		else if (source_code == c_code)
		{
			c_page.start_color = -1;
			c_page.end_color = -1;
			c_page.start_loop_color = -1;
			c_page.end_loop_color = -1;

			asm_page.start_color = -1;
			asm_page.end_color = -1;
			asm_page.start_loop_color = -1;
			asm_page.end_loop_color = -1;
		}
		break;
	}
	case KEY_LEFT:
	{
		Page *proxy;
		WINDOW *win;
		if (source_code == c_code)
		{
			proxy = &c_page;
			win = my_wins[0];
		}
		else if (source_code == asm_code)
		{
			proxy = &asm_page;
			win = my_wins[1];
		}

		if (proxy->cursor->x > 0)
			proxy->cursor->x -= 1;

		else if (proxy->cursor->y > 0)
		{
			Row *ptr = proxy->row;
			for (int i = 0; i < proxy->cursor->y - 1; i++)
				ptr = ptr->next;

			proxy->cursor->x = ptr->length;
			proxy->cursor->y -= 1;
		}
		wmove(win, proxy->cursor->y, proxy->cursor->x);
		proxy->cursor->lastX = -1;
		if (source_code == asm_code)
		{
			c_page.start_loop_color = -1;
			c_page.end_loop_color = -1;
			asm_page.start_loop_color = -1;
			asm_page.end_loop_color = -1;
			colorant();
		}
		else if (source_code == c_code)
		{
			c_page.start_color = -1;
			c_page.end_color = -1;
			c_page.start_loop_color = -1;
			c_page.end_loop_color = -1;

			asm_page.start_color = -1;
			asm_page.end_color = -1;
			asm_page.start_loop_color = -1;
			asm_page.end_loop_color = -1;
		}
		break;
	}
	case KEY_UP:
	{
		Page *proxy;
		WINDOW *win;
		if (source_code == c_code)
		{
			proxy = &c_page;
			win = my_wins[0];
		}
		else if (source_code == asm_code)
		{
			proxy = &asm_page;
			win = my_wins[1];
		}

		if (proxy->cursor->y > 0)
		{
			proxy->cursor->y -= 1;
			if (proxy->cursor->y < proxy->screen_y)
				proxy->screen_y -= 1;
		}

		Row *ptr = proxy->row;
		for (int i = 0; i < proxy->cursor->y; i++)
			ptr = ptr->next;
		if (proxy->cursor->lastX != -1)
			proxy->cursor->x = proxy->cursor->lastX;

		if (ptr->length <= proxy->cursor->x)
		{
			proxy->cursor->lastX = proxy->cursor->x;
			proxy->cursor->x = ptr->length;
		}
		if (source_code == asm_code)
		{
			c_page.start_loop_color = -1;
			c_page.end_loop_color = -1;
			asm_page.start_loop_color = -1;
			asm_page.end_loop_color = -1;
			colorant();
		}
		else if (source_code == c_code)
		{
			c_page.start_color = -1;
			c_page.end_color = -1;
			c_page.start_loop_color = -1;
			c_page.end_loop_color = -1;

			asm_page.start_color = -1;
			asm_page.end_color = -1;
			asm_page.start_loop_color = -1;
			asm_page.end_loop_color = -1;
		}
		break;
	}
	case KEY_DOWN:
	{
		Page *proxy;
		WINDOW *win;
		if (source_code == c_code)
		{
			proxy = &c_page;
			win = my_wins[0];
		}
		else if (source_code == asm_code)
		{
			proxy = &asm_page;
			win = my_wins[1];
		}

		Row *ptr = proxy->row;
		for (int i = 0; i < proxy->cursor->y; i++)
			ptr = ptr->next;
		if (ptr->next != NULL)
		{
			proxy->cursor->y += 1;
			ptr = ptr->next;

			if (proxy->cursor->y >= proxy->screen_y + proxy->max_y)
				proxy->screen_y += 1;
		}
		else
			proxy->cursor->x = ptr->length;

		if (proxy->cursor->lastX != -1)
			proxy->cursor->x = proxy->cursor->lastX;

		if (ptr->length - 1 < proxy->cursor->x)
		{
			proxy->cursor->lastX = proxy->cursor->x;
			proxy->cursor->x = ptr->length;
		}

		if (source_code == asm_code)
		{
			c_page.start_loop_color = -1;
			c_page.end_loop_color = -1;
			asm_page.start_loop_color = -1;
			asm_page.end_loop_color = -1;
			colorant();
		}
		else if (source_code == c_code)
		{
			c_page.start_color = -1;
			c_page.end_color = -1;
			c_page.start_loop_color = -1;
			c_page.end_loop_color = -1;

			asm_page.start_color = -1;
			asm_page.end_color = -1;
			asm_page.start_loop_color = -1;
			asm_page.end_loop_color = -1;
		}
		break;
	}
	case KEY_BACKSPACE:
	{
		if (source_code == asm_code)
			break;
		backspace();
		break;
	}
	case ALT_DELETE:
	{
		if (source_code == asm_code)
			break;
		delete ();
		break;
	}
	case ALT_ENTER:
	{
		if (source_code == asm_code && input_device == input_device_stdin)
			break;

		Page *proxy;
		if (source_code == c_code)
			proxy = &c_page;
		else if (source_code == asm_code)
			proxy = &asm_page;

		new_line(ch, source_code);
		proxy->cursor->lastX = -1;
		if (proxy->cursor->y >= proxy->screen_y + proxy->max_y)
			proxy->screen_y += 1;
		break;
	} //TOD tab tuşu boşluğa çevrildi mümküse tab olarak kalsın
	case ALT_TAB:
	{
		if (source_code == asm_code && input_device == input_device_stdin)
			break;
		Page *proxy;
		if (source_code == c_code)
			proxy = &c_page;
		else if (source_code == asm_code)
			proxy = &asm_page;

		int tab_size = 8;
		for (int i = 0; i < tab_size; i++)
			add_ch(' ', source_code);
		proxy->cursor->x += tab_size;
		proxy->cursor->lastX = -1;
		break;
	}
	default:
	{
		if (source_code == asm_code && input_device == input_device_stdin)
			break;

		Page *proxy;
		if (source_code == c_code)
			proxy = &c_page;
		else if (source_code == asm_code)
			proxy = &asm_page;

		ch = unicode_to_utf8(ch, input_device, source_code);
		add_ch(ch, source_code);
		proxy->cursor->x += 1;
		proxy->cursor->lastX = -1;
	}
	}
}

void print(int source_code)
{
	Page *proxy;
	WINDOW *win;
	if (source_code == c_code)
	{
		proxy = &c_page;
		win = my_wins[0];
	}
	else if (source_code == asm_code)
	{
		proxy = &asm_page;
		win = my_wins[1];
	}

	char temp[5] = {0};
	int bytes = 0;

	wmove(win, 0, 0);
	wclear(win);
	clear();
	Row *ptr = proxy->row;
	int start_index;
	for (start_index = 0; start_index < proxy->screen_y; start_index++)
		ptr = ptr->next;

	int printing_row_count = 0;
	int start_y = 0;
	int end_y = 0;

	while (ptr != NULL && printing_row_count < proxy->max_y)
	{
		end_y = proxy->max_x - 1 > ptr->length ? ptr->length : proxy->max_x - 1;
		start_y = proxy->cursor->x > proxy->max_x - 1 ? proxy->cursor->x - (proxy->max_x - 1) : 0;
		int i;
		for (i = start_y; i < end_y + start_y && i < ptr->length; i++)
		{
			for (int j = 0; j < 5; j++)
				temp[j] = 0;

			bytes = byte_number(ptr->line[i] & 255);
			for (int j = 0; j < bytes; j++)
				temp[j] = (ptr->line[i] >> (8 * j)) & 255;

			if ((source_code == asm_code && asm_page.end_color != -1 && start_index + printing_row_count >= asm_page.start_color && start_index + printing_row_count <= asm_page.end_color) ||
				(source_code == asm_code && asm_page.end_loop_color != -1 && start_index + printing_row_count >= asm_page.start_loop_color && start_index + printing_row_count <= asm_page.end_loop_color) ||
				(source_code == c_code && c_page.end_color != -1 && start_index + printing_row_count == c_page.start_color))
			{
				wattron(win, COLOR_PAIR(5));
				wprintw(win, "%s", temp);
				wattroff(win, COLOR_PAIR(5));
			}
			else
				wprintw(win, "%s", temp);
		}
		if ((source_code == asm_code && asm_page.end_color != -1 && start_index + printing_row_count >= asm_page.start_color && start_index + printing_row_count <= asm_page.end_color) ||
			(source_code == asm_code && asm_page.end_loop_color != -1 && start_index + printing_row_count >= asm_page.start_loop_color && start_index + printing_row_count <= asm_page.end_loop_color) ||
			(source_code == c_code && c_page.end_color != -1 && start_index + printing_row_count == c_page.start_color))
		{
			wattron(win, COLOR_PAIR(5));
			for (int j = 0; j < COLS / 2 - 2 - i; j++)
				waddch(win, ' ');
			wattroff(win, COLOR_PAIR(5));
		}
		ptr = ptr->next;
		wprintw(win, "\n");
		printing_row_count += 1;
	}
	asm_list_ptr = assemblyList;
	for (int i = 0; i < asm_page.start_color; i++)
	{
		asm_list_ptr = asm_list_ptr->next;
	}

	proxy->screen_x = proxy->cursor->x > proxy->max_x - 1 ? proxy->max_x - 1 : proxy->cursor->x;
	wmove(win, (proxy->cursor->y - proxy->screen_y), proxy->screen_x);

	attron(COLOR_PAIR(1));
	mvwvline(stdscr, 0, COLS / 2, ACS_VLINE, LINES);
	attroff(COLOR_PAIR(1));

	attron(COLOR_PAIR(3));
	mvwhline(stdscr, LINES - 1, 0, ACS_HLINE, COLS);
	attroff(COLOR_PAIR(3));

	attron(COLOR_PAIR(2));
	mvwprintw(stdscr, LINES - 1, 0, "line %d", c_page.cursor->y + 1);
	mvwprintw(stdscr, LINES - 1, COLS / 2 + 1, "line %d", asm_page.cursor->y + 1);
	attroff(COLOR_PAIR(2));

	wrefresh(win);
	update_panels();
	doupdate();
}

void remove_comment()
{
	char *sourceFile = c_file_name;			//your source code
	char *outputFile = c_temp_file_name; //output file

	FILE *fp, *ft;
	char ch, nextc;

	fp = fopen(sourceFile, "r");
	ft = fopen(outputFile, "w");

	nextc = fgetc(fp);
	while (nextc != EOF)
	{
		ch = nextc;
		nextc = fgetc(fp);

		if ((ch == '/') && (nextc == '/'))
		{
			nextc = fgetc(fp);
			while (nextc != '\n')
			{ // move to the end of line
				nextc = fgetc(fp);
			}
			ch = nextc;		   //end of line character
			nextc = fgetc(fp); //read 1st character from a new line
		}

		else if ((ch == '/') && (nextc == '*'))
		{
			{
				nextc = fgetc(fp);
				while (!((ch == '*') && (nextc == '/')))
				{ /* move to the end of comment*/
					ch = nextc;
					nextc = fgetc(fp);
				}
				ch = fgetc(fp); //read first character after the end of comment block
				nextc = fgetc(fp);
			}
		}

		putc(ch, ft);
	}

	fclose(fp);
	fclose(ft);
}

void fix_else_problem()
{
	remove_comment();
	FILE *file = fopen(c_temp_file_name, "r");
	struct stat st;
	if (stat(c_temp_file_name, &st) == 0)
	{
		if (st.st_size == 0)
		{
			fclose(file);
			return;
		}
	}
	//dosyayı oku ve depola
	cstring code = String("");
	int ch;
	char c[1];
	do
	{
		ch = fgetc(file);
		if (feof(file))
			break;

		c[0] = (char)ch;
		code->addEnd(code, c);
	} while (1);

	//else leri bul
	int index = 0;
	index = code->find(code, index, "else");
	while (index != -1)
	{
		index = code->find(code, index, "\n");
		index++;
		while (code->string[index] == '\n' || code->string[index] == '\t' || code->string[index] == ' ')
		{
			index++;
		}
		if (code->string[index] == '{')
		{
			int close = 1;
			while (close != 0)
			{
				index++;
				if (code->string[index] == '{')
					close++;
				else if (code->string[index] == '}')
					close--;
			}
			index++;
			cstring temp = code;
			int temp_index = index;
			while (temp->string[temp_index] == '\n' || temp->string[temp_index] == '\t' || temp->string[temp_index] == ' ')
			{
				temp_index++;
			}
			int next_else = temp->find(temp,index,"else");
			if(next_else != temp_index)
				code->addTo(code, index, "\nasm(\"\");");
		}
		else
		{
			index = code->find(code, index, ";");
			code->addTo(code, index + 1, "\nasm(\"\");");
		}
		index = code->find(code, index, "else");
	}
	fclose(file);

	file = fopen(c_temp_file_name, "w+");
	fprintf(file, "%s", code->string);
	fclose(file);
}

void destroy(int source_code);
void read_file();

//indenet işleminin ardından yeni bir dosya oluştur ve else lerin sonuna asm("") yi ekle
//bu dosyayı derleyip sonucu ana dosyayla karşılaştır. bu sayede okurken asm ayıklamasına gerek kalmaz
void disassembly()
{
	//girintileme yap
	system("indent data/c_code.c -nbad -bap -nbc -bbo -hnl -bli0 -brs -c33 -cd33 -ncdb -ci4 -cli0 -d0 -di1 -nfc1 -i8 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts8 -il1");
	//eski kodu sil ve girintilenmiş olanı oku
	destroy(c_code);
	read_file();
	//else sorununun çözümü için her else sonuna asm ekle
	fix_else_problem();
	//derle
	system("gcc -c data/c_code_temp.c -o data/c_code.o -g  2> data/asm_code.asm");
	if (access("data/c_code.o", F_OK) == -1)
	{
		destroy(asm_code);
		asm_file_ptr = fopen(asm_file_name, "r");
		struct stat st;
		if (stat(c_file_name, &st) == 0)
		{
			if (st.st_size == 0)
			{
				fclose(c_file_ptr);
				return;
			}
		}
		if (asm_file_ptr != NULL)
		{
			unsigned int ch;
			ch = fgetc(asm_file_ptr);
			while (ch != EOF)
			{
				process_key(ch, input_device_file, asm_code);
				ch = fgetc(asm_file_ptr);
			}
			print(asm_code);
			fclose(asm_file_ptr);
		}
		return;
	}
	//else
	system("objdump  --no-show-raw-insn -S  -M intel -d data/c_code.o >> data/asm_code.asm");
	system("rm data/c_code.o");

	destroy(asm_code);
	wclear(my_wins[1]);
	asm_file_ptr = fopen(asm_file_name, "r");
	c_file_ptr = fopen(c_temp_file_name, "r");

	//dosya içerikleri için string oluştur
	cstring sourceCode = String("");
	cstring assemblyCode = String("");

	//dosyadan karakter alan ve bu karakteri char* olarak depolayan değişkenler
	int ch;
	char c[1];

	//asm nin içi boşsa kapatıp çık
	struct stat st;
	if (stat(asm_file_name, &st) == 0)
	{
		if (st.st_size == 0)
		{
			fclose(asm_file_ptr);
			return;
		}
	}

		if (stat(c_file_name, &st) == 0)
	{
		if (st.st_size == 0)
		{
			fclose(c_file_ptr);
			return;
		}
	}

	//assebley dosyasından oku
	do
	{
		ch = fgetc(asm_file_ptr);
		if (feof(asm_file_ptr))
		{
			break;
		}
		c[0] = (char)ch;
		assemblyCode->addEnd(assemblyCode, c);
	} while (1);

	//source dosyasından oku
	do
	{
		ch = fgetc(c_file_ptr);
		if (feof(c_file_ptr))
		{
			break;
		}
		c[0] = (char)ch;
		sourceCode->addEnd(sourceCode, c);
	} while (1);

	sourceList = sourceCode->getListFromParse(sourceCode, "\n");
	assemblyList = assemblyCode->getListFromParse(assemblyCode, "\n");
	free(sourceCode->string);
	free(sourceCode);
	free(assemblyCode->string);
	free(assemblyCode);
	cstringList loop = StringList();

	source_list_ptr = sourceList;
	asm_list_ptr = assemblyList;

	while (asm_list_ptr != NULL)
	{
		//ilk fonksiyona konuşlan
		int count = 0;
		for (;; asm_list_ptr = asm_list_ptr->next)
		{
			count = asm_list_ptr->data->howManyAreIn(asm_list_ptr->data, ">:");
			if (count > 0)
				break;
		}
		//fprintf(stderr,"%s\n", asm_list_ptr->data->string);
		for (int i = 0; i < asm_list_ptr->data->length(asm_list_ptr->data); i++)
		{
			process_key(asm_list_ptr->data->string[cstr_i], input_device_cstring, asm_code);
			cstr_i += 1;
		}
		process_key('\n', input_device_cstring, asm_code);
		cstr_i = 0;

		//global değişkenleri atla
		//BUG global değişkende () bulunabilir
		count = 0;
		for (;; asm_list_ptr = asm_list_ptr->next)
		{
			count = asm_list_ptr->data->howManyAreIn(asm_list_ptr->data, "(");
			if (count > 0)
				break;
		}

		//fonksiyona gel ama prototipleri atla
		do
		{
			count = 0;
			for (;; source_list_ptr = source_list_ptr->next)
			{
				count = source_list_ptr->data->howManyAreIn(source_list_ptr->data, "(");
				if (count > 0)
					break;
			}
		} while (source_list_ptr->data->howManyAreIn(source_list_ptr->data, ";") > 0);

		// aynı verileri atla
		while (!strcmp(source_list_ptr->data->string, asm_list_ptr->data->string))
		{
			if ((asm_list_ptr->data->howManyAreIn(asm_list_ptr->data, "for") > 0 || asm_list_ptr->data->howManyAreIn(asm_list_ptr->data, "while") > 0))
			{
				loop->listAdd(&loop, asm_list_ptr->data->string);
			}
			asm_list_ptr = asm_list_ptr->next;
			source_list_ptr = source_list_ptr->next;
			//asm olanları yaz ta ki normal veri gelene kadar
			if (asm_list_ptr == NULL || source_list_ptr == NULL)
				goto exit;
			while (strcmp(source_list_ptr->data->string, asm_list_ptr->data->string))
			{
				if (loop->data == NULL || loop->listFind(&loop, asm_list_ptr->data->string) == NULL)
				{
					for (int i = 0; i < asm_list_ptr->data->length(asm_list_ptr->data); i++)
					{
						process_key(asm_list_ptr->data->string[cstr_i], input_device_cstring, asm_code);
						cstr_i += 1;
					}
					process_key('\n', input_device_cstring, asm_code);

					//fprintf(stderr,"%s\n", asm_list_ptr->data->string);
				}
				asm_list_ptr = asm_list_ptr->next;
				cstr_i = 0;

				if (asm_list_ptr == NULL || source_list_ptr == NULL)
					goto exit;
			}
		}
	exit:
		if (loop->data != NULL)
			loop->listDestroy(&loop);
	}
	print(asm_code);
	fclose(asm_file_ptr);
	fclose(c_file_ptr);
}

void read_file()
{
	c_file_ptr = fopen(c_file_name, "r");

	struct stat st;
	if (stat(c_file_name, &st) == 0)
	{
		if (st.st_size == 0)
		{
			fclose(c_file_ptr);
			return;
		}
	}
	if (c_file_ptr != NULL)
	{
		unsigned int ch;
		ch = fgetc(c_file_ptr);
		while (ch != EOF)
		{
			process_key(ch, input_device_file, c_code);
			ch = fgetc(c_file_ptr);
		}
		print(c_code);
		fclose(c_file_ptr);
	}
}

void save_file()
{
	c_file_ptr = fopen(c_file_name, "w");

	char temp[5] = {0};
	int bytes = 0;

	Row *ptr = c_page.row;
	int start_y = 0;
	int end_y = 0;

	while (ptr != NULL)
	{
		end_y = c_page.max_x - 1 > ptr->length ? ptr->length : c_page.max_x - 1;
		start_y = c_page.cursor->x > c_page.max_x - 1 ? c_page.cursor->x - (c_page.max_x - 1) : 0;
		for (int i = start_y; i < end_y + start_y && i < ptr->length; i++)
		{
			for (int j = 0; j < 5; j++)
				temp[j] = 0;

			bytes = byte_number(ptr->line[i] & 255);
			for (int j = 0; j < bytes; j++)
				temp[j] = (ptr->line[i] >> (8 * j)) & 255;

			fprintf(c_file_ptr, "%s", temp);
		}
		ptr = ptr->next;
		fprintf(c_file_ptr, "\n");
	}
	fclose(c_file_ptr);
	disassembly();
}

void init_wins()
{
	int x, y, i;
	attron(COLOR_PAIR(1));
	mvwvline(stdscr, 0, COLS / 2, ACS_VLINE, LINES);
	attroff(COLOR_PAIR(1));

	y = 0;
	x = 0;
	for (i = 0; i < 2; ++i)
	{
		my_wins[i] = newwin(c_page.max_y, c_page.max_x, y, x);

		y += 0;
		x += c_page.max_x + 2;
	}
}

void init_editor()
{
	c_page.cursor = (Cursor *)malloc(sizeof(Cursor));
	c_page.row = (Row *)malloc(sizeof(Row));

	asm_page.cursor = (Cursor *)malloc(sizeof(Cursor));
	asm_page.row = (Row *)malloc(sizeof(Row));

	c_page.row->line = (wchar *)malloc(sizeof(wchar));
	c_page.row->line[0] = '\0';
	c_page.row->next = NULL;
	c_page.row->length = 0;
	c_page.row_count = 1;

	asm_page.row->line = (wchar *)malloc(sizeof(wchar));
	asm_page.row->line[0] = '\0';
	asm_page.row->next = NULL;
	asm_page.row->length = 0;
	asm_page.row_count = 1;

	c_page.cursor->y = 0;
	c_page.cursor->x = 0;
	c_page.cursor->lastX = -1;

	asm_page.cursor->y = 0;
	asm_page.cursor->x = 0;
	asm_page.cursor->lastX = -1;

	c_page.start_color = -1;
	c_page.end_color = -1;
	c_page.start_loop_color = -1;
	c_page.end_loop_color = -1;

	asm_page.start_color = -1;
	asm_page.end_color = -1;
	asm_page.start_loop_color = -1;
	asm_page.end_loop_color = -1;

	getmaxyx(stdscr, c_page.max_y, c_page.max_x);
	c_page.max_x = c_page.max_x / 2 - 1;
	c_page.max_y -= 1;
	c_page.screen_x = c_page.max_x;
	c_page.screen_y = 0;

	getmaxyx(stdscr, asm_page.max_y, asm_page.max_x);
	asm_page.max_x = asm_page.max_x / 2 - 1;
	asm_page.max_y -= 1;
	asm_page.screen_x = asm_page.max_x;
	asm_page.screen_y = 0;

	init_color(COLOR_WHITE, 900, 900, 900);
	init_color(COLOR_BLUE, 0, 350, 700);
	init_color(COLOR_YELLOW, 700, 700, 0);
	init_color(COLOR_RED, 700, 0, 0);
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	init_pair(2, COLOR_WHITE, COLOR_BLUE);
	init_pair(3, COLOR_BLUE, COLOR_BLUE);
	init_pair(4, COLOR_WHITE, COLOR_WHITE);
	init_pair(5, COLOR_WHITE, COLOR_RED);

	init_wins();
	wbkgd(my_wins[0], COLOR_PAIR(1));
	wbkgd(my_wins[1], COLOR_PAIR(1));
	wbkgd(stdscr, COLOR_PAIR(4));

	/* Her pencereyi bir panele bağla */  /* Sıralama aşağıdan yukarıya */
	my_panels[0] = new_panel(my_wins[0]); /* 0'a  it, sıra: stdscr-0 */
	my_panels[1] = new_panel(my_wins[1]); /* 1'e  it, sıra: stdscr-0-1 */

	/* Kullanıcı işaretçilerini bir sonraki panele ayarla */
	set_panel_userptr(my_panels[0], my_panels[1]);
	set_panel_userptr(my_panels[1], my_panels[0]);

	/* Yığını güncelle. 2. panel en üstte olacak */
	update_panels();
	doupdate();
	top = my_panels[0];
	top_panel(my_panels[0]);
}

void destroy(int source_code)
{
	Page *proxy;

	if (source_code == c_code)
		proxy = &c_page;

	else if (source_code == asm_code)
		proxy = &asm_page;

	Row *ptr = proxy->row;
	Row *temp;
	while (ptr != NULL)
	{
		temp = ptr;
		ptr = ptr->next;
		free(temp->line);
		free(temp);
	}
	proxy->row = (Row *)malloc(sizeof(Row));
	proxy->row->line = (wchar *)malloc(sizeof(wchar));
	proxy->row->line[0] = '\0';
	proxy->row->next = NULL;
	proxy->row->length = 0;
	proxy->row_count = 1;

	proxy->cursor->y = 0;
	proxy->cursor->x = 0;
	proxy->cursor->lastX = -1;

	getmaxyx(stdscr, proxy->max_y, proxy->max_x);
	proxy->max_x = proxy->max_x / 2 - 1;
	proxy->screen_x = proxy->max_x;
	proxy->screen_y = 0;
}

static void sighandler(int signum)
{
	if (SIGWINCH == signum)
	{
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		COLS = w.ws_col;
		LINES = w.ws_row;
		c_page.max_y = w.ws_row - 1;
		c_page.max_x = w.ws_col / 2 - 1;
		asm_page.max_y = w.ws_row - 1;
		asm_page.max_x = w.ws_col / 2 - 1;
		resizeterm(w.ws_row, w.ws_col);
		wresize(my_wins[0], c_page.max_y, c_page.max_x);
		wresize(my_wins[1], asm_page.max_y, asm_page.max_x);
		wresize(stdscr, w.ws_row, w.ws_col);
		mvwin(my_wins[1], 0, c_page.max_x + 2);
		print(c_code);
		print(asm_code);
		//clear buffer
		while ((getch()) != 410)
			;
	}
}

int main(int argc, char const *argv[])
{
	setlocale(LC_ALL, ""); //utf-8 dessteği
	initscr();
	start_color();
	raw();				  //tuşa basar basmaz eriş
	noecho();			  //bastığım tuş gözükmesin
	keypad(stdscr, true); //f ve ok tuşlarını etkinleştir

	init_editor();
	read_file();
	signal(SIGWINCH, sighandler);

	unsigned int ch;
	int n;
	while (1 == 1)
	{
		ch = getch();
		if (top == my_panels[0])
		{
			process_key(ch, input_device_stdin, c_code);
			if (ioctl(0, FIONREAD, &n) == 0 && n == 0)
				print(c_code);
		}
		else if (top == my_panels[1])
		{
			process_key(ch, input_device_stdin, asm_code);
			if (ioctl(0, FIONREAD, &n) == 0 && n == 0)
				print(c_code);
			if (ioctl(0, FIONREAD, &n) == 0 && n == 0)
				print(asm_code);
		}
	}

	destroy(c_code);
	destroy(asm_code);
	endwin();
	return 0;
}