#include "Engine/PrecompiledHeader.h"
#include "Engine/TerrainQuadTree.h"
#include "Engine/TerrainSection.h"
#include "Engine/EngineCVars.h"
#include "Engine/DataFormats.h"
Log_SetChannel(TerrainSectionQuadTree);

// todo: optimize by not creating leaf nodes when the height is static
// todo: background optimization of quadtree

TerrainSectionQuadTree::TerrainSectionQuadTree(const TerrainSection *pSection)
    : m_pSection(pSection),
      m_pNodes(NULL),
      m_nNodes(0)
{

}

TerrainSectionQuadTree::~TerrainSectionQuadTree()
{
    delete[] m_pNodes;
}

bool TerrainSectionQuadTree::Build(const TerrainParameters *pParameters, const TerrainSection *pSection, ByteStream *pOutputStream, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    const uint32 sectionSize = pSection->GetPointCount() - 1;
    DebugAssert(pParameters->LODCount < TERRAIN_MAX_RENDER_LODS && (sectionSize >> (pParameters->LODCount - 1)) > 0);

    // determine the number of nodes
    uint32 levelNodeSizes[TERRAIN_MAX_RENDER_LODS];
    uint32 allocatedNodeCount = 0;
    {
        uint32 currentNodeSize = sectionSize;
        for (uint32 i = 0; i < pParameters->LODCount; i++)
        {
            DebugAssert(currentNodeSize > 0);
            levelNodeSizes[i] = currentNodeSize;
            allocatedNodeCount += Math::Square(sectionSize / currentNodeSize);
            currentNodeSize /= 2;
        }
    }

    // allocate nodes
    TerrainQuadTreeNode *pNodes = new TerrainQuadTreeNode[allocatedNodeCount];
    uint32 currentNodeIndex = 0;

    // set up progress
    pProgressCallbacks->SetCancellable(false);
    pProgressCallbacks->SetStatusText("Creating quadtree nodes...");
    pProgressCallbacks->SetProgressRange(allocatedNodeCount);
    pProgressCallbacks->SetProgressValue(0);

    // build top level nodes, this will recurse down and fill the other levels
    BuildTopLevel(pNodes, currentNodeIndex, allocatedNodeCount, pParameters, pSection, pProgressCallbacks);

    // set to max since some nodes may have been skipped
    pProgressCallbacks->SetProgressValue(allocatedNodeCount);

    // write the quadtree out
    bool writeResult = SaveNodeBuffer(pOutputStream, pNodes, currentNodeIndex, pParameters->LODCount);

    // clean up
    delete[] pNodes;
    return writeResult;
}

