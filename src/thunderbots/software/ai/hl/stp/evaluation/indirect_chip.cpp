#include "indirect_chip.h"

#include "ai/world/world.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "geom/util.h"
#include "shared/constants.h"

std::optional<Point> Evaluation::findTargetPointForIndirectChipAndChase(
    const World &world)
{
    std::optional<Robot> enemy_goalie_opt = world.enemyTeam().goalie();

    std::vector<Point> non_goalie_enemy_positions;
    std::vector<Point> all_enemy_positions;

    for (Robot i : world.enemyTeam().getAllRobots())
    {
        all_enemy_positions.push_back(i.position());
        if (std::optional<Robot>(i) != enemy_goalie_opt)
        {
            non_goalie_enemy_positions.push_back(i.position());
        }
    }

    std::vector<Triangle> allTriangles =
            getAllTrianglesBetweenEnemyPlayers(world, non_goalie_enemy_positions);

    std::vector<Triangle> target_triangles =
        findOpenTriangles(allTriangles, all_enemy_positions);

    target_triangles = remove_outofbounds_triangles(world, target_triangles);

    Point ball_position = world.ball().position();

    return findTargetPointForIndirectChipAndChase(target_triangles, ball_position);
}

std::optional<Point> Evaluation::findTargetPointForIndirectChipAndChase(
    const std::vector<Triangle> &triangles, Point ball_position)
{
    if (!triangles.empty())
    {
        // Get the largest triangle within the vector of triangles that has area greater
        // than minimum area of chip target triangle, and all edge lengths greater than
        // minimum edge length of chip target triangle
        std::optional<Triangle> largest_triangle = getLargestValidTriangle(
            triangles,
            Util::DynamicParameters::Evaluation::Indirect_Chip::min_chip_tri_area.value(),
            Util::DynamicParameters::Evaluation::Indirect_Chip::min_chip_tri_edge_len
                .value());
        Triangle t = largest_triangle.value();

        Point target = getTriangleCenter(t);
        // Adjust the target point to have a length of distance between itself and the
        // ball's position, then scaling it by a certain percentage
        target = target.norm((target - ball_position).len() *
                             Util::DynamicParameters::Evaluation::Indirect_Chip::
                                 chip_cherry_power_downscale.value());

        // Target should never be further away than maximum chip power
        if ((target - ball_position).len() >
            Util::DynamicParameters::Evaluation::Indirect_Chip::max_chip_power.value())
        {
            target =
                ball_position +
                (target - ball_position)
                    .norm(
                        Util::DynamicParameters::Evaluation::Indirect_Chip::max_chip_power
                            .value());
        }

        return std::optional(target);
    }
    else
    {
        return std::nullopt;
    }
}

std::vector<Triangle> Evaluation::getAllTrianglesBetweenEnemyPlayers(const World &world,
                                                                     std::vector<Point> enemy_players)
{
    std::vector<Triangle> triangles;
    std::vector<Point> allPts = enemy_players;

    allPts.push_back(world.field().enemyCornerNeg());
    allPts.push_back(world.field().enemyCornerPos());
    allPts.push_back(Point(0, world.field().enemyCornerPos().y()));
    allPts.push_back(Point(0, world.field().enemyCornerNeg().y()));

    for (unsigned int i = 0; i < allPts.size() - 2; i++)
    {
        for (unsigned int j = i + 1; j < allPts.size() - 1; j++)
        {
            for (unsigned int k = j + 1; k < allPts.size(); k++)
            {
                // Set up 3 different points from the vector of non-goalie enemy players'
                // positions and the four points for the rectangular region to chip and
                // chase at. With the 3 points, create a possible triangle and place in
                // vector of all triangles. Eventually all permutations of points will be
                // picked
                Point p1   = allPts[i];
                Point p2   = allPts[j];
                Point p3   = allPts[k];
                Triangle t = triangle(p1, p2, p3);
                triangles.push_back(t);
            }
        }
    }

    return triangles;
}

