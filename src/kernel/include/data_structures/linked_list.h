

#ifndef LINKEDLIST_H
#define LINKEDLIST_H
struct dll_Entry
{
    struct dll_Entry* next;
    struct dll_Entry* prev;
};

struct dll_Head
{
    struct dll_Entry* first;
    struct dll_Entry* last;
};




#endif

struct dll_Entry* remove_dll_entry(struct dll_Entry* to_rm);
void insert_dll_entry_after(struct dll_Entry* insert_after, struct dll_Entry* to_insert);
void insert_dll_entry_head(struct dll_Head* header, struct dll_Entry* to_insert);
void insert_dll_entry_tail(struct dll_Head* header, struct dll_Entry* to_insert);