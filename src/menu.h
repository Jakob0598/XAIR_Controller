#ifndef MENU_H
#define MENU_H

#define MENU_MAX_CHILDREN 8

struct MenuItem
{
    const char* name;

    MenuItem* parent;

    MenuItem* children[MENU_MAX_CHILDREN];
    uint8_t childCount;

    void (*action)();
};

void menuInit();

void menuNext();
void menuPrev();
void menuEnter();
void menuBack();

MenuItem* menuCurrent();

#endif