#include "config.h"

static MenuItem* current = nullptr;

void addChild(MenuItem* parent, MenuItem* child)
{
    parent->children[parent->childCount++] = child;
    child->parent = parent;
}

MenuItem root;
MenuItem channels;
MenuItem meters;
MenuItem scenes;
MenuItem settings;

void menuInit()
{
    root = {"HOME", nullptr, {}, 0, nullptr};

    channels = {"CHANNEL", &root, {}, 0, nullptr};
    meters   = {"METERS", &root, {}, 0, nullptr};
    scenes   = {"SCENES", &root, {}, 0, nullptr};
    settings = {"SETTINGS", &root, {}, 0, nullptr};

    addChild(&root,&channels);
    addChild(&root,&meters);
    addChild(&root,&scenes);
    addChild(&root,&settings);

    current = &root;
}

MenuItem* menuCurrent()
{
    return current;
}

void menuNext()
{
    if (!current->parent) return;

    MenuItem* parent = current->parent;

    for(int i=0;i<parent->childCount;i++)
    {
        if(parent->children[i] == current)
        {
            current = parent->children[(i+1)%parent->childCount];
            return;
        }
    }
}

void menuPrev()
{
    if (!current->parent) return;

    MenuItem* parent = current->parent;

    for(int i=0;i<parent->childCount;i++)
    {
        if(parent->children[i] == current)
        {
            int idx = i-1;

            if(idx < 0)
                idx = parent->childCount-1;

            current = parent->children[idx];
            return;
        }
    }
}

void menuEnter()
{
    if(current->childCount > 0)
    {
        current = current->children[0];
        return;
    }

    if(current->action)
        current->action();
}

void menuBack()
{
    if(current->parent)
        current = current->parent;
}