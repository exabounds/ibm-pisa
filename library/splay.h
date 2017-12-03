/*******************************************************************************
 * (C) Copyright IBM Corporation 2017
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    IBM Algorithms & Machines team
 *******************************************************************************/ 

#ifndef __SPLAY_H__
#define __SPLAY_H__

typedef struct tree {
    unsigned long long key;
    unsigned long long subtree_size;
    struct tree* left;
    struct tree* right;
    struct tree* p;
} tree;

tree* NewNode(unsigned long long key);
void CleanNode(tree* root, unsigned long long key);
tree* Splay(unsigned long long key, tree* root);
tree* Insert(unsigned long long key, tree* root);
tree* Update(unsigned long long previousKey, unsigned long long newKey, tree* root);
inline tree* RightRotate(tree* root);
inline tree* LeftRotate(tree* root);
tree* Delete(unsigned long long key, tree* root);
void DeleteAll(tree* root);
tree* Find(unsigned long long key, tree* root);
tree* ComputeDistance(unsigned long long key, tree *root, unsigned long long *distance);
unsigned long long TreeSize(tree* root);

#endif