void TerrainSectionQuadTree::BuildTopLevel(TerrainQuadTreeNode *pNodeBuffer, uint32 &currentNodeIndex, uint32 allocatedNodeCount, const TerrainParameters *pParameters, const TerrainSection *pSection, ProgressCallbacks *pProgressCallbacks)
{
    const uint32 pointCount = pSection->GetPointCount();
    const uint32 quadCount = pParameters->SectionSize;
    const float3 &sectionMinBounds = pSection->GetBoundingBox().GetMinBounds();
    const float3 &sectionMaxBounds = pSection->GetBoundingBox().GetMaxBounds();

    // should have lod0
    DebugAssert(pSection->GetStorageLODLevel() == 0);

    // allocate a node
    DebugAssert(currentNodeIndex < allocatedNodeCount);
    TerrainQuadTreeNode *pNode = pNodeBuffer + (currentNodeIndex++);

    // fill the base details
    pNode->m_LODLevel = (pParameters->LODCount - 1);
    pNode->m_startQuadX = 0;
    pNode->m_startQuadY = 0;
    pNode->m_nodeSize = quadCount;

    // extract min/max heights
    float minHeight = Y_FLT_INFINITE;
    float maxHeight = -Y_FLT_INFINITE;
    for (uint32 sy = 0; sy < pointCount; sy++)
    {
        for (uint32 sx = 0; sx < pointCount; sx++)
        {
            float height = pSection->GetHeightMapValue(sx, sy);
            if (height != Y_FLT_INFINITE)
            {
                minHeight = Min(minHeight, height);
                maxHeight = Max(maxHeight, height);
            }
        }
    }

    // if no values were found, set both to be infinite, so this quad never gets rendered
    if (minHeight == Y_FLT_INFINITE && maxHeight == -Y_FLT_INFINITE)
        minHeight = maxHeight = Y_FLT_INFINITE;

    // bounds can be copied directly from the section
    float3 minBounds(sectionMinBounds.x, sectionMinBounds.y, minHeight);
    float3 maxBounds(sectionMaxBounds.x, sectionMaxBounds.y, maxHeight);
    pNode->m_boundingBox.SetBounds(minBounds, maxBounds);
    pNode->m_boundingSphere = Sphere::FromAABox(pNode->m_boundingBox);
    
    // initialize child nodes to null for now
    Y_memzero(pNode->m_pChildNodes, sizeof(pNode->m_pChildNodes));
    pNode->m_isLeafNode = false;
    pNode->m_isFlat = false;

    // build children, however if we don't have any heights, don't bother
    if (pNode->m_LODLevel > 0 && minHeight != Y_FLT_INFINITE && maxHeight != Y_FLT_INFINITE)
        BuildChildrenRecursive(pNodeBuffer, currentNodeIndex, allocatedNodeCount, pNode, pParameters, pSection, pProgressCallbacks);
    else
        pNode->m_isLeafNode = true;
}

