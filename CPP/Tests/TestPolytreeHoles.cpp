#include <gtest/gtest.h>
#include "clipper2/clipper.h"
#include "ClipFileLoad.h"
using namespace Clipper2Lib;
TEST(Clipper2Tests, TestPolytreeHoles1)
{
  std::ifstream ifs("PolytreeHoleOwner.txt");
  ASSERT_TRUE(ifs.good());
  Paths64 subject, subject_open, clip;
  PolyTree64 solution;
  Paths64 solution_open;
  ClipType ct = ClipType::NoClip;
  FillRule fr = FillRule::EvenOdd;
  int64_t area = 0, count = 0;
  bool success = false;
  ASSERT_TRUE(LoadTestNum(ifs, 1, subject, subject_open, clip, area, count, ct, fr));
  Clipper64 c;
  c.AddSubject(subject);
  c.AddOpenSubject(subject_open);
  c.AddClip(clip);
  c.Execute(ct, fr, solution, solution_open);
  EXPECT_TRUE(CheckPolytreeFullyContainsChildren(solution));
}

void PolyPathContainsPoint(const PolyPath64& pp, const Point64 pt, int& counter)
{
  if (pp.Polygon().size() > 0)
  {
    if (PointInPolygon(pt, pp.Polygon()) != PointInPolygonResult::IsOutside)
    {
      if (pp.IsHole()) --counter;
      else  ++counter;
    }
  }
  for (const auto& child : pp)
    PolyPathContainsPoint(*child, pt, counter);
}

bool PolytreeContainsPoint(const PolyPath64& pp, const Point64 pt)
{
  int counter = 0;
  for (const auto& child : pp)
    PolyPathContainsPoint(*child, pt, counter);
  EXPECT_GE(counter, 0); //ie 'pt' can't be inside more holes than outers
  return counter != 0;
}

void GetPolyPathArea(const PolyPath64& pp, double& area)
{
  area += Area(pp.Polygon());
  for (const auto& child : pp)
    GetPolyPathArea(*child, area);
}
double GetPolytreeArea(const PolyPath64& pp)
{
  double result = 0;
  for (const auto& child : pp)
    GetPolyPathArea(*child, result);
  return result;
}

TEST(Clipper2Tests, TestPolytreeHoles2)
{
  std::ifstream ifs("PolytreeHoleOwner2.txt");
  ASSERT_TRUE(ifs);
  ASSERT_TRUE(ifs.good());
  Paths64 subject, subject_open, clip;
  ClipType ct = ClipType::NoClip;
  FillRule fr = FillRule::EvenOdd;
  int64_t area = 0, count = 0;
  ASSERT_TRUE(LoadTestNum(ifs, 1, subject, subject_open, clip, area, count, ct, fr));
  const std::vector<Point64> points_of_interest_outside = {
     Point64(21887, 10420),
     Point64(21726, 10825),
     Point64(21662, 10845),
     Point64(21617, 10890)
  };
  // confirm that each 'points_of_interest_outside' is outside every subject,
  for (const auto& poi_outside : points_of_interest_outside)
  {
    int outside_subject_count = 0;
    for (const auto& path : subject)
      if (PointInPolygon(poi_outside, path) != PointInPolygonResult::IsOutside)
        ++outside_subject_count;
    EXPECT_EQ(outside_subject_count, 0);
  }
  const std::vector<Point64> points_of_interest_inside = {
     Point64(21887, 10430),
     Point64(21843, 10520),
     Point64(21810, 10686),
     Point64(21900, 10461)
  };
  // confirm that each 'points_of_interest_inside' is inside a subject,
  // and inside only one subject (to exclude possible subject holes)
  for (const auto& poi_inside : points_of_interest_inside)
  {
    int inside_subject_count = 0;
    for (const auto& path : subject)
    {
      if (PointInPolygon(poi_inside, path) != PointInPolygonResult::IsOutside)
        ++inside_subject_count;
    }
    EXPECT_EQ(inside_subject_count, 1);
  }
  PolyTree64 solution_tree;
  Paths64 solution_open;
  Clipper64 c;
  c.AddSubject(subject);
  c.AddOpenSubject(subject_open);
  c.AddClip(clip);
  c.Execute(ct, FillRule::Negative, solution_tree, solution_open);
  const auto solution_paths = PolyTreeToPaths64(solution_tree);
  ASSERT_FALSE(solution_paths.empty());
  const double subject_area = -Area(subject); //negate (see fillrule)
  const double solution_tree_area = GetPolytreeArea(solution_tree);
  const double solution_paths_area = Area(solution_paths);
  // 1a. check solution_paths_area  is smaller than subject_area
  EXPECT_LT(solution_paths_area, subject_area);
  // 1b. but not too much smaller
  EXPECT_GT(solution_paths_area, (subject_area * 0.92));
  // 2. check solution_tree's area matches solution_paths' area
  EXPECT_NEAR(solution_tree_area, solution_paths_area, 0.0001);
  // 3. check that all children are inside their parents
  EXPECT_TRUE(CheckPolytreeFullyContainsChildren(solution_tree));
  // 4. confirm all 'point_of_interest_outside' are outside polytree
  for (const auto& poi_outside : points_of_interest_outside)
    EXPECT_FALSE(PolytreeContainsPoint(solution_tree, poi_outside));
  // 5. confirm all 'point_of_interest_inside' are inside polytree
  for (const auto& poi_inside : points_of_interest_inside)
    EXPECT_TRUE(PolytreeContainsPoint(solution_tree, poi_inside));
}

