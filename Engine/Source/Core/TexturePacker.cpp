#include "Core/PrecompiledHeader.h"
#include "Core/TexturePacker.h"
#include "YBaseLib/Log.h"
Log_SetChannel(TexturePacker);

TexturePacker::TexturePacker(PIXEL_FORMAT pixelFormat)
    : m_pixelFormat(pixelFormat)
{

}

TexturePacker::~TexturePacker()
{
    for (uint32 i = 0; i < m_images.GetSize(); i++)
        delete m_images[i].pImage;
}

bool TexturePacker::AddImage(const Image *pImage, uint32 paddingAmount, void *pReferenceData)
{
    if (pImage->GetPixelFormat() != m_pixelFormat)
        return false;

    ImageToPack itp;
    itp.pImage = new Image(*pImage);
    itp.PaddingAmount = paddingAmount;
    itp.TotalArea = (pImage->GetWidth() + (paddingAmount * 2)) + (pImage->GetHeight() + (paddingAmount * 2));
    itp.pReferenceData = pReferenceData;
    m_images.Add(itp);
    return true;
}

void TexturePacker::GuessPackedImageDimensions(uint32 *pWidth, uint32 *pHeight)
{
    // get the total area of all images
    uint32 areaSum = 0;
    for (uint32 i = 0; i < m_images.GetSize(); i++)
        areaSum += m_images[i].TotalArea;

    // shouldn't be zero..
    areaSum = Max(areaSum, (uint32)1);

    // assume a square texture, sqrt() the area, and select the next power of 2
    uint32 textureDimensions = Y_nextpow2((uint32)Math::Truncate(Math::Round(Math::Sqrt((float)areaSum))));
    *pWidth = textureDimensions;
    *pHeight = textureDimensions;
}

bool TexturePacker::Pack(uint32 textureWidth, uint32 textureHeight, float paddingR /* = 0.0f */, float paddingG /* = 0.0f */, float paddingB /* = 0.0f */, float paddingA /* = 0.0f */)
{
    Log_DevPrintf("TexturePacker::Pack: Packing %u images to %ux%u texture", m_images.GetSize(), textureWidth, textureHeight);

    // sort images, largest -> smallest
    m_images.Sort([](const ImageToPack *pLeft, const ImageToPack *pRight) {
        return static_cast<int32>(pLeft->TotalArea) - static_cast<int32>(pRight->TotalArea);
    });

    // create one node encompassing the entire texture
    m_nodes.Add(Node(0, 0, textureWidth, textureHeight));

    // try to pack each image
    for (uint32 i = 0; i < m_images.GetSize(); i++)
    {
        const ImageToPack &img = m_images[i];
        int32 nodeIndex = TryInsertIntoNode(0, img.pImage->GetWidth() + (img.PaddingAmount * 2), img.pImage->GetHeight() + (img.PaddingAmount * 2));
        if (nodeIndex == -1)
        {
            // pack failed
            Log_ErrorPrintf("failed to pack image %u %ux%u", i, img.pImage->GetWidth(), img.pImage->GetHeight());
            m_nodes.Clear();
            return false;
        }

        //Log_DevPrintf("img[%u, %ux%u] -> %i (%u %u)", i, img.pImage->GetWidth(), img.pImage->GetHeight(), nodeIndex, m_nodes[nodeIndex].Left, m_nodes[nodeIndex].Top);

        DebugAssert(m_nodes[nodeIndex].pImage == nullptr);
        m_nodes[nodeIndex].pImage = &img;
        m_nodes[nodeIndex].pReferenceData = img.pReferenceData;
    }

    // create the output image, and zero all pixels
    m_packedImage.Create(m_pixelFormat, textureWidth, textureHeight, 1);

    // allocate a 1x1 image of the padding colour
    {
        uint32 paddingImageSize = PixelFormat_CalculateImageSize(m_pixelFormat, 1, 1, 1);
        byte *pPaddingPixel = new byte[paddingImageSize];
        float sourcePixel[4] = { paddingR, paddingG, paddingB, paddingA };
        if (!PixelFormat_ConvertPixels(1, 1, sourcePixel, sizeof(sourcePixel), PIXEL_FORMAT_R32G32B32A32_FLOAT, pPaddingPixel, paddingImageSize, m_pixelFormat, &paddingImageSize))
        {
            Log_ErrorPrintf("failed to allocate padding pixel");
            delete[] pPaddingPixel;
            m_nodes.Clear();
            return false;
        }

        // replace every pixel in the image with the padding pixel [slowww]
        for (uint32 y = 0; y < textureHeight; y++)
        {
            byte *pDestRow = m_packedImage.GetData() + (m_packedImage.GetDataRowPitch() * y);
            for (uint32 x = 0; x < textureWidth; x++)
            {
                Y_memcpy(pDestRow, pPaddingPixel, paddingImageSize);
                pDestRow += paddingImageSize;
            }

            DebugAssert(pDestRow <= m_packedImage.GetData() + (m_packedImage.GetDataRowPitch() * (y + 1)));
        }

        delete[] pPaddingPixel;
    }

    // pack each node into the texture
    for (uint32 i = 0; i < m_nodes.GetSize(); i++)
    {
        const Node &node = m_nodes[i];

        // skip branch nodes, or leaf nodes with no data
        if (!node.IsLeaf || node.pImage == nullptr)
            continue;

        // get the image
        const ImageToPack *img = node.pImage;

        // blit it into place
        if (!m_packedImage.Blit(node.Left + img->PaddingAmount, node.Top + img->PaddingAmount, *img->pImage, 0, 0, img->pImage->GetWidth(), img->pImage->GetHeight()))
        {
            Log_ErrorPrintf("failed to blit image");
            m_nodes.Clear();
            return false;
        }
    }

    // all done
    return true;
}