void TerrainSectionQuadTree::BuildChildrenRecursive(TerrainQuadTreeNode *pNodeBuffer, uint32 &currentNodeIndex, uint32 allocatedNodeCount, TerrainQuadTreeNode *pSplittingNode, const TerrainParameters *pParameters, const TerrainSection *pSection, ProgressCallbacks *pProgressCallbacks)
{
    const uint32 scale = pParameters->Scale;
    const float3 &sectionMinBounds = pSection->GetBoundingBox().GetMinBounds();

    // store the starting quads and size
    uint32 startQuadX = pSplittingNode->m_startQuadX;
    uint32 startQuadY = pSplittingNode->m_startQuadY;
    uint32 quadCount = pSplittingNode->m_nodeSize;

    // determine child node size
    uint32 childQuadCount = quadCount / 2;

    // divide the splitting node into 4 children
    // array order: TL, TR, BL, BR
    float quadrantMinHeights[4];
    float quadrantMaxHeights[4];
    uint32 quadrantStartPointsX[4];
    uint32 quadrantStartPointsY[4];
    bool quadrantVariableHeights[4];

    // fill starting points
    quadrantStartPointsX[0] = startQuadX;                   // TL
    quadrantStartPointsY[0] = startQuadY;                   // TL
    quadrantStartPointsX[1] = startQuadX + childQuadCount;  // TR
    quadrantStartPointsY[1] = startQuadY;                   // TR
    quadrantStartPointsX[2] = startQuadX;                   // BL
    quadrantStartPointsY[2] = startQuadY + childQuadCount;  // BL
    quadrantStartPointsX[3] = startQuadX + childQuadCount;  // BR
    quadrantStartPointsY[3] = startQuadY + childQuadCount;  // BR

    // find heights from quadrants
    for (uint32 i = 0; i < 4; i++)
    {
        // find starting and ending points
        uint32 startPointX = quadrantStartPointsX[i];
        uint32 startPointY = quadrantStartPointsY[i];
        uint32 endPointX = startPointX + childQuadCount;
        uint32 endPointY = startPointY + childQuadCount;
        
        // retrieve the heights
        float minHeight = -Y_FLT_INFINITE;
        float maxHeight = Y_FLT_INFINITE;
        bool firstHeight = true;
        bool variableHeight = false;
        for (uint32 sy = startPointY; sy <= endPointY; sy++)
        {
            for (uint32 sx = startPointX; sx <= endPointX; sx++)
            {
                float height = pSection->GetHeightMapValue(sx, sy);
                if (height != Y_FLT_INFINITE)
                {
                    if (!firstHeight)
                    {
                        if (height != minHeight && height != maxHeight)
                            variableHeight = true;

                        minHeight = Min(minHeight, height);
                        maxHeight = Max(maxHeight, height);
                    }
                    else
                    {
                        minHeight = height;
                        maxHeight = height;
                        firstHeight = false;
                    }


                }
            }
        }

        // store heights
        quadrantMinHeights[i] = minHeight;
        quadrantMaxHeights[i] = maxHeight;
        quadrantVariableHeights[i] = variableHeight;
    }

    // are all quadrant heights the same? if so, there is no point in constructing leaf nodes
    if (!quadrantVariableHeights[0] && !quadrantVariableHeights[1] && !quadrantVariableHeights[2] && !quadrantVariableHeights[3])
    {
        // flag as leaf node, and exit out
        pSplittingNode->m_isLeafNode = true;
        pSplittingNode->m_isFlat = true;
        return;
    }

    // create children
    for (uint32 i = 0; i < 4; i++)
    {
        // if the child has no heights (hole), don't allocate it
        if (quadrantMinHeights[i] == Y_FLT_INFINITE && quadrantMaxHeights[i] == Y_FLT_INFINITE)
            continue;

        // allocate a node
        DebugAssert(currentNodeIndex < allocatedNodeCount);
        TerrainQuadTreeNode *pNode = pNodeBuffer + (currentNodeIndex++);

        // fill basic details
        pNode->m_LODLevel = pSplittingNode->m_LODLevel - 1;
        pNode->m_startQuadX = quadrantStartPointsX[i];
        pNode->m_startQuadY = quadrantStartPointsY[i];
        pNode->m_nodeSize = childQuadCount;

        // calculate bounding box
        float3 minBounds(sectionMinBounds.x + (float)(quadrantStartPointsX[i] * scale), sectionMinBounds.y + (float)(quadrantStartPointsY[i] * scale), quadrantMinHeights[i]);
        float3 maxBounds(minBounds.x + (float)(childQuadCount * scale), minBounds.y + (float)(childQuadCount * scale), quadrantMaxHeights[i]);

        // store it
        pNode->m_boundingBox.SetBounds(minBounds, maxBounds);
        pNode->m_boundingSphere = Sphere::FromAABox(pNode->m_boundingBox);

        // initialize child nodes to null for now
        Y_memzero(pNode->m_pChildNodes, sizeof(pNode->m_pChildNodes));
        pNode->m_isLeafNode = false;
        pNode->m_isFlat = false;

        // if this node is not level 0, subdivide it
        if (pNode->m_LODLevel > 0)
            BuildChildrenRecursive(pNodeBuffer, currentNodeIndex, allocatedNodeCount, pNode, pParameters, pSection, pProgressCallbacks);
        else
            pNode->m_isLeafNode = true;

        // store as child node
        pSplittingNode->m_pChildNodes[i] = pNode;
    }
}

