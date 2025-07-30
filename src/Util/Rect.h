#pragma once
#include "Vectors.h"

class McRect {
   public:
    constexpr McRect(float x = 0, float y = 0, float width = 0, float height = 0, bool isCentered = false) {
        this->set(x, y, width, height, isCentered);
    }

    constexpr McRect(const Vector2 &pos, const Vector2 &size, bool isCentered = false) {
        this->set(pos, size, isCentered);
    }

    [[nodiscard]] inline bool contains(const Vector2 &point) const {
        Vector2 max = this->vMin + this->vSize;
        return glm::all(glm::greaterThanEqual(static_cast<const glm::vec2 &>(point),
                                              static_cast<const glm::vec2 &>(this->vMin))) &&
               glm::all(glm::lessThanEqual(static_cast<const glm::vec2 &>(point), static_cast<const glm::vec2 &>(max)));
    }

    [[nodiscard]] McRect intersect(const McRect &rect) const;
    [[nodiscard]] bool intersects(const McRect &rect) const;
    [[nodiscard]] McRect Union(const McRect &rect) const;

    [[nodiscard]] inline Vector2 getCenter() const { return this->vMin + this->vSize * 0.5f; }
    [[nodiscard]] inline Vector2 getMax() const { return this->vMin + this->vSize; }

    // get
    [[nodiscard]] constexpr const Vector2 &getPos() const { return this->vMin; }
    [[nodiscard]] constexpr const Vector2 &getMin() const { return this->vMin; }
    [[nodiscard]] constexpr const Vector2 &getSize() const { return this->vSize; }

    [[nodiscard]] constexpr const float &getX() const { return this->vMin.x; }
    [[nodiscard]] constexpr const float &getY() const { return this->vMin.y; }
    [[nodiscard]] constexpr const float &getMinX() const { return this->vMin.x; }
    [[nodiscard]] constexpr const float &getMinY() const { return this->vMin.y; }

    [[nodiscard]] inline float getMaxX() const { return this->vMin.x + this->vSize.x; }
    [[nodiscard]] inline float getMaxY() const { return this->vMin.y + this->vSize.y; }

    [[nodiscard]] constexpr const float &getWidth() const { return this->vSize.x; }
    [[nodiscard]] constexpr const float &getHeight() const { return this->vSize.y; }

    // set
    inline void setMin(const Vector2 &min) { this->vMin = min; }
    inline void setMax(const Vector2 &max) { this->vSize = max - this->vMin; }
    inline void setMinX(float minx) { this->vMin.x = minx; }
    inline void setMinY(float miny) { this->vMin.y = miny; }
    inline void setMaxX(float maxx) { this->vSize.x = maxx - this->vMin.x; }
    inline void setMaxY(float maxy) { this->vSize.y = maxy - this->vMin.y; }
    inline void setPos(const Vector2 &pos) { this->vMin = pos; }
    inline void setPosX(float posx) { this->vMin.x = posx; }
    inline void setPosY(float posy) { this->vMin.y = posy; }
    inline void setSize(const Vector2 &size) { this->vSize = size; }
    inline void setWidth(float width) { this->vSize.x = width; }
    inline void setHeight(float height) { this->vSize.y = height; }

   private:
    constexpr void set(float x, float y, float width, float height, bool isCentered = false) {
        this->set(Vector2(x, y), Vector2(width, height), isCentered);
    }

    constexpr void set(const Vector2 &pos, const Vector2 &size, bool isCentered = false) {
        if(isCentered) {
            Vector2 halfSize = size * 0.5f;
            this->vMin = pos - halfSize;
        } else {
            this->vMin = pos;
        }
        this->vSize = size;
    }

    Vector2 vMin;
    Vector2 vSize;
};
