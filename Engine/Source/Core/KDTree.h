#ifndef __Y_YMAIN_KDTREE_H
#define __Y_YMAIN_KDTREE_H

#include "ymain/Common.h"
#include "ymain/Vector3.h"
#include "ymain/AABox.h"

Y_NAMESPACE_BEGIN

template<class T>
class KDTree
{
    // public methods
public:
    KDTree()
    {
        // allocate root node
        m_root = createNewNode(AABox::infinite);
    }

    ~KDTree()
    {
        // delete root node, this will delete all other nodes
        deleteNode(m_root);
    }

    // insert a member into the tree
    void insert(T m, const AABox &bounds)
    {
        // find node
        Node *node = findDeepestNode(bounds);

        // create member
        Member member;
        member.val = m;
        member.node = node;
        member.bounds = bounds;

        // store in m_members
        m_members.push_back(&member);

        // small hack to get a pointer to the list's allocated data
        Member *memberPtr = &(*m_members.back());

        // store in the node's members
        node->members.push_back(memberPtr);        
    }

    // remove a member from the tree
    void remove(T m)
    {
        // find in the member list
        MemberList::iterator itr = std::find(m_members.begin(), m_members.end(), m);
        Y_VerifyMsg(itr != m_members.end(), "Trying to remove a non-member from KDTree");

        // dereference the iterator
        Member &member = *itr;

        // find in the node list
        MemberPtrList::iterator pitr = std::find(member.node->members.begin(), member.node->members.end(), &member);
        Y_VerifyMsg(pitr != member.node->members.end(), "Internal KDTree corruption");

        // remove from the node's member list
        member.node->members.erase(pitr);

        // remove from the main member list
        m_members.erase(itr);
    }

    // enable/disable automatic splitting. 

    // get the size of the tree
    inline size_t getSize() const { return m_members.size(); }

    // private classes
private:
    // forward declare node class
    class Node;

    // member type
    struct Member
    {
        T val;                  // actual member
        AABox bounds;           // we cache the bounds here for faster access
        Node *node;             // node this member is located in
    };

    // list types
    typedef std::list<Member> MemberList;
    typedef std::list<Member *> MemberPtrList;

    // node type
    struct Node
    {
        AABox bounds;           // bounds of node
        MemberPtrList members;  // objects in node
        Node *children[2];      // children of node
        float splitLocation;    // distance along split plane that split occurred
        uint8 splitAxis;        // axis that we split along

        // quickly determine if the node is a leaf node
        inline bool isLeaf() const { return (children[0] != NULL); }
    };

    // private methods
private:
    // find the deepest node that would contain the specified box
    // it may not necessarily return a 
    Node *findDeepestNode(const AABox &box)
    {
        Node *node = m_root;
        while (1)
        {
            if (node->isLeaf())
            {
                // node is a leaf node, so we can't go any deeper.
                return node;
            }

            if (box.GetHigh()[node->splitAxis] < node->splitLocation)
            {
                // bounds finish before the splitting plane, recurse into this node
                node = node->children[0];
            }
            else if (box.GetLow()[node->splitAxis] > node->splitLocation)
            {
                // bounds start after splitting plane, recurse into right child
                node = node->children[1];
            }
            else
            {
                // bounds are inside this node, but overlap the splitting plane
                // we will return this node.
                return node;
            }
        }
    }

    // create a new node
    Node *createNewNode(AABox &bounds)
    {
        Node *node = new Node;
        node->splitAxis = 0;
        node->splitLocation = Y_finf();
        node->bounds = bounds;
        node->children[0] = NULL;
        node->children[1] = NULL;
        return node;
    }

    // delete a node and all its children recursively
    Node *deleteNode(Node *node)
    {
        if (node->children[0] != NULL)
            deleteNode(node->children[0]);
        if (node->children[1] != NULL)
            deleteNode(node->children[1]);

        delete node;
    }

    // private variables
private:
    Node *m_root;
    MemberList m_members;
    bool m_autoSplit;
    uint32 m_splitThreshold;
};

Y_NAMESPACE_END

#endif              // __Y_YMAIN_KDTREE_H