TEST(Clipper2Tests, TestPolytreeHoles3)
{
  Paths64 subject, clip, sol;
  PolyTree64 solution;
  Clipper64 c;
  subject.push_back(MakePath({ 1072,501, 1072,501, 1072,539, 1072,539, 1072,539, 870,539,
    870,539, 870,539, 870,520, 894,520, 898,524, 911,524, 915,520, 915,520, 936,520,
    940,524, 953,524, 957,520, 957,520, 978,520, 983,524, 995,524, 1000,520, 1021,520,
    1025,524, 1038,524, 1042,520, 1038,516, 1025,516, 1021,520, 1000,520, 995,516,
    983,516, 978,520, 957,520, 953,516, 940,516, 936,520, 915,520, 911,516, 898,516,
    894,520, 870,520, 870,516, 870,501, 870,501, 870,501, 1072,501 }));
  clip.push_back(MakePath({ 870,501, 971,501, 971,539, 870,539 }));
  c.AddSubject(subject);
  c.AddClip(clip);
  c.Execute(ClipType::Intersection, FillRule::NonZero, solution);
  EXPECT_TRUE(solution.Count() == 1 && solution[0]->Count() == 2);
}

TEST(Clipper2Tests, TestPolytreeHoles4) //#618
{
  Paths64 subject;
  PolyTree64 solution;
  Clipper64 c;
  subject.push_back(MakePath({ 50,500, 50,300, 100,300, 100,350, 150,350,
    150,250, 200,250, 200,450, 350,450, 350,200, 400,200, 400,225, 450,225,
    450,175, 400,175, 400,200, 350,200, 350,175, 200,175, 200,250, 150,250,
    150,200, 100,200, 100,300, 50,300, 50,125, 500,125, 500,500 }));
  subject.push_back(MakePath({ 250,425, 250,375,  300,375, 300,425 }));
  c.AddSubject(subject);
  c.Execute(ClipType::Union, FillRule::NonZero, solution);
  // Polytree root
  //   +- Polygon with 3 holes.
  //      +- Hole with 1 nested polygon.
  //         +-Polygon
  //      +- Hole
  //      +- Hole
  EXPECT_TRUE(solution.Count() == 1 && solution[0]->Count() == 3);
}

TEST(Clipper2Tests, TestPolytreeHoles5)
{
  Paths64 subject, clip;
  subject.push_back(MakePath({ 0,30, 400,30, 400,100, 0,100 }));
  clip.push_back(MakePath({ 20,30, 30,30, 30,150, 20,150 }));
  clip.push_back(MakePath({ 200,0, 300,0, 300,30, 280,30, 280,20, 220,20, 220,30, 200,30 }));
  clip.push_back(MakePath({ 200,50, 300,50, 300,80, 200,80 }));
  Clipper64 c;
  c.AddSubject(subject);
  c.AddClip(clip);
  PolyTree64 tree;
  c.Execute(ClipType::Xor, FillRule::NonZero, tree);
  ////std::cout << tree << std::endl;
  //Polytree with 3 polygons.
  //  + -Polygon (2) contains 2 holes.
  EXPECT_TRUE(tree.Count() == 3 && tree[2]->Count() == 2);
}