std::vector<Triangle> Evaluation::findOpenTriangles(std::vector<Triangle> triangles,
                                                    std::vector<Point> enemy_players)
{
    std::vector<Triangle> filtered_triangles;

    // For every triangle, the 3 points are adjusted so that the robots making up the
    // vertices won't be counted within the triangle, i.e. make every triangle slightly
    // smaller
    for (unsigned int i = 0; i < triangles.size(); i++)
    {
        Point p1 = triangles[i][0] + ((getTriangleCenter(triangles[i])) - triangles[i][0])
                                         .norm(2.5 * ROBOT_MAX_RADIUS_METERS);
        Point p2 = triangles[i][1] + ((getTriangleCenter(triangles[i])) - triangles[i][1])
                                         .norm(2.5 * ROBOT_MAX_RADIUS_METERS);
        Point p3 = triangles[i][2] + ((getTriangleCenter(triangles[i])) - triangles[i][2])
                                         .norm(2.5 * ROBOT_MAX_RADIUS_METERS);
        Triangle t         = triangle(p1, p2, p3);
        bool containsEnemy = false;

        for (unsigned int k = 0; k < enemy_players.size(); k++)
        {
            if (contains(t, enemy_players[k]))
            {
                containsEnemy = true;
                break;
            }
        }

        if (!containsEnemy)
        {
            filtered_triangles.push_back(triangles[i]);
        }
    }

    return filtered_triangles;
}

Point Evaluation::getTriangleCenter(Triangle triangle)
{
    Point p1 = triangle[0];
    Point p2 = triangle[1];
    Point p3 = triangle[2];

    double center_x = (p1.x() + p2.x() + p3.x()) / 3;
    double center_y = (p1.y() + p2.y() + p3.y()) / 3;

    Point center = Point(center_x, center_y);

    return center;
}

double Evaluation::getTriangleArea(Triangle triangle)
{
    Point p1 = triangle[0];
    Point p2 = triangle[1];
    Point p3 = triangle[2];

    double area = abs(0.5 * ((p2.x() - p1.x()) * (p3.y() - p1.y()) -
                             (p3.x() - p1.x()) * (p2.y() - p1.y())));

    return area;
}

std::vector<Triangle> Evaluation::remove_outofbounds_triangles(
    const World &world, std::vector<Triangle> triangles)
{
    std::vector<Triangle> valid_triangles;
    std::vector<Point> chip_area_corners = findBestChipTargetArea(
        world, Util::DynamicParameters::Evaluation::Indirect_Chip::chip_target_area_inset
                   .value());
    Point center;

    double smallest_x = chip_area_corners[0].x();
    double smallest_y = chip_area_corners[0].y();
    double largest_x  = chip_area_corners[0].x();
    double largest_y  = chip_area_corners[0].y();

    for (unsigned int i = 0; i < chip_area_corners.size(); i++)
    {
        Point p = chip_area_corners[i];
        if (p.x() < smallest_x)
            smallest_x = p.x();
        if (p.x() > largest_x)
            largest_x = p.x();
        if (p.y() < smallest_y)
            smallest_y = p.y();
        if (p.y() > largest_y)
            largest_y = p.y();
    }

    for (unsigned int i = 0; i < triangles.size(); i++)
    {
        Triangle t = triangles[i];
        center     = getTriangleCenter(triangles[i]);
        if (center.x() <= largest_x && center.x() >= smallest_x &&
            center.y() <= largest_y && center.y() >= smallest_y)
        {
            valid_triangles.push_back(t);
        }
    }

    return valid_triangles;
}

std::vector<Point> Evaluation::findBestChipTargetArea(const World &world, double inset)
{
    double ballX     = world.ball().position().x();
    double fieldX    = world.field().enemyGoal().x() - inset;
    double negFieldY = world.field().enemyCornerNeg().y() + inset;
    double posFieldY = world.field().enemyCornerPos().y() - inset;

    std::vector<Point> corners = {Point(ballX, negFieldY), Point(ballX, posFieldY),
                                  Point(fieldX, negFieldY), Point(fieldX, posFieldY)};

    return corners;
}

std::optional<Triangle> Evaluation::getLargestValidTriangle(
    std::vector<Triangle> allTriangles, double min_area, double min_edge_len,
    double min_edge_angle)
{
    Triangle largest    = allTriangles[0];
    double largest_area = getTriangleArea(largest);

    for (unsigned int i = 0; i < allTriangles.size(); i++)
    {
        Triangle t  = allTriangles[i];
        double area = getTriangleArea(t);
        double l1   = (t[1] - t[0]).len();
        double l2   = (t[2] - t[0]).len();
        double l3   = (t[2] - t[1]).len();

        Angle a1 = vertexAngle(t[1], t[0], t[2]).angleMod().abs();
        Angle a2 = vertexAngle(t[0], t[1], t[2]).angleMod().abs();
        Angle a3 = vertexAngle(t[0], t[2], t[1]).angleMod().abs();

        if (area >= largest_area && area >= min_area && l1 >= min_edge_len &&
            l2 >= min_edge_len && l3 >= min_edge_len &&
            a1.toDegrees() >= min_edge_angle && a2.toDegrees() >= min_edge_angle &&
            a3.toDegrees() >= min_edge_angle)
        {
            largest      = t;
            largest_area = area;
        }
    }

    return std::optional(largest);
}
