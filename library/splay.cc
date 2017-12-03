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

#include "splay.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// Create new tree node.
tree* NewNode(unsigned long long key) {
    tree* node = new tree;
    
    if(!node) {
        fprintf(stderr, "Node memory allocation failed!\n");
        exit(1);
    }
    
    node->key = key;
    node->subtree_size = 1;
    node->p = NULL ;
    node->left = NULL;
    node->right = NULL;
    
    return node;
}

// Remove tree node.
void CleanNode(tree* node,unsigned long long key) {
    if (node) {
        node->key = key;
        node->subtree_size = 1;
        node->p = NULL;
        node->left = NULL;
        node->right = NULL;
    }
}

// Right tree rotation.
inline tree* RightRotate(tree* root) {
    tree* left = root->left;
    root->left = left->right;
    
    if (root->left)
        root->left->p = root;
    
    left->right = root;
    
    if (left->right)
        left->right->p = left;
    
    left->p = NULL;
    root->subtree_size = 1;
    
    if (root->left)
        root->subtree_size += root->left->subtree_size;
    if (root->right)
        root->subtree_size += root->right->subtree_size;

    left->subtree_size = 1;
    
    if (left->left)
        left->subtree_size += left->left->subtree_size;
    if (left->right)
        left->subtree_size += left->right->subtree_size;

    return left;
}

// Left tree rotation.
inline tree* LeftRotate(tree* root) {
    tree* right = root->right;
    root->right = right->left;
    
    if (root->right)
        root->right->p = root;
    
    right->left = root;
    
    if (right->left)
        right->left->p = right;
    
    right->p = NULL;

    root->subtree_size = 1;
    
    if (root->left)
        root->subtree_size += root->left->subtree_size;
    if (root->right)
        root->subtree_size += root->right->subtree_size;

    right->subtree_size = 1;
    
    if (right->left)
        right->subtree_size += right->left->subtree_size;
    if (right->right)
        right->subtree_size += right->right->subtree_size;

    return right;
}

// Splay tree implementation.
tree* Splay(unsigned long long key, tree* root) {
    if(!root)
        return NULL;
    
    // Preliminary allocation on the stack 
    // without calling the OS with malloc.
    tree ref[1];
    CleanNode(ref,0);
    tree* t = ref;
    
    tree* ltree = t;
    tree* rtree = t;

    while(1) {
        if(key < root->key) {
            if(!root->left)
                break;
            
            if(key < root->left->key) {
                root = RightRotate(root); 
                if(!root->left)
                    break;
            }

            rtree->left = root;
            if (rtree->left)
                rtree->left->p = rtree;
            
            tree *tmp = rtree;
            rtree = rtree->left;
            root = root->left;
            rtree->left = NULL;
            rtree->subtree_size = 1;
            
            if (rtree->right)
                rtree->subtree_size += rtree->right->subtree_size;

            while (tmp && tmp->key) {
                tmp->subtree_size = 1;
                if (tmp->right)
                    tmp->subtree_size += tmp->right->subtree_size;
                if (tmp->left)
                    tmp->subtree_size += tmp->left->subtree_size;
                tmp = tmp->p;
            }
        } else if(key > root->key) {
            if(!root->right)
                break;
            if(key > root->right->key) {
                root = LeftRotate(root);
                if(!root->right)
                    break;
            }
            
            ltree->right = root;
            if (ltree->right)
                ltree->right->p = ltree;
            tree *tmp = ltree;

            ltree = ltree->right;
            root = root->right;
            ltree->right = NULL;

            ltree->subtree_size = 1;
            if (ltree->left)
                ltree->subtree_size += ltree->left->subtree_size;

            while (tmp && tmp->key) {
                tmp->subtree_size = 1;
                if (tmp->right)
                    tmp->subtree_size += tmp->right->subtree_size;
                if (tmp->left)
                    tmp->subtree_size += tmp->left->subtree_size;
                tmp = tmp->p;
            }

        } else
            break;
    }

    tree *tmp = NULL;
    ltree->right = root->left;
    tmp = ltree->right;
    
    if (ltree->right) {
        ltree->right->p = ltree;

        while (tmp && tmp->key) {
            tmp->subtree_size = 1;
            
            if (tmp->right)
                tmp->subtree_size += tmp->right->subtree_size;
            if (tmp->left)
                tmp->subtree_size += tmp->left->subtree_size;

            tmp = tmp->p;
        }
    }

    rtree->left = root->right;
    tmp = rtree->left;
    
    if (rtree->left) {
        rtree->left->p = rtree;

        while (tmp && tmp->key) {
            tmp->subtree_size = 1;
            
            if (tmp->right)
                tmp->subtree_size += tmp->right->subtree_size;
            if (tmp->left)
                tmp->subtree_size += tmp->left->subtree_size;

            tmp = tmp->p;
        }
    }

    root->left = t->right;
    root->right = t->left;
    root->p = NULL;
    
    if (root->left)
        root->left->p = root;
    if (root->right)
        root->right->p = root;

    root->subtree_size = 1;
    if (root->left) 
        root->subtree_size += root->left->subtree_size;
    if (root->right)
        root->subtree_size += root->right->subtree_size;

    return root;
}