TEST(Clipper2Tests, TestPolytreeHoles6) //#618
{
  Paths64 subject, clip;
  subject.push_back(MakePath({ 150,50, 200,50, 200,100, 150,100 }));
  subject.push_back(MakePath({ 125,100, 150,100, 150,150, 125,150 }));
  subject.push_back(MakePath({ 225,50, 300,50, 300,80, 225,80 }));
  subject.push_back(MakePath({ 225,100, 300,100, 300,150, 275,150, 275,175, 260,175,
    260,250, 235,250, 235,300, 275,300, 275,275, 300,275, 300,350, 225,350 }));
  subject.push_back(MakePath({ 300,150, 350,150, 350,175, 300,175 }));
  clip.push_back(MakePath({ 0,0, 400,0, 400,50, 0,50 }));
  clip.push_back(MakePath({ 0,100, 400,100, 400,150, 0,150 }));
  clip.push_back(MakePath({ 260,175, 325,175, 325,275, 260,275 }));
  Clipper64 c;
  c.AddSubject(subject);
  c.AddClip(clip);
  PolyTree64 tree;
  c.Execute(ClipType::Xor, FillRule::NonZero, tree);
  ////std::cout << tree << std::endl;
  //Polytree with 3 polygons.
  //  + -Polygon (2) contains 1 holes.
  EXPECT_TRUE(tree.Count() == 3 && tree[2]->Count() == 1);
}

TEST(Clipper2Tests, TestPolytreeHoles7) //#618
{
  Paths64 subject;
  subject.push_back(MakePath({ 0, 0, 100000, 0, 100000, 100000, 200000, 100000,
    200000, 0, 300000, 0, 300000, 200000, 0, 200000 }));
  subject.push_back(MakePath({ 0, 0, 0, -100000, 250000, -100000, 250000, 0 }));
  PolyTree64 polytree;
  Clipper64 c;
  c.AddSubject(subject);
  c.Execute(ClipType::Union, FillRule::NonZero, polytree);
  //std::cout << polytree << std::endl;
  EXPECT_TRUE(polytree.Count() == 1 && polytree[0]->Count() == 1);
}

TEST(Clipper2Tests, TestPolytreeHoles8) // #942
{
  Paths64 subject;
  PolyTree64 solution;
  Clipper64 c;
  subject.push_back(MakePath({ 1588700,-8717600, 1616200,-8474800,
    1588700,-8474800 }));
  subject.push_back(MakePath({ 13583800,-15601600, 13582800,-15508500,
    13555300,-15508500, 13555500,-15182200, 13010900,-15185400 }));
  subject.push_back(MakePath({ 956700,-3092300, 1152600,3147400, 25600,3151700 }));
  subject.push_back(MakePath({ 22575900,-16604000, 31286800,-12171900,
    31110200,4882800, 30996200,4826300, 30414400,5447400, 30260000,5391500,
    29662200,5805400, 28844500,5337900, 28435000,5789300, 27721400,5026400,
    22876300,5034300, 21977700,4414900, 21148000,4654700, 20917600,4653400,
    19334300,12411000, -2591700,12177200, 53200,3151100, -2564300,12149800,
    7819400,4692400, 10116000,5228600, 6975500,3120100, 7379700,3124700,
    11037900,596200, 12257000,2587800, 12257000,596200, 15227300,2352700,
    18444400,1112100, 19961100,5549400, 20173200,5078600, 20330000,5079300,
    20970200,4544300, 20989600,4563700, 19465500,1112100, 21611600,4182100,
    22925100,1112200, 22952700,1637200, 23059000,1112200, 24908100,4181200,
    27070100,3800600, 27238000,3800700, 28582200,520300, 29367800,1050100,
    29291400,179400, 29133700,360700, 29056700,312600, 29121900,332500,
    29269900,162300, 28941400,213100, 27491300,-3041500, 27588700,-2997800,
    22104900,-16142800, 13010900,-15603000, 13555500,-15182200,
    13555300,-15508500, 13582800,-15508500, 13583100,-15154700,
    1588700,-8822800, 1588700,-8379900, 1588700,-8474800, 1616200,-8474800,
    1003900,-630100, 1253300,-12284500, 12983400,-16239900 }));
  subject.push_back(MakePath({ 198200,12149800, 1010600,12149800, 1011500,11859600 }));
  subject.push_back(MakePath({ 21996700,-7432000, 22096700,-7432000, 22096700,-7332000 }));
  c.AddSubject(subject);
  c.Execute(ClipType::Union, FillRule::NonZero, solution);
  EXPECT_TRUE(solution.Count() == 1 && solution[0]->Count() == 2 && (*solution[0])[1]->Count() == 1);
}

