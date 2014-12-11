/*
    MessyBsp. BSP collision and loading example code.
    Copyright (C) 2014 Richard Maxwell <jodi.the.tigger@gmail.com>
    This file is part of MessyBsp
    MessyBsp is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.
    You should have received a copy of the GNU Affero General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>
*/

#include "BspBrushToMesh.hpp"
#include "Bsp.hpp"
#include "VectorMaths3.hpp"

#include <cmath> // abs

namespace Bsp {

bool inline PointInPlane(
        const std::vector<Plane>& planes,
        const Vec3& point,
        float epislon = 0.0f)
{
    for (const auto& plane : planes)
    {
        auto distance =
                DotF(plane.normal, point) +
                plane.distance -
                epislon;

        if (distance > 0.0f)
        {
            return false;
        }
    }

    return true;
}

std::vector<Vec3> VerticiesFromIntersectingPlanes(
        const std::vector<Plane> planes)
{
    const auto planeCount = planes.size();
    std::vector<Vec3> result;

    // Ugh, brute force.
    for (unsigned i = 0; i < planeCount; ++i)
    {
        const auto& n1 = planes[i];

        for (unsigned j = i; j < planeCount; ++j)
        {
            const auto& n2 = planes[j];

            for (unsigned k = j; k < planeCount; ++k)
            {
                const auto& n3 = planes[k];

                auto n2n3 = Cross(n2.normal, n3.normal);
                auto n3n1 = Cross(n3.normal, n1.normal);
                auto n1n2 = Cross(n1.normal, n2.normal);

                // Don't bother if the lengths are too small.
                if  (
                        ( SquareF(n2n3) < 0.0001f) ||
                        ( SquareF(n3n1) < 0.0001f) ||
                        ( SquareF(n1n2) < 0.0001f)
                    )
                {
                    continue;
                }

                // From bullet physics:

                // point P out of 3 plane equations:
                // (. == Dot(), * = Cross())

                //	     d1(N2 * N3) + d2(N3 * N1) + d3(N1 * N2)
                //  P =  ---------------------------------------
                //       N1 . (N2 * N3)

                auto quotient = DotF(n2n3, n1.normal);

                if (std::abs(quotient) <= 0.000001f)
                {
                    continue;
                }

                // Bullet makes the quotent -ve, dunno why (yet).
                quotient = 1.0f / quotient;

                auto d1n2n3 = n2n3 * n1.normal;
                auto d2n3n1 = n3n1 * n2.normal;
                auto d3n1n2 = n1n2 * n3.normal;

                auto point = d2n3n1 + d3n1n2;
                point = point + d1n2n3;
                point = point * quotient;

                if (!PointInPlane(planes, point, 0.01f))
                {
                    continue;
                }

                result.push_back(point);
            }
        }
    }

    return result;
}

std::vector<Mesh> GetBrushMeshes(CollisionBsp &bsp)
{
    std::vector<Mesh> result;

    // For each brush that's solid, get all the plane equations
    // and then get all the intersection points between all the planes.
    // Once you have those, get the convex hull for those points, then
    // turn it into a triangle mesh. Return.

    for (const auto& leaf : bsp.leaves)
    {
        const auto* leafBrushes = &bsp.leafBrushes[leaf.firstLeafBrushIndex];

        for (auto i = 0; i < leaf.leafBrushCount; ++i)
        {
            auto index = leafBrushes[i].brushIndex;

            const auto& brush = bsp.brushes[index].brush;

            // RAM: TODO: setting textureindex to -1 to prevent doing
            // brush multiple times, figure out how to do that here.
            if (brush.textureIndex < 0)
            {
                continue;
            }

            if (!(bsp.textures[brush.textureIndex].contentFlags & 1))
            {
                continue;
            }

            // RAM: TODO: Set brush as done.

            std::vector<Plane> planes;
            const auto* brushSides = &bsp.brushSides[brush.firstBrushSideIndex];

            for (auto j = 0; j < brush.sideCount; ++j)
            {
                const auto& brushSide = brushSides[j];

                auto plane = bsp.planes[brushSide.planeIndex];
                plane.distance = -plane.distance;

                planes.push_back(plane);
            }

            if (!planes.empty())
            {
                // Get verticies from plane equations.
                // Get hull from verticies
                // Get triangle mesh from hull.
            }
        }
    }

    return result;
}

} // namespace