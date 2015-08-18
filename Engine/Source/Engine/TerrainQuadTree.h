#pragma once
#include "Engine/Common.h"
#include "Engine/TerrainTypes.h"
#include "Engine/TerrainLayerList.h"

class BinaryWriter;
class TerrainManager;
class TerrainSection;
class TerrainQuadTreeNode;
class TerrainQuadTreeQuery;

class TerrainSectionQuadTree
{
public:
    friend class TerrainQuadTreeNode;
    friend class TerrainQuadTreeQuery;

public:
    TerrainSectionQuadTree(const TerrainSection *pSection);
    ~TerrainSectionQuadTree();

    const TerrainSection *GetSection() const { return m_pSection; }
    uint32 GetLODCount() const { return m_LODCount; }

    uint32 GetNodeCount() const { return m_nNodes; }

    const TerrainQuadTreeNode *GetTopLevelNode() const { return m_pNodes; }

    // building
    static bool Build(const TerrainParameters *pParameters, const TerrainSection *pSection, ByteStream *pOutputStream, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // loading
    bool LoadFromStream(ByteStream *pStream);

    // updating. if oldHeight is unknown, Y_FLT_INFINITE can be passed
    void UpdateMinMaxHeight(uint32 pointX, uint32 pointY, float newHeight, float oldHeight, bool *pNeedsRebuild);

    // saving
    bool SaveToStream(ByteStream *pOutputStream) const;

    // enumerate overlapping nodes
    template<typename CALLBACK_TYPE>
    void EnumerateNodesOverlappingBox(const AABox &searchBounds, uint32 maxLODLevel, CALLBACK_TYPE callback) const;

    // ray cast into quadtree
    template<typename CALLBACK_TYPE>
    void EnumerateNodesIntersectingRay(const Ray &ray, uint32 maxLODLevel, CALLBACK_TYPE callback) const;

    // find sections to render with callback
    template<typename CALLBACK_TYPE>
    void EnumerateNodesToRender(const float3 &cameraPosition, const Frustum &cameraFrustum, const float visibilityRanges[TERRAIN_MAX_RENDER_LODS], CALLBACK_TYPE callback) const;

private:
    // building
    static void BuildTopLevel(TerrainQuadTreeNode *pNodeBuffer, uint32 &currentNodeIndex, uint32 allocatedNodeCount, const TerrainParameters *pParameters, const TerrainSection *pSection, ProgressCallbacks *pProgressCallbacks);
    static void BuildChildrenRecursive(TerrainQuadTreeNode *pNodeBuffer, uint32 &currentNodeIndex, uint32 allocatedNodeCount, TerrainQuadTreeNode *pSplittingNode, const TerrainParameters *pParameters, const TerrainSection *pSection, ProgressCallbacks *pProgressCallbacks);
    static bool SaveNodeBuffer(ByteStream *pStream, TerrainQuadTreeNode *pNodeBuffer, uint32 nodeCount, uint32 LODCount);

    // updating
    void UpdateMinMaxHeightRecursive(TerrainQuadTreeNode *pNode, uint32 pointX, uint32 pointY, float newHeight, float oldHeight, bool &needsRebuild);

    template<typename CALLBACK_TYPE>
    void RecursiveEnumerateNodesOverlappingBox(const AABox &searchBounds, uint32 maxLODLevel, CALLBACK_TYPE callback, const TerrainQuadTreeNode *pNode, bool parentCompletelyInBox) const;

    template<typename CALLBACK_TYPE>
    void RecursiveEnumerateNodesIntersectingRay(const Ray &ray, uint32 maxLODLevel, CALLBACK_TYPE callback, const TerrainQuadTreeNode *pNode) const;

    template<typename CALLBACK_TYPE>
    bool RecursiveEnumerateNodesToRender(const float3 &cameraPosition, const Frustum &cameraFrustum, const float visibilityRanges[TERRAIN_MAX_RENDER_LODS], CALLBACK_TYPE callback, const TerrainQuadTreeNode *pNode, bool parentCompletelyInFrustum) const;
    
private:
    const TerrainSection *m_pSection;
    uint32 m_LODCount;

    TerrainQuadTreeNode *m_pNodes;
    uint32 m_nNodes;
};

class TerrainQuadTreeNode
{
    friend class TerrainSectionQuadTree;

public:
    enum ChildReference
    {
        ChildReferenceTopLeft,
        ChildReferenceTopRight,
        ChildReferenceBottomLeft,
        ChildReferenceBottomRight,
    };

public:
    const uint32 GetLODLevel() const { return m_LODLevel; }