TEST(Clipper2Tests, TestPolytreeHoles9) // #957
{
  Paths64 subject = {
    MakePath({ 77910,46865, 78720,46865, 78720,48000, 77910,48000, 77910,46865}),
    MakePath({ 82780,53015, 93600,53015, 93600,54335, 82780,54335, 82780,53015}),
    MakePath({ 82780,48975, 84080,48975, 84080,53015, 82780,53015, 82780,48975}),
    MakePath({ 77910,48000, 84080,48000, 84080,48975, 77910,48975, 77910,48000}),
    MakePath({ 89880,40615, 90700,40615, 90700,46865, 89880,46865, 89880,40615}),
    MakePath({ 92700,54335, 93600,54335, 93600,61420, 92700,61420, 92700,54335}),
    MakePath({ 78950,47425, 84080,47425, 84080,47770, 78950,47770, 78950,47425}),
    MakePath({ 82780,61420, 93600,61420, 93600,62435, 82780,62435, 82780,61420}),
    MakePath({ 101680,63085, 100675,63085, 100675,47770, 100680,47770, 100680,40615, 101680,40615, 101680,63085}),
    MakePath({ 76195,39880, 89880,39880, 89880,41045, 76195,41045, 76195,39880}),
    MakePath({ 85490,56145, 90520,56145, 90520,59235, 85490,59235, 85490,56145}),
    MakePath({ 89880,39880, 101680,39880, 101680,40615, 89880,40615, 89880,39880}),
    MakePath({ 89880,46865, 100680,46865, 100680,47770, 89880,47770, 89880,46865}),
    MakePath({ 82780,54335, 83280,54335, 83280,61420, 82780,61420, 82780,54335}),
    MakePath({ 76195,41045, 76855,41045, 76855,62665, 76195,62665, 76195,41045}),
    MakePath({ 76195,62665, 100675,62665, 100675,63085, 76195,63085, 76195,62665}),
    MakePath({ 82780,41045, 84080,41045, 84080,47425, 82780,47425, 82780,41045}) };

  PolyTree64 solution;
  Clipper64 c;
  c.AddSubject(subject);
  c.Execute(ClipType::Union, FillRule::NonZero, solution);

  //  Polytree with 1 polygon.
  //    + -Polygon(0) contains 2 holes.
  //    + -Hole(0) contains 1 nested polygon.
  //    + -Polygon(0) contains 1 hole.
  //    + -Hole(0) contains 1 nested polygon.

  EXPECT_TRUE(solution.Count() == 1 && solution[0]->Count() == 2 && (*solution[0])[0]->Count() == 1);
}


TEST(Clipper2Tests, TestPolytreeHoles10) // #973
{
  Paths64 subject = {
      MakePath({ 0,0,  79530,0,  79530,940,  0,940,  0,0 }),
      MakePath({ 0,33360,  79530,33360,  79530,34300,  0,34300,  0,33360 }),
      MakePath({ 78470,940,  79530,940,  79530,33360,  78470,33360,  78470,940 }),
      MakePath({ 0,940,  940,940,  940,33360,  0,33360,  0,940 }),
      MakePath({ 29290,940,  30350,940,  30350,33360,  29290,33360,  29290,940 })
  };

  Clipper64	c;
  c.AddSubject(subject);

  Paths64 sol;
  PolyTree64	solution;
  c.Execute(ClipType::Union, FillRule::NonZero, solution);

  // cout << solution << endl;
  // //  Polytree with 1 polygon.
  // //    + -Polygon(0) contains 2 holes.
  EXPECT_TRUE(solution.Count() == 1 && solution[0]->Count() == 2);
}