int32 TexturePacker::TryInsertIntoNode(int32 nodeIndex, uint32 neededWidth, uint32 neededHeight)
{
    int32 insertedID;

    if (!m_nodes[nodeIndex].IsLeaf)
    {
        // try first child
        insertedID = TryInsertIntoNode(m_nodes[nodeIndex].ChildIndices[0], neededWidth, neededHeight);
        if (insertedID != -1)
            return insertedID;

        // try second child
        insertedID = TryInsertIntoNode(m_nodes[nodeIndex].ChildIndices[1], neededWidth, neededHeight);
        if (insertedID != -1)
            return insertedID;

        // nope
        return -1;
    }
    else
    {
        // leaf node
        // skip nodes that are used
        if (m_nodes[nodeIndex].IsUsed)
            return -1;

        // do we fit into this leaf?
        uint32 nodeLeft = m_nodes[nodeIndex].Left;
        uint32 nodeTop = m_nodes[nodeIndex].Top;
        uint32 nodeWidth = m_nodes[nodeIndex].Width;
        uint32 nodeHeight = m_nodes[nodeIndex].Height;
        if (nodeWidth < neededWidth || nodeHeight < neededHeight)
            return -1;

        // perfect fit?
        if (nodeWidth == neededWidth && nodeHeight == neededHeight)
        {
            m_nodes[nodeIndex].IsUsed = true;
            return nodeIndex;
        }

        // determine split direction and split the node
        uint32 dw = nodeWidth - neededWidth;
        uint32 dh = nodeHeight - neededHeight;
        if (dw > dh)
        {
            // split horizontally
            Node child0(nodeLeft, nodeTop, neededWidth, nodeHeight);
            Node child1(nodeLeft + neededWidth, nodeTop, dw - 1, nodeHeight);

            m_nodes[nodeIndex].ChildIndices[0] = (int32)m_nodes.GetSize();
            m_nodes.Add(child0);

            m_nodes[nodeIndex].ChildIndices[1] = (int32)m_nodes.GetSize();
            m_nodes.Add(child1);

            m_nodes[nodeIndex].IsLeaf = false;
        }
        else
        {
            // split vertically
            Node child0(nodeLeft, nodeTop, nodeWidth, neededHeight);
            Node child1(nodeLeft, nodeTop + neededHeight, nodeWidth, dh - 1);

            m_nodes[nodeIndex].ChildIndices[0] = (int32)m_nodes.GetSize();
            m_nodes.Add(child0);

            m_nodes[nodeIndex].ChildIndices[1] = (int32)m_nodes.GetSize();
            m_nodes.Add(child1);

            m_nodes[nodeIndex].IsLeaf = false;
        }

        // reinsert into this node, this should go into the first child
        return TryInsertIntoNode(m_nodes[nodeIndex].ChildIndices[0], neededWidth, neededHeight);
    }
}

bool TexturePacker::GetImageLocation(void *pReferenceData, uint32 *pLeft, uint32 *pRight, uint32 *pTop, uint32 *pBottom)
{
    for (uint32 i = 0; i < m_nodes.GetSize(); i++)
    {
        const Node &node = m_nodes[i];
        if (node.pReferenceData == pReferenceData)
        {
            DebugAssert(node.pImage != nullptr);
            uint32 paddingAmount = node.pImage->PaddingAmount;
            DebugAssert(node.Width > (paddingAmount * 2));

            *pLeft = node.Left + paddingAmount;
            *pRight = node.Left + paddingAmount + node.Width - 1 - paddingAmount;
            *pTop = node.Top + paddingAmount;
            *pBottom = node.Top + paddingAmount + node.Height - 1 - paddingAmount;
            return true;
        }
    }

    return false;
}

