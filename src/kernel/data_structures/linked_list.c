#include <stdint.h>
#include <stddef.h>
#include <panic.h>
#include <linked_list.h>


struct dll_Entry* remove_dll_entry(struct dll_Entry* to_rm)
{
    struct dll_Entry* prev = to_rm->prev;
    struct dll_Entry* next = to_rm->prev;

    if (prev != NULL)
        prev->next = next;
    if (next != NULL)
        next->prev = prev;
    return to_rm;
}



void insert_dll_entry_after(struct dll_Entry* insert_after, struct dll_Entry* to_insert)
{
    if (insert_after == NULL)
        panic("insert_dll_entry_after() --> insert_after == NULL.\n");

    struct dll_Entry* old_next = insert_after->next;
    
    insert_after->next = to_insert;
    to_insert->prev = insert_after;
    to_insert->next = old_next;

    if (old_next != NULL)
        old_next->prev = to_insert;
}


void insert_dll_entry_head(struct dll_Head* header, struct dll_Entry* to_insert)
{
    if (header->first == NULL)
    {
        if (header->last != NULL)
            panic("insert_dll_entry_head() --> header->first was NULL, but non NULL value in header->last");

        header->first = to_insert;
        to_insert->prev = (struct dll_Entry* )header;
        to_insert->next = (struct dll_Entry* )header;
    }
    else
    {
        struct dll_Entry* old_next = header->first;
        
        header->first = to_insert;
        to_insert->prev = (struct dll_Entry* )header;
        to_insert->next = old_next;
    }
}

void insert_dll_entry_tail(struct dll_Head* header, struct dll_Entry* to_insert)
{
    if (header->last == NULL)
    {
        if (header->first != NULL)
            panic("insert_dll_entry_tail() --> header->last was NULL, but non NULL value in header->first");
        header->last = to_insert;
        header->first = to_insert;
        to_insert->next = (struct dll_Entry* )header;
        to_insert->prev = (struct dll_Entry* )header;
    }
    else
    {
        struct dll_Entry* old_last = header->last;
        
        header->last = to_insert;
        to_insert->next = (struct dll_Entry* )header;
        to_insert->prev = old_last;
        old_last->next = to_insert;
    }
}