bool TerrainSectionQuadTree::SaveNodeBuffer(ByteStream *pStream, TerrainQuadTreeNode *pNodeBuffer, uint32 nodeCount, uint32 LODCount)
{
    // write nodes
    DF_TERRAIN_QUADTREE_HEADER quadTreeHeader;
    quadTreeHeader.Magic = DF_TERRAIN_QUADTREE_HEADER_MAGIC;
    quadTreeHeader.HeaderSize = sizeof(quadTreeHeader);
    quadTreeHeader.LODCount = LODCount;
    quadTreeHeader.NodeCount = nodeCount;
    if (!pStream->Write2(&quadTreeHeader, sizeof(quadTreeHeader)))
        return false;

    // write nodes
    for (uint32 i = 0; i < nodeCount; i++)
    {
        const TerrainQuadTreeNode *pNode = pNodeBuffer + i;

        // write node
        DF_TERRAIN_QUADTREE_NODE writeNode;
        writeNode.LODLevel = pNode->m_LODLevel;
        writeNode.StartQuadX = pNode->m_startQuadX;
        writeNode.StartQuadY = pNode->m_startQuadY;
        writeNode.NodeSize = pNode->m_nodeSize;
        pNode->m_boundingBox.GetMinBounds().Store(writeNode.BoundingBoxMin);
        pNode->m_boundingBox.GetMaxBounds().Store(writeNode.BoundingBoxMax);
        pNode->m_boundingSphere.GetCenter().Store(writeNode.BoundingSphereCenter);
        writeNode.BoundingSphereRadius = pNode->m_boundingSphere.GetRadius();
        writeNode.IsLeafNode = (pNode->m_isLeafNode) ? 1 : 0;
        writeNode.IsFlat = (pNode->m_isFlat) ? 1 : 0;

        // write children indices
        for (uint32 j = 0; j < 4; j++)
        {
            int32 childIndex;
            if (pNode->m_pChildNodes[j] != NULL)
                childIndex = (int32)(pNode->m_pChildNodes[j] - pNodeBuffer);
            else
                childIndex = -1;

            writeNode.ChildNodeIndices[j] = childIndex;
        }

        // save it
        if (!pStream->Write2(&writeNode, sizeof(writeNode)))
            return false;
    }

    return true;
}

bool TerrainSectionQuadTree::LoadFromStream(ByteStream *pStream)
{
    DF_TERRAIN_QUADTREE_HEADER quadTreeHeader;
    if (!pStream->Read2(&quadTreeHeader, sizeof(quadTreeHeader)) ||
        quadTreeHeader.Magic != DF_TERRAIN_QUADTREE_HEADER_MAGIC ||
        quadTreeHeader.HeaderSize != sizeof(quadTreeHeader))
    {
        return false;
    }

    m_LODCount = quadTreeHeader.LODCount;
    m_nNodes = quadTreeHeader.NodeCount;
    DebugAssert(m_nNodes > 0);

    // allocate nodes
    m_pNodes = new TerrainQuadTreeNode[m_nNodes];

    // read nodes
    for (uint32 nodeIndex = 0; nodeIndex < m_nNodes; nodeIndex++)
    {
        DF_TERRAIN_QUADTREE_NODE readNode;
        if (!pStream->Read2(&readNode, sizeof(readNode)))
            return false;

        TerrainQuadTreeNode *pNode = m_pNodes + nodeIndex;

        pNode->m_LODLevel = readNode.LODLevel;
        pNode->m_startQuadX = readNode.StartQuadX;
        pNode->m_startQuadY = readNode.StartQuadY;
        pNode->m_nodeSize = readNode.NodeSize;

        float3 boundingBoxMin(readNode.BoundingBoxMin);
        float3 boundingBoxMax(readNode.BoundingBoxMax);
        pNode->m_boundingBox.SetBounds(boundingBoxMin, boundingBoxMax);

        float3 boundingSphereCenter(readNode.BoundingSphereCenter);
        pNode->m_boundingSphere.SetCenter(boundingSphereCenter);
        pNode->m_boundingSphere.SetRadius(readNode.BoundingSphereRadius);

        pNode->m_isLeafNode = (readNode.IsLeafNode != 0);
        pNode->m_isFlat = (readNode.IsFlat != 0);

        for (uint32 j = 0; j < 4; j++)
        {
            int32 childIndex = readNode.ChildNodeIndices[j];
            if (childIndex < 0)
            {
                pNode->m_pChildNodes[j] = NULL;
                continue;
            }

            DebugAssert((uint32)childIndex < m_nNodes);
            pNode->m_pChildNodes[j] = m_pNodes + childIndex;
        }
    }

    return true;
}

