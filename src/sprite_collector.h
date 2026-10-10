#include "mylib.h"
#include "basetype.h"
#include "sprite_list.h"
#include "list.h"
#include <stddef.h>
class SPRITE;
class VID;
class TEXTURE;
class ANGLE;
struct VECTOR;
struct VECTOR2;
struct IDirect3DSurface9;

class SPRITE_COLLECTOR
{
public:
    struct CELL
    {

        float originX;
        float originY;
        SPRITE_LIST sprites;
        SPRITE_LIST buildedSprites;
        SPRITE_LIST alphaBuildedSprites;
        SPRITE_LIST layerSprites[16];
        TEXTURE* renderTargetGround;
        TEXTURE* sourceGroundTexture;

        CELL();
        ~CELL() noexcept(false);
        void CreateRenderTargetGround();
    };

    int renderTargetGroundCount;
    CELL* renderTargetGroundCells[130];
    CELL* spatialCells;
    SPRITE_LIST depthSortedSprites;
    SPRITE_LIST allSprites;
    SPRITE_LIST armySprites[8];
    LIST<SPRITE*> namedSprites;
    int gridWidth;
    int gridHeight;
    int gridShift;
    float gridScaleX;
    float gridScaleY;
    int scanMinX;
    int scanCurrentY;
    int scanMaxX;
    int scanMaxY;
    int scanCurrentX;
    int scanItem;
    int reserved2EC;
    int deletingAll;

    explicit SPRITE_COLLECTOR(const VECTOR2& dimensions);
    ~SPRITE_COLLECTOR() noexcept(false);
    void DeleteAll();
    void OptimizeForDraw();
    void Insert(SPRITE* sprite);
    void InsertToHash(SPRITE* sprite);
    int DeleteFromActive(SPRITE* sprite);
    int DeleteFromHash(SPRITE* sprite);
    int Delete(SPRITE* sprite);
    SPRITE* FirstAllSprite() const;
    SPRITE* NextAllSprite() const;
    SPRITE* FirstAllSpriteFrom0() const;
    SPRITE* NextAllSpriteFrom0() const;
    SPRITE* FirstActiveSprite(int* index) const;
    SPRITE* NextActiveSprite(int* index) const;
    void ChangeCoor(SPRITE* sprite, const VECTOR& position);
    void InvalidateDeviceObjects();
    void RestoreDeviceObjects();
    void DrawLayer(int layer, const VECTOR2& minPoint, const VECTOR2& maxPoint) const;
    void DrawLayerOnlyZ(const VECTOR2& minPoint, const VECTOR2& maxPoint) const;
    void BuildAllTerrain();
    void DrawBuilded(const VECTOR2& minPoint, const VECTOR2& maxPoint, IDirect3DSurface9* depthSurface);
    int GetFarGroundCell(const VECTOR2& minPoint, const VECTOR2& maxPoint) const;
    void DrawShadowInLayer(int layer, const VECTOR& position, const VECTOR2& halfExtents, const VECTOR2& offset) const;
    void ChangeArmy(SPRITE* sprite, int army);
    void DeleteFromNamed(SPRITE* sprite);
    void InsertToNamed(SPRITE* sprite);
    SPRITE* CanPlace(const VID* vid, const VECTOR& position, ANGLE direction);
    SPRITE* CanPlace(const VID* vid, float x, float y, float z, ANGLE direction);
    SPRITE* FirstHashInBox(float minX, float minY, float maxX, float maxY);
    SPRITE* NextHashInBox();
    SPRITE* FirstUnit(int* index, int type) const;
    SPRITE* NextUnit(int* index) const;
    SPRITE* FirstDrawingSprite(int* index, int category);
    SPRITE* NextDrawingSprite(int* index, int category);
    SPRITE* GetSprite(int type, float x, float y, SPRITE* previous);
    SPRITE* GetSpriteScr(int type, const VECTOR2& screenPoint, SPRITE* previous);
    SPRITE* FindNearestSprite(int type, const VECTOR2& point, float radius, SPRITE* previous);
    int DrawNamed() const;
    int GetNoFreeBuildedGround() const;
};


