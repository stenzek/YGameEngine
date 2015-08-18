#ifndef __Y_YMAIN_BSPTREE_H
#define __Y_YMAIN_BSPTREE_H

#include "ymain/Common.h"

// till custom classes are added
#include <list>

Y_NAMESPACE_BEGIN

template<class T>
struct BSPTreeBoundsTrait
{
    inline static void GetBounds(const T &);
}

template<class T, class BOUNDSCLASS> 
class BSPTree
{
public:
    BSPTree()
    {
        m_root = NULL;
    }

    ~BSPTree()
    {

    }

    // remove all members from bsp tree
    void Clear()
    {
        if (m_root != NULL)
        {
            _Clear(m_root);
            m_root = NULL;
        }

        m_members.clear();
    }



private:
    // recursive clear
    void _Clear(Node *node)
    {
        // clear children
        if (node->child[0] != NULL)
            _Clear(node->child[0]);
        if (node->child[1] != NULL)
            _Clear(node->child[1]);
        
        // clear this ndoe
        delete node;
    }

    // forward declare internal types
    class Node;
    class Member;
    typedef std::list<Member *> MemberPtrList;
    typedef std::list<Member> MemberList;

    // member type
    struct Member
    {
        Node *node;
        T value;
    };

    // node type
    class Node
    {
        uint8 splitAxis;
        float splitDistance;
        Node *child[2];
        MemberPtrList members;
    };

    // root node pointer
    Node *m_root;

    // all members
    MemberList m_members;
};