    const uint32 GetStartQuadX() const { return m_startQuadX; }
    const uint32 GetStartQuadY() const { return m_startQuadY; }
    const uint32 GetNodeSize() const { return m_nodeSize; }

    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const Sphere &GetBoundingSphere() const { return m_boundingSphere; }

    const bool IsLeafNode() const { return m_isLeafNode; }
    const bool IsFlat() const { return m_isFlat; }

    const TerrainQuadTreeNode *GetChildNode(uint32 child) const { DebugAssert(child < 4); return m_pChildNodes[child]; }

private:
    // arrangement is strange yes, but it's best for memory usage (struct packing-wise) this way
    
    // child nodes
    TerrainQuadTreeNode *m_pChildNodes[4];

    // bounds of this node
    AABox m_boundingBox;
    Sphere m_boundingSphere;

    // node parameters
    uint32 m_LODLevel;

    // point indices
    uint32 m_startQuadX;
    uint32 m_startQuadY;

    // node size in *quads*
    uint32 m_nodeSize;

    // leaf node flag
    bool m_isLeafNode;

    // reduce to single quad flag
    bool m_isFlat;
};

class TerrainQuadTreeQuery
{
public:
    enum DRAW_FLAGS
    {
        DrawFlagTopLeft          = (1 << 0),
        DrawFlagTopRight         = (1 << 1),
        DrawFlagBottomLeft       = (1 << 2),
        DrawFlagBottomRight      = (1 << 3),
        DrawFlagSingleQuad       = (1 << 4),
        DrawFlagAll              = (DrawFlagTopLeft | DrawFlagTopRight | DrawFlagBottomLeft | DrawFlagBottomRight),
    };

    struct SelectedNode
    {
        const TerrainSection *pSection;
        const TerrainQuadTreeNode *pNode;
        uint32 DrawFlags;
    };

public:
    TerrainQuadTreeQuery();
    ~TerrainQuadTreeQuery();

    void Invoke(const TerrainSection *const *ppSections, uint32 nSections, const float3 &cameraPosition, const Frustum &cameraFrustum, const float visibilityRanges[TERRAIN_MAX_RENDER_LODS]);

    const SelectedNode &GetSelectedNode(uint32 i) const { return m_selectedNodes[i]; }
    const uint32 GetSelectedNodeCount() const { return m_selectedNodes.GetSize(); }

private:
    typedef MemArray<SelectedNode> SelectedNodeArray;
    SelectedNodeArray m_selectedNodes;

