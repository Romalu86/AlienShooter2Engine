#pragma once

#include "basetype.h"
#include "exports.h"
#include <new>

// Native ordered-tree operations over each owner's actual node and field types.
// Node accessors do not add fields or virtual slots.
namespace TreeOperations
{
template<class Node>
void RotateLeft(Node*& head, Node* current, Node* const& nilOwner)
{
    Node* const pivot = current->RightLink();
    current->RightLink() = pivot->LeftLink();
    Node* const child = pivot->LeftLink();
    if (child != nilOwner)
        child->ParentLink() = current;
    pivot->ParentLink() = current->ParentLink();
    Node* const header = head;
    if (current == header->ParentLink())
        header->ParentLink() = pivot;
    else
    {
        Node* const parent = current->ParentLink();
        if (current == parent->LeftLink())
            parent->LeftLink() = pivot;
        else
            parent->RightLink() = pivot;
    }
    pivot->LeftLink() = current;
    current->ParentLink() = pivot;
}

template<class Node>
void RotateRight(Node*& head, Node* current, Node* const& nilOwner)
{
    Node* const pivot = current->LeftLink();
    current->LeftLink() = pivot->RightLink();
    Node* const child = pivot->RightLink();
    if (child != nilOwner)
        child->ParentLink() = current;
    pivot->ParentLink() = current->ParentLink();
    Node* const header = head;
    if (current == header->ParentLink())
        header->ParentLink() = pivot;
    else
    {
        Node* const parent = current->ParentLink();
        if (current == parent->RightLink())
            parent->RightLink() = pivot;
        else
            parent->LeftLink() = pivot;
    }
    pivot->RightLink() = current;
    current->ParentLink() = pivot;
}

template<class Node>
Node* Successor(Node* current, const Node* sentinel)
{
    Node* const right = current->RightLink();
    if (right != sentinel)
    {
        current = right;
        while (current->LeftLink() != sentinel)
            current = current->LeftLink();
        return current;
    }
    Node* parent = current->ParentLink();
    while (current == parent->RightLink())
    {
        current = parent;
        parent = parent->ParentLink();
    }
    if (current->RightLink() != parent)
        current = parent;
    return current;
}

template<class Node>
Node* Minimum(Node* current, const Node* sentinel)
{
    while (current->LeftLink() != sentinel)
        current = current->LeftLink();
    return current;
}

template<class Node>
Node* Maximum(Node* current, const Node* sentinel)
{
    while (current->RightLink() != sentinel)
        current = current->RightLink();
    return current;
}

template<class Node>
Node* Predecessor(Node* current, Node* const& nilOwner)
{
    if (current->ColorValue() == 0u &&
        current->ParentLink()->ParentLink() == current)
        return current->RightLink();
    Node* const left = current->LeftLink();
    Node* const sentinel = nilOwner;
    if (left != sentinel)
    {
        current = left;
        while (current->RightLink() != sentinel)
            current = current->RightLink();
        return current;
    }
    Node* parent = current->ParentLink();
    while (current == parent->LeftLink())
    {
        current = parent;
        parent = parent->ParentLink();
    }
    return parent;
}

template<class Node, class ConstructValue, class LessThanParent>
Node* Insert(Node*& head, unsigned int& size, Node* hintNode, Node* parent,
    Node* const& nilOwner, ConstructValue constructValue, LessThanParent lessThanParent)
{
    Node* const node = ::new (CrtNew(sizeof(Node))) Node;
    node->ParentLink() = parent;
    node->ColorValue() = 0u;
    node->LeftLink() = nilOwner;
    node->RightLink() = nilOwner;
    constructValue(node);

    Node* header = head;
    ++size;
    const bool putLeft = parent == header || hintNode != nilOwner || lessThanParent(parent);
    if (putLeft)
    {
        parent->LeftLink() = node;
        header = head;
        if (parent == header)
        {
            header->ParentLink() = node;
            head->RightLink() = node;
        }
        else if (parent == header->LeftLink())
            header->LeftLink() = node;
    }
    else
    {
        parent->RightLink() = node;
        header = head;
        if (parent == header->RightLink()) header->RightLink() = node;
    }
    Node* current = node;
    while (current != head->ParentLink() &&
          current->ParentLink()->ColorValue() == 0u)
    {
        Node* parentNode = current->ParentLink();
        Node* grandparentNode = parentNode->ParentLink();
        if (parentNode == grandparentNode->LeftLink())
        {
            Node* uncle = grandparentNode->RightLink();
            if (uncle->ColorValue() == 0u)
            {
                parentNode->ColorValue() = 1u;
                uncle->ColorValue() = 1u;
                current->ParentLink()->ParentLink()->ColorValue() = 0u;
                current = current->ParentLink()->ParentLink();
            }
            else
            {
                if (current == parentNode->RightLink())
                {
                    current = parentNode;
                    RotateLeft(head, current, nilOwner);
                }
                current->ParentLink()->ColorValue() = 1u;
                current->ParentLink()->ParentLink()->ColorValue() = 0u;
                grandparentNode = current->ParentLink()->ParentLink();
                RotateRight(head, grandparentNode, nilOwner);
            }
        }
        else
        {
            Node* uncle = grandparentNode->LeftLink();
            if (uncle->ColorValue() == 0u)
            {
                parentNode->ColorValue() = 1u;
                uncle->ColorValue() = 1u;
                current->ParentLink()->ParentLink()->ColorValue() = 0u;
                current = current->ParentLink()->ParentLink();
            }
            else
            {
                if (current == parentNode->LeftLink())
                {
                    current = parentNode;
                    RotateRight(head, current, nilOwner);
                }
                current->ParentLink()->ColorValue() = 1u;
                current->ParentLink()->ParentLink()->ColorValue() = 0u;
                grandparentNode = current->ParentLink()->ParentLink();
                RotateLeft(head, grandparentNode, nilOwner);
            }
        }
    }
    head->ParentLink()->ColorValue() = 1u;
    return node;
}

template<class Node>
Node* Erase(Node*& head, unsigned int& size, Node* erasedNode, Node* const& nilOwner)
{
    Node* const next = Successor(erasedNode, nilOwner);
    Node* const nil = nilOwner;

    Node* removedNode = erasedNode;
    Node* replacementChild = nil;

    if (removedNode->LeftLink() == nil)
        replacementChild = removedNode->RightLink();
    else if (removedNode->RightLink() == nil)
        replacementChild = removedNode->LeftLink();
    else
    {
        removedNode = removedNode->RightLink();
        while (removedNode->LeftLink() != nil)removedNode = removedNode->LeftLink();
        replacementChild = removedNode->RightLink();
    }

    unsigned int runtimeGuard = 0u;
    (void)CrtStaticLockConstruct(PtrToU32(&runtimeGuard));

    if (removedNode != erasedNode)
    {
        erasedNode->LeftLink()->ParentLink() = removedNode;
        removedNode->LeftLink() = erasedNode->LeftLink();

        if (removedNode != erasedNode->RightLink())
        {
            replacementChild->ParentLink() = removedNode->ParentLink();
            removedNode->ParentLink()->LeftLink() = replacementChild;
            removedNode->RightLink() = erasedNode->RightLink();
            erasedNode->RightLink()->ParentLink() = removedNode;
        }
        else
        {
            replacementChild->ParentLink() = removedNode;
        }

        Node* const header = head;
        if (header->ParentLink() == erasedNode)
            header->ParentLink() = removedNode;
        else
        {
            Node* const erasedParent = erasedNode->ParentLink();
            if (erasedParent->LeftLink() == erasedNode)
                erasedParent->LeftLink() = removedNode;
            else
                erasedParent->RightLink() = removedNode;
        }
        removedNode->ParentLink() = erasedNode->ParentLink();

        const unsigned int erasedColor = erasedNode->ColorValue();
        const unsigned int replacementColor = removedNode->ColorValue();
        removedNode->ColorValue() = erasedColor;
        erasedNode->ColorValue() = replacementColor;
        removedNode = erasedNode;
    }
    else
    {
        replacementChild->ParentLink() = removedNode->ParentLink();
        Node* header = head;
        if (header->ParentLink() == erasedNode)
            header->ParentLink() = replacementChild;
        else
        {
            Node* const erasedParent = erasedNode->ParentLink();
            if (erasedParent->LeftLink() == erasedNode)
                erasedParent->LeftLink() = replacementChild;
            else
                erasedParent->RightLink() = replacementChild;
        }

        header = head;
        if (header->LeftLink() == erasedNode)
        {
            Node* const sentinel = nilOwner;
            if (erasedNode->RightLink() == sentinel)
                header->LeftLink() = erasedNode->ParentLink();
            else
                header->LeftLink() = Minimum(replacementChild, sentinel);
        }
        header = head;
        if (header->RightLink() == erasedNode)
        {
            Node* const sentinel = nilOwner;
            if (erasedNode->LeftLink() == sentinel)
                header->RightLink() = erasedNode->ParentLink();
            else
                header->RightLink() = Maximum(replacementChild, sentinel);
        }
    }

    if (removedNode->ColorValue() == 1u)
    {
        while (replacementChild != head->ParentLink() && replacementChild->ColorValue() == 1u)
        {
            if (replacementChild == replacementChild->ParentLink()->LeftLink())
            {
                Node* sibling = replacementChild->ParentLink()->RightLink();
                if (sibling->ColorValue() == 0u)
                {
                    sibling->ColorValue() = 1u;
                    replacementChild->ParentLink()->ColorValue() = 0u;
                    RotateLeft(head, replacementChild->ParentLink(), nilOwner);
                    sibling = replacementChild->ParentLink()->RightLink();
                }
                if (sibling->LeftLink()->ColorValue() == 1u &&
                   sibling->RightLink()->ColorValue() == 1u)
                {
                    sibling->ColorValue() = 0u;
                    replacementChild = replacementChild->ParentLink();
                }
                else
                {
                    if (sibling->RightLink()->ColorValue() == 1u)
                    {
                        sibling->LeftLink()->ColorValue() = 1u;
                        sibling->ColorValue() = 0u;
                        RotateRight(head, sibling, nilOwner);
                        sibling = replacementChild->ParentLink()->RightLink();
                    }
                    sibling->ColorValue() = replacementChild->ParentLink()->ColorValue();
                    replacementChild->ParentLink()->ColorValue() = 1u;
                    sibling->RightLink()->ColorValue() = 1u;
                    RotateLeft(head, replacementChild->ParentLink(), nilOwner);
                    break;
                }
            }
            else
            {
                Node* sibling = replacementChild->ParentLink()->LeftLink();
                if (sibling->ColorValue() == 0u)
                {
                    sibling->ColorValue() = 1u;
                    replacementChild->ParentLink()->ColorValue() = 0u;
                    RotateRight(head, replacementChild->ParentLink(), nilOwner);
                    sibling = replacementChild->ParentLink()->LeftLink();
                }
                if (sibling->RightLink()->ColorValue() == 1u &&
                   sibling->LeftLink()->ColorValue() == 1u)
                {
                    sibling->ColorValue() = 0u;
                    replacementChild = replacementChild->ParentLink();
                }
                else
                {
                    if (sibling->LeftLink()->ColorValue() == 1u)
                    {
                        sibling->RightLink()->ColorValue() = 1u;
                        sibling->ColorValue() = 0u;
                        RotateLeft(head, sibling, nilOwner);
                        sibling = replacementChild->ParentLink()->LeftLink();
                    }
                    sibling->ColorValue() = replacementChild->ParentLink()->ColorValue();
                    replacementChild->ParentLink()->ColorValue() = 1u;
                    sibling->LeftLink()->ColorValue() = 1u;
                    RotateRight(head, replacementChild->ParentLink(), nilOwner);
                    break;
                }
            }
        }
        replacementChild->ColorValue() = 1u;
    }

    (void)CrtStaticLockDestruct(PtrToU32(&runtimeGuard));
    CrtFreePointer(removedNode);
    size = size-1u;
    return next;
}

}
