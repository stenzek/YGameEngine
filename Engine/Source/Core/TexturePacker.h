#pragma once
#include "Core/Common.h"
#include "Core/Image.h"
#include "YBaseLib/MemArray.h"
#include "YBaseLib/PODArray.h"

class TexturePacker
{
public:
    TexturePacker(PIXEL_FORMAT pixelFormat);
    ~TexturePacker();

    // add an image
    bool AddImage(const Image *pImage, uint32 paddingAmount, void *pReferenceData);

    // guess image dimensions that may be sufficient for packing
    void GuessPackedImageDimensions(uint32 *pWidth, uint32 *pHeight);

    // pack to texture
    bool Pack(uint32 textureWidth, uint32 textureHeight, float paddingR = 0.0f, float paddingG = 0.0f, float paddingB = 0.0f, float paddingA = 0.0f);

    // get result image
    const Image *GetPackedImage() const { return &m_packedImage; }

    // get result pack location
    bool GetImageLocation(void *pReferenceData, uint32 *pLeft, uint32 *pRight, uint32 *pTop, uint32 *pBottom);

private:
    struct ImageToPack
    {
        uint32 PaddingAmount;
        uint32 TotalArea;
        Image *pImage;
        void *pReferenceData;
    };

    struct Node
    {
        int32 ChildIndices[2];
        bool IsLeaf;
        bool IsUsed;
        uint32 Left, Top;
        uint32 Width, Height;
        const ImageToPack *pImage;
        void *pReferenceData;

        Node(uint32 left, uint32 top, uint32 width, uint32 height)
        {
            ChildIndices[0] = -1;
            ChildIndices[1] = -1;
            IsLeaf = true;
            IsUsed = false;
            Left = left;
            Top = top;
            Width = width;
            Height = height;
            pImage = nullptr;
            pReferenceData = nullptr;
        }        
    };

    typedef MemArray<Node> NodeArray;
    typedef MemArray<ImageToPack> ImageArray;

    // we have to use indices here as it's possible the array will get reallocated and addresses change
    int32 TryInsertIntoNode(int32 nodeIndex, uint32 neededWidth, uint32 neededHeight);

    PIXEL_FORMAT m_pixelFormat;
    NodeArray m_nodes;
    ImageArray m_images;
    Image m_packedImage;
};