// Insert key implementation.
tree* Insert(unsigned long long key, tree* root) {
    tree* node = NewNode(key);

    if(!root) {
        root = node;
        node = NULL;
        return root;
    }
    
    root = Splay(key, root);
   
    if(key < root->key) {
        node->left = root->left;
        
        if (node->left)
            node->left->p = node;

        node->right = root;
        
        if (node->right)
            node->right->p = node;

        root->left = NULL;
        node->subtree_size = root->subtree_size + 1;
        
        if (node->left)
            root->subtree_size = root->subtree_size - node->left->subtree_size;

        root = node;
    } else if(key > root->key) {
        node->right = root->right;
        
        if (node->right)
            node->right->p = node;

        node->left = root;
        
        if (node->left)
            node->left->p = node;

        root->right = NULL;
        node->subtree_size = root->subtree_size + 1;
        
        if (node->right)
            root->subtree_size = root->subtree_size - node->right->subtree_size;

        root = node;
    } else {
        delete node;
        return root;
    }

    node = NULL;
    return root;
    
}

// Delete key implementation.
tree* Delete(unsigned long long key, tree* root) {
    tree* tmp;
    
    if(!root)
        return NULL;
    
    root = Splay(key, root);
    
    if(key != root->key)
        return root;
    else {
        if(!root->left) {
            tmp = root;
            root = root->right;
        } else {
            tmp = root;
            root = Splay(key, root->left);
            root->right = tmp->right;
            
            if (root->right)
                root->right->p = root;

            root->subtree_size = 1;
            
            if (root->right)
                root->subtree_size += root->right->subtree_size;
            if (root->left)
                root->subtree_size += root->left->subtree_size;
        }
        
        free(tmp);
        return root;
    }
}

void DeleteAll(tree* root) {
    if (!root) 
        return;

    if (root->right)
        DeleteAll(root->right);

    if (root->left)
        DeleteAll(root->left);

    delete root;
}

unsigned long long TreeSize(tree* root) {
    if (root == NULL) 
        return 0;
    else 
        return TreeSize(root->right) + TreeSize(root->left) + 1;
}

// Update tree implementation.
tree* Update(unsigned long long previousKey, unsigned long long newKey, tree* root) {
    // First similar to the Delete function, 
    // but do not free the node, reuse it later as in Insert.
    tree* tmp;
    
    if(!root)
        tmp = NewNode(newKey);
    else {
        root = Splay(previousKey, root);
        
        if(previousKey != root->key)
            tmp = NewNode(newKey);
        else {
            if(!root->left) {
                tmp = root;
                root = root->right;
            } else {
                tmp = root;
                root = Splay(previousKey, root->left);
                root->right = tmp->right;
                
                if (root->right)
                    root->right->p = root;

                root->subtree_size = 1;
                
                if (root->right)
                    root->subtree_size += root->right->subtree_size;
                if (root->left)
                    root->subtree_size += root->left->subtree_size;
            }
            CleanNode(tmp, newKey);
        }
    }

    // Now insert but using the pre-allocated tmp variable.
    tree* node = tmp;

    if(!root) {
        root = node;
        node = NULL;
        return root;
    }
    
    root = Splay(newKey, root);
    
    if(newKey < root->key) {
        node->left = root->left;
        
        if (node->left)
            node->left->p = node;

        node->right = root;
        
        if (node->right)
            node->right->p = node;

        root->left = NULL;
        node->subtree_size = root->subtree_size + 1;
        
        if (node->left)
            root->subtree_size = root->subtree_size - node->left->subtree_size;

        root = node;
    } else if(newKey > root->key) {
        node->right = root->right;
        
        if (node->right)
            node->right->p = node;

        node->left = root;
        
        if (node->left)
            node->left->p = node;

        root->right = NULL;
        node->subtree_size = root->subtree_size + 1;
        
        if (node->right)
            root->subtree_size = root->subtree_size - node->right->subtree_size;

        root = node;
    } else {
        delete tmp;
        return root;
    }
    
    node = NULL;
    return root;
}

tree* ComputeDistance(unsigned long long key, tree *root, unsigned long long *distance) {
    tree *tmp = Splay(key, root);

    if (tmp && tmp->key == key && tmp->right)
        *distance = tmp->right->subtree_size;
    else
        *distance = 0;

    return tmp;
}

tree* Find(unsigned long long key, tree* root) {
    return Splay(key, root);
}