bool TerrainSectionQuadTree::SaveToStream(ByteStream *pOutputStream) const
{
    // write nodes
    DF_TERRAIN_QUADTREE_HEADER quadTreeHeader;
    quadTreeHeader.Magic = DF_TERRAIN_QUADTREE_HEADER_MAGIC;
    quadTreeHeader.HeaderSize = sizeof(quadTreeHeader);
    quadTreeHeader.LODCount = m_LODCount;
    quadTreeHeader.NodeCount = m_nNodes;
    if (!pOutputStream->Write2(&quadTreeHeader, sizeof(quadTreeHeader)))
        return false;

    // write nodes
    for (uint32 i = 0; i < m_nNodes; i++)
    {
        const TerrainQuadTreeNode *pNode = m_pNodes + i;
        
        // write node
        DF_TERRAIN_QUADTREE_NODE writeNode;
        writeNode.LODLevel = pNode->m_LODLevel;
        writeNode.StartQuadX = pNode->m_startQuadX;
        writeNode.StartQuadY = pNode->m_startQuadY;
        writeNode.NodeSize = pNode->m_nodeSize;
        pNode->m_boundingBox.GetMinBounds().Store(writeNode.BoundingBoxMin);
        pNode->m_boundingBox.GetMaxBounds().Store(writeNode.BoundingBoxMax);
        pNode->m_boundingSphere.GetCenter().Store(writeNode.BoundingSphereCenter);
        writeNode.BoundingSphereRadius = pNode->m_boundingSphere.GetRadius();
        writeNode.IsLeafNode = (pNode->m_isLeafNode) ? 1 : 0;
        writeNode.IsFlat = (pNode->m_isFlat) ? 1 : 0;

        // write children indices
        for (uint32 j = 0; j < 4; j++)
        {
            int32 childIndex;
            if (pNode->m_pChildNodes[j] != NULL)
                childIndex = (int32)(pNode->m_pChildNodes[j] - m_pNodes);
            else
                childIndex = -1;

            writeNode.ChildNodeIndices[j] = childIndex;
        }
        
        // save it
        if (!pOutputStream->Write2(&writeNode, sizeof(writeNode)))
            return false;
    }

    return true;
}

void TerrainSectionQuadTree::UpdateMinMaxHeight(uint32 pointX, uint32 pointY, float newHeight, float oldHeight, bool *pNeedsRebuild)
{
    bool needsRebuild = false;

    // ignore infinite (hole) heights
    if (newHeight != Y_FLT_INFINITE)
    {
        // update top level node
        UpdateMinMaxHeightRecursive(m_pNodes, pointX, pointY, newHeight, oldHeight, needsRebuild);
    }

    if (pNeedsRebuild != NULL)
        *pNeedsRebuild = needsRebuild;
}

void TerrainSectionQuadTree::UpdateMinMaxHeightRecursive(TerrainQuadTreeNode *pNode, uint32 pointX, uint32 pointY, float newHeight, float oldHeight, bool &needsRebuild)
{
    // get current bounds
    float3 minBounds(pNode->m_boundingBox.GetMinBounds());
    float3 maxBounds(pNode->m_boundingBox.GetMaxBounds());

    // if the old height was the minimum or maximum height, and it's
    // changed in the opposite direction, trigger a rebuild
    // todo: reenable when rebuilding is backgrounded
    /*if (newHeight != Y_FLT_INFINITE)
    {
        if ((minBounds.z == oldHeight && newHeight > oldHeight) ||
            (maxBounds.z == oldHeight && newHeight < oldHeight))
        {
            needsRebuild = true;
        }
    }
    else if (oldHeight != Y_FLT_INFINITE)
    {
        // adding a hole
        if (minBounds.z == oldHeight || maxBounds.z == oldHeight)
            needsRebuild = true;

        return;
    }*/

    // update this node
    bool changed = false;
    if (newHeight < minBounds.z)
    {
        minBounds.z = newHeight;
        changed = true;
    }

    if (newHeight > maxBounds.z)
    {
        maxBounds.z = newHeight;
        changed = true;
    }

    if (changed)
    {
        // signal a rebuild is needed if this was previously a single height leaf node at a lod > 0
        if (pNode->GetLODLevel() > 0 && pNode->IsLeafNode())
            needsRebuild = true;

        pNode->m_boundingBox.SetBounds(minBounds, maxBounds);
        pNode->m_boundingSphere = Sphere::FromAABox(pNode->m_boundingBox);
    }

    // not a leaf node?
    if (!pNode->IsLeafNode())
    {
        // update children
        for (uint32 i = 0; i < 4; i++)
        {
            TerrainQuadTreeNode *pChildNode = pNode->m_pChildNodes[i];
            if (pChildNode != NULL)
            {
                if (pointX >= pChildNode->m_startQuadX && pointX <= (pChildNode->m_startQuadX + pChildNode->m_nodeSize) &&
                    pointY >= pChildNode->m_startQuadY && pointY <= (pChildNode->m_startQuadY + pChildNode->m_nodeSize))
                {
                    UpdateMinMaxHeightRecursive(pChildNode, pointX, pointY, newHeight, oldHeight, needsRebuild);
                }
            }
        }
    }
}