    bool ProcessNode(const float3 &cameraPosition, const Frustum &cameraFrustum, const float visibilityRanges[TERRAIN_MAX_RENDER_LODS], const TerrainSection *pSection, const TerrainQuadTreeNode *pNode, bool parentCompletelyInFrustum);
};

template<typename CALLBACK_TYPE>
void TerrainSectionQuadTree::EnumerateNodesOverlappingBox(const AABox &searchBounds, uint32 maxLODLevel, CALLBACK_TYPE callback) const
{
    // start recursion
    const TerrainQuadTreeNode *pTopLevelNode = m_pNodes;
    if (pTopLevelNode->GetLODLevel() >= maxLODLevel)
        RecursiveEnumerateNodesOverlappingBox<CALLBACK_TYPE>(searchBounds, maxLODLevel, callback, pTopLevelNode);
}

template<typename CALLBACK_TYPE>
void TerrainSectionQuadTree::RecursiveEnumerateNodesOverlappingBox(const AABox &searchBounds, uint32 maxLODLevel, CALLBACK_TYPE callback, const TerrainQuadTreeNode *pNode, bool parentCompletelyInBox) const
{
    bool thisCompletelyInBox = parentCompletelyInBox;
    if (!parentCompletelyInBox)
    {
        thisCompletelyInBox = pNode->GetBoundingBox().ContainsAABox(searchBounds);
        if (!thisCompletelyInBox && !pNode->GetBoundingBox().AABoxIntersection(searchBounds))
            return;
    }

    // stopping at this level?
    if (pNode->IsLeafNode() || maxLODLevel == pNode->GetLODLevel())
    {
        callback(pNode);
        return;
    }

    // test children
    const TerrainQuadTreeNode *pChildNode;
    if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceTopLeft)) != NULL)
        RecursiveEnumerateNodesOverlappingBox<CALLBACK_TYPE>(searchBounds, maxLODLevel, callback, pChildNode, thisCompletelyInBox);
    if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceTopRight)) != NULL)
        RecursiveEnumerateNodesOverlappingBox<CALLBACK_TYPE>(searchBounds, maxLODLevel, callback, pChildNode, thisCompletelyInBox);
    if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceBottomLeft)) != NULL)
        RecursiveEnumerateNodesOverlappingBox<CALLBACK_TYPE>(searchBounds, maxLODLevel, callback, pChildNode, thisCompletelyInBox);
    if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceBottomRight)) != NULL)
        RecursiveEnumerateNodesOverlappingBox<CALLBACK_TYPE>(searchBounds, maxLODLevel, callback, pChildNode, thisCompletelyInBox);
}

template<typename CALLBACK_TYPE>
void TerrainSectionQuadTree::EnumerateNodesIntersectingRay(const Ray &ray, uint32 maxLODLevel, CALLBACK_TYPE callback) const
{
    // start recursion
    const TerrainQuadTreeNode *pTopLevelNode = m_pNodes;
    if (pTopLevelNode->GetLODLevel() >= maxLODLevel)
        RecursiveEnumerateNodesIntersectingRay<CALLBACK_TYPE>(ray, maxLODLevel, callback, pTopLevelNode);
}

template<typename CALLBACK_TYPE>
void TerrainSectionQuadTree::RecursiveEnumerateNodesIntersectingRay(const Ray &ray, uint32 maxLODLevel, CALLBACK_TYPE callback, const TerrainQuadTreeNode *pNode) const
{
    if (!ray.AABoxIntersection(pNode->GetBoundingBox()))
        return;

    // stopping at this level?
    if (pNode->IsLeafNode() || maxLODLevel == pNode->GetLODLevel())
    {
        callback(pNode);
        return;
    }

    // test children
    const TerrainQuadTreeNode *pChildNode;
    if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceTopLeft)) != NULL)
        RecursiveEnumerateNodesIntersectingRay<CALLBACK_TYPE>(ray, maxLODLevel, callback, pChildNode);
    if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceTopRight)) != NULL)
        RecursiveEnumerateNodesIntersectingRay<CALLBACK_TYPE>(ray, maxLODLevel, callback, pChildNode);
    if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceBottomLeft)) != NULL)
        RecursiveEnumerateNodesIntersectingRay<CALLBACK_TYPE>(ray, maxLODLevel, callback, pChildNode);
    if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceBottomRight)) != NULL)
        RecursiveEnumerateNodesIntersectingRay<CALLBACK_TYPE>(ray, maxLODLevel, callback, pChildNode);
}

template<typename CALLBACK_TYPE>
void TerrainSectionQuadTree::EnumerateNodesToRender(const float3 &cameraPosition, const Frustum &cameraFrustum, const float visibilityRanges[TERRAIN_MAX_RENDER_LODS], CALLBACK_TYPE callback) const
{
    // frustum intersect the top level node
    const TerrainQuadTreeNode *pTopLevelNode = GetTopLevelNode();
    Frustum::IntersectionType intersectionType = cameraFrustum.AABoxIntersectionType(pTopLevelNode->GetBoundingBox());
    if (intersectionType != Frustum::INTERSECTION_TYPE_OUTSIDE)
    {
        bool completelyInFrustum = (intersectionType == Frustum::INTERSECTION_TYPE_INTERSECTS);
        RecursiveEnumerateNodesToRender<CALLBACK_TYPE>(cameraPosition, cameraFrustum, visibilityRanges, callback, pTopLevelNode, completelyInFrustum);
    }
}

template<typename CALLBACK_TYPE>
bool TerrainSectionQuadTree::RecursiveEnumerateNodesToRender(const float3 &cameraPosition, const Frustum &cameraFrustum, const float visibilityRanges[TERRAIN_MAX_RENDER_LODS], CALLBACK_TYPE callback, const TerrainQuadTreeNode *pNode, bool parentCompletelyInFrustum) const
{
    // frustum test it
    Frustum::IntersectionType intersectionType;
    if (parentCompletelyInFrustum)
    {
        // shortcut when parent is entirely in frustum
        intersectionType = Frustum::INTERSECTION_TYPE_INSIDE;
    }
    else
    {
        // have to calculate box and test it
        intersectionType = cameraFrustum.AABoxIntersectionType(pNode->GetBoundingBox());
    }

    // outside frustum?
    if (intersectionType == Frustum::INTERSECTION_TYPE_OUTSIDE)
    {
        // return true to consume the node, preventing the parent from drawing it
        return true;
    }

    // out of range for this lod level?
    Sphere cameraSphere(cameraPosition, visibilityRanges[pNode->GetLODLevel()]);
    if (!pNode->GetBoundingBox().SphereIntersection(cameraSphere))
        return false;

    // has lower levels?
    uint32 drawFlags = 0;
    if (pNode->IsLeafNode())
    {
        // at lowest lod level, draw the whole thing
        //if (pNode->IsFlat())
            //callback(pNode, TerrainQuadTreeQuery::DRAW_FLAG_SINGLE_QUAD);
        //else
            callback(pNode, TerrainQuadTreeQuery::DrawFlagAll);
    }
    else
    {
        // out of range for next lod level, if the parent is they all will be
        cameraSphere.SetRadius(visibilityRanges[pNode->GetLODLevel() - 1]);
        if (pNode->GetBoundingBox().SphereIntersection(cameraSphere))
        {
            // test each of the children
            const TerrainQuadTreeNode *pChildNode;
            bool completelyInFrustum = (intersectionType == Frustum::INTERSECTION_TYPE_INSIDE);

            if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceTopLeft)) != NULL && !RecursiveEnumerateNodesToRender<CALLBACK_TYPE>(cameraPosition, cameraFrustum, visibilityRanges, callback, pChildNode, completelyInFrustum)) { drawFlags |= TerrainQuadTreeQuery::DrawFlagTopLeft; }
            if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceTopRight)) != NULL && !RecursiveEnumerateNodesToRender<CALLBACK_TYPE>(cameraPosition, cameraFrustum, visibilityRanges, callback, pChildNode, completelyInFrustum)) { drawFlags |= TerrainQuadTreeQuery::DrawFlagTopRight; }
            if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceBottomLeft)) != NULL && !RecursiveEnumerateNodesToRender<CALLBACK_TYPE>(cameraPosition, cameraFrustum, visibilityRanges, callback, pChildNode, completelyInFrustum)) { drawFlags |= TerrainQuadTreeQuery::DrawFlagBottomLeft; }
            if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceBottomRight)) != NULL && !RecursiveEnumerateNodesToRender<CALLBACK_TYPE>(cameraPosition, cameraFrustum, visibilityRanges, callback, pChildNode, completelyInFrustum)) { drawFlags |= TerrainQuadTreeQuery::DrawFlagBottomRight; }

            if (drawFlags != 0)
                callback(pNode, drawFlags);
        }
        else
        {
            // draw everything at this level
            callback(pNode, TerrainQuadTreeQuery::DrawFlagAll);
        }
    }

    // consume node
    return true;
}