TerrainQuadTreeQuery::TerrainQuadTreeQuery()
{

}

TerrainQuadTreeQuery::~TerrainQuadTreeQuery()
{

}

void TerrainQuadTreeQuery::Invoke(const TerrainSection *const *ppSections, uint32 nSections, const float3 &cameraPosition, const Frustum &cameraFrustum, const float visibilityRanges[TERRAIN_MAX_RENDER_LODS])
{
    // clear previous results
    m_selectedNodes.Clear();

    // go over sections
    for (uint32 i = 0; i < nSections; i++)
    {
        const TerrainSection *pSection = ppSections[i];
        
        // get toplevel node of quadtree
        const TerrainSectionQuadTree *pQuadTree = pSection->GetQuadTree();
        const TerrainQuadTreeNode *pTopLevelNode = pQuadTree->GetTopLevelNode();

        // get intersection type
        Frustum::IntersectionType intersectionType = cameraFrustum.AABoxIntersectionType(pTopLevelNode->GetBoundingBox());
        if (intersectionType != Frustum::INTERSECTION_TYPE_OUTSIDE)
        {
            bool completelyInFrustum = (intersectionType == Frustum::INTERSECTION_TYPE_INTERSECTS);
            ProcessNode(cameraPosition, cameraFrustum, visibilityRanges, pSection, pTopLevelNode, completelyInFrustum);
        }
    }
}

bool TerrainQuadTreeQuery::ProcessNode(const float3 &cameraPosition, const Frustum &cameraFrustum, const float visibilityRanges[TERRAIN_MAX_RENDER_LODS], const TerrainSection *pSection, const TerrainQuadTreeNode *pNode, bool parentCompletelyInFrustum)
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
            //drawFlags = DRAW_FLAG_SINGLE_QUAD;
        //else
            drawFlags = DrawFlagAll;
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

            if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceTopLeft)) != NULL && !ProcessNode(cameraPosition, cameraFrustum, visibilityRanges, pSection, pChildNode, completelyInFrustum)) { drawFlags |= DrawFlagTopLeft; }
            if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceTopRight)) != NULL && !ProcessNode(cameraPosition, cameraFrustum, visibilityRanges, pSection, pChildNode, completelyInFrustum)) { drawFlags |= DrawFlagTopRight; }
            if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceBottomLeft)) != NULL && !ProcessNode(cameraPosition, cameraFrustum, visibilityRanges, pSection, pChildNode, completelyInFrustum)) { drawFlags |= DrawFlagBottomLeft; }
            if ((pChildNode = pNode->GetChildNode(TerrainQuadTreeNode::ChildReferenceBottomRight)) != NULL && !ProcessNode(cameraPosition, cameraFrustum, visibilityRanges, pSection, pChildNode, completelyInFrustum)) { drawFlags |= DrawFlagBottomRight; }
        }
        else
        {
            // draw everything at this level
            drawFlags = DrawFlagAll;
        }
    }

    // drawing this node at all?
    if (drawFlags != 0)
    {
        // add to list
        SelectedNode selectedNode;
        selectedNode.pSection = pSection;
        selectedNode.pNode = pNode;
        selectedNode.DrawFlags = drawFlags;
        m_selectedNodes.Add(selectedNode);
    }

    // consume node
    return true;
}
