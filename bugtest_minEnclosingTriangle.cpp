/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//  INFORMATION REGARDING THE CONTRIBUTION:
//
//  Author: Ovidiu Parvu
//  Affiliation: Brunel University
//  Created: 11.09.2013
//  E-mail: <ovidiu.parvu[AT]gmail.com>
//  Web: http://people.brunel.ac.uk/~cspgoop
//
//  These functions were implemented during Ovidiu Parvu's first year as a PhD student at
//  Brunel University, London, UK. The PhD project is supervised by prof. David Gilbert (principal)
//  and prof. Nigel Saunders (second).
//
//  THE IMPLEMENTATION OF THE MODULES IS BASED ON THE FOLLOWING PAPERS:
//
//  [1] V. Klee and M. C. Laskowski, "Finding the smallest triangles containing a given convex
//  polygon", Journal of Algorithms, vol. 6, no. 3, pp. 359-375, Sep. 1985.
//  [2] J. O'Rourke, A. Aggarwal, S. Maddila, and M. Baldwin, "An optimal algorithm for finding
//  minimal enclosing triangles", Journal of Algorithms, vol. 7, no. 2, pp. 258-269, Jun. 1986.
//
//  The overall complexity of the algorithm is theta(n) where "n" represents the number
//  of vertices in the convex polygon.
//
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Copyright (C) 2013, Ovidiu Parvu, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <fstream>  // file


///////////////////////////////// Constants definitions //////////////////////////////////


// Intersection of line and polygon

#define INTERSECTS_BELOW        1
#define INTERSECTS_ABOVE        2
#define INTERSECTS_CRITICAL     3

// Error messages

#define ERR_SIDE_B_GAMMA        "The position of side B could not be determined, because gamma(b) could not be computed."
#define ERR_VERTEX_C_ON_SIDE_B  "The position of the vertex C on side B could not be determined, because the considered lines do not intersect."

// Possible values for validation flag

#define VALIDATION_SIDE_A_TANGENT   0
#define VALIDATION_SIDE_B_TANGENT   1
#define VALIDATION_SIDES_FLUSH      2

// Threshold value for comparisons

#define EPSILON 1E-5


////////////////////////////// Helper functions declarations /////////////////////////////


namespace minEnclosingTriangle {

static void advance(unsigned int &index, unsigned int nrOfPoints);

static void advanceBToRightChain(const std::vector<cv::Point2f> &polygon,
                                 unsigned int nrOfPoints, unsigned int &b,
                                 unsigned int c);

static bool almostEqual(double number1, double number2);

static double angleOfLineWrtOxAxis(const cv::Point2f &a, const cv::Point2f &b);

static bool areEqualPoints(const cv::Point2f &point1, const cv::Point2f &point2);

static bool areIdenticalLines(const std::vector<double> &side1Params,
                              const std::vector<double> &side2Params, double sideCExtraParam);

static bool areIdenticalLines(double a1, double b1, double c1, double a2, double b2, double c2);

static bool areIntersectingLines(const std::vector<double> &side1Params,
                                 const std::vector<double> &side2Params,
                                 double sideCExtraParam, cv::Point2f &intersectionPoint1,
                                 cv::Point2f &intersectionPoint2);

static bool areOnTheSameSideOfLine(const cv::Point2f &p1, const cv::Point2f &p2,
                                   const cv::Point2f &a, const cv::Point2f &b);

static double areaOfTriangle(const cv::Point2f &a, const cv::Point2f &b, const cv::Point2f &c);

static void copyResultingTriangle(const std::vector<cv::Point2f> &resultingTriangle, cv::OutputArray triangle);

static void createConvexHull(cv::InputArray points, std::vector<cv::Point2f> &polygon);

static double distanceBtwPoints(const cv::Point2f &a, const cv::Point2f &b);

static double distanceFromPointToLine(const cv::Point2f &a, const cv::Point2f &linePointB,
                                      const cv::Point2f &linePointC);

static bool findGammaIntersectionPoints(const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                                        unsigned int c, unsigned int polygonPointIndex,
                                        const cv::Point2f &side1StartVertex, const cv::Point2f &side1EndVertex,
                                        const cv::Point2f &side2StartVertex, const cv::Point2f &side2EndVertex,
                                        cv::Point2f &intersectionPoint1, cv::Point2f &intersectionPoint2);

static void findMinEnclosingTriangle(cv::InputArray points,
                                     CV_OUT cv::OutputArray triangle, CV_OUT double &area);

static void findMinEnclosingTriangle(const std::vector<cv::Point2f> &polygon,
                                     std::vector<cv::Point2f> &triangle, double &area);

static void findMinimumAreaEnclosingTriangle(const std::vector<cv::Point2f> &polygon,
                                             std::vector<cv::Point2f> &triangle, double &area);

static cv::Point2f findVertexCOnSideB(const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                                      unsigned int a, unsigned int c,
                                      const cv::Point2f &sideBStartVertex,
                                      const cv::Point2f &sideBEndVertex,
                                      const cv::Point2f &sideCStartVertex,
                                      const cv::Point2f &sideCEndVertex);

static bool gamma(unsigned int polygonPointIndex, cv::Point2f &gammaPoint,
                  const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                  unsigned int a, unsigned int c);

static bool greaterOrEqual(double number1, double number2);

static double height(const cv::Point2f &polygonPoint, const std::vector<cv::Point2f> &polygon,
                     unsigned int nrOfPoints, unsigned int c);

static double height(unsigned int polygonPointIndex, const std::vector<cv::Point2f> &polygon,
                     unsigned int nrOfPoints, unsigned int c);

static void initialise(std::vector<cv::Point2f> &triangle, double &area);

static unsigned int intersects(double angleGammaAndPoint, unsigned int polygonPointIndex,
                               const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                               unsigned int c);

static bool intersectsAbove(const cv::Point2f &gammaPoint, unsigned int polygonPointIndex,
                            const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                            unsigned int c);

static unsigned int intersectsAboveOrBelow(unsigned int succPredIndex, unsigned int pointIndex,
                                           const std::vector<cv::Point2f> &polygon,
                                           unsigned int nrOfPoints, unsigned int c);

static bool intersectsBelow(const cv::Point2f &gammaPoint, unsigned int polygonPointIndex,
                            const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                            unsigned int c);

static bool isAngleBetween(double angle1, double angle2, double angle3);

static bool isAngleBetweenNonReflex(double angle1, double angle2, double angle3);

static bool isFlushAngleBtwPredAndSucc(double &angleFlushEdge, double anglePred, double angleSucc);

static bool isGammaAngleBtw(double &gammaAngle, double angle1, double angle2);

static bool isGammaAngleEqualTo(double &gammaAngle, double angle);

static bool isLocalMinimalTriangle(cv::Point2f &vertexA, cv::Point2f &vertexB,
                                   cv::Point2f &vertexC, const std::vector<cv::Point2f> &polygon,
                                   unsigned int nrOfPoints, unsigned int a, unsigned int b,
                                   unsigned int validationFlag, const cv::Point2f &sideAStartVertex,
                                   const cv::Point2f &sideAEndVertex, const cv::Point2f &sideBStartVertex,
                                   const cv::Point2f &sideBEndVertex, const cv::Point2f &sideCStartVertex,
                                   const cv::Point2f &sideCEndVertex);

static bool isNotBTangency(const std::vector<cv::Point2f> &polygon,
                           unsigned int nrOfPoints, unsigned int a, unsigned int b,
                           unsigned int c);

static bool isOppositeAngleBetweenNonReflex(double angle1, double angle2, double angle3);

static bool isPointOnLineSegment(const cv::Point2f &point, const cv::Point2f &lineSegmentStart,
                                 const cv::Point2f &lineSegmentEnd);

static bool isValidMinimalTriangle(const cv::Point2f &vertexA, const cv::Point2f &vertexB,
                                   const cv::Point2f &vertexC, const std::vector<cv::Point2f> &polygon,
                                   unsigned int nrOfPoints, unsigned int a, unsigned int b,
                                   unsigned int validationFlag, const cv::Point2f &sideAStartVertex,
                                   const cv::Point2f &sideAEndVertex, const cv::Point2f &sideBStartVertex,
                                   const cv::Point2f &sideBEndVertex, const cv::Point2f &sideCStartVertex,
                                   const cv::Point2f &sideCEndVertex);

static bool lessOrEqual(double number1, double number2);

static void lineEquationDeterminedByPoints(const cv::Point2f &p, const cv::Point2f &q,
                                           double &a, double &b, double &c);

static std::vector<double> lineEquationParameters(const cv::Point2f& p, const cv::Point2f &q);

static bool lineIntersection(const cv::Point2f &a1, const cv::Point2f &b1, const cv::Point2f &a2,
                             const cv::Point2f &b2, cv::Point2f &intersection);

static bool lineIntersection(double a1, double b1, double c1, double a2, double b2, double c2,
                             cv::Point2f &intersection);

static double maximum(double number1, double number2, double number3);

static cv::Point2f middlePoint(const cv::Point2f &a, const cv::Point2f &b);

static bool middlePointOfSideB(cv::Point2f &middlePointOfSideB, const cv::Point2f &sideAStartVertex,
                               const cv::Point2f &sideAEndVertex, const cv::Point2f &sideBStartVertex,
                               const cv::Point2f &sideBEndVertex, const cv::Point2f &sideCStartVertex,
                               const cv::Point2f &sideCEndVertex);

static void moveAIfLowAndBIfHigh(const std::vector<cv::Point2f> &polygon,
                                 unsigned int nrOfPoints, unsigned int &a, unsigned int &b,
                                 unsigned int c);

static double oppositeAngle(double angle);

static unsigned int predecessor(unsigned int index, unsigned int nrOfPoints);

static void returnMinimumAreaEnclosingTriangle(const std::vector<cv::Point2f> &polygon,
                                               std::vector<cv::Point2f> &triangle, double &area);

static void searchForBTangency(const std::vector<cv::Point2f> &polygon,
                               unsigned int nrOfPoints, unsigned int a, unsigned int &b,
                               unsigned int c);

static int sign(double number);

static unsigned int successor(unsigned int index, unsigned int nrOfPoints);

static void updateMinimumAreaEnclosingTriangle(std::vector<cv::Point2f> &triangle, double &area,
                                               const cv::Point2f &vertexA, const cv::Point2f &vertexB,
                                               const cv::Point2f &vertexC);

static void updateSideB(const std::vector<cv::Point2f> &polygon,
                        unsigned int nrOfPoints, unsigned int a, unsigned int b,
                        unsigned int c, unsigned int &validationFlag,
                        cv::Point2f &sideBStartVertex, cv::Point2f &sideBEndVertex);

static void updateSidesBA(const std::vector<cv::Point2f> &polygon,
                          unsigned int nrOfPoints, unsigned int a, unsigned int b,
                          unsigned int c, unsigned int &validationFlag,
                          cv::Point2f &sideAStartVertex, cv::Point2f &sideAEndVertex,
                          cv::Point2f &sideBStartVertex, cv::Point2f &sideBEndVertex,
                          const cv::Point2f &sideCStartVertex, const cv::Point2f &sideCEndVertex);

static void updateSidesCA(const std::vector<cv::Point2f> &polygon,
                          unsigned int nrOfPoints, unsigned int a, unsigned int c,
                          cv::Point2f &sideAStartVertex, cv::Point2f &sideAEndVertex,
                          cv::Point2f &sideCStartVertex, cv::Point2f &sideCEndVertex);

}


///////////////////////////////////// Main functions /////////////////////////////////////


//! Find the minimum enclosing triangle for the given set of points and return its area
/*!
* @param points         Set of points
* @param triangle       Minimum area triangle enclosing the given set of points

double cv::minEnclosingTriangle(cv::InputArray points, CV_OUT cv::OutputArray triangle) {
    double area;

    minEnclosingTriangle::findMinEnclosingTriangle(points, triangle, area);

    return area;
}
*/

/////////////////////////////// Helper functions definition //////////////////////////////


namespace minEnclosingTriangle {

//! Find the minimum enclosing triangle and its area
/*!
* @param points         Set of points
* @param triangle       Minimum area triangle enclosing the given set of points
* @param area           Area of the minimum area enclosing triangle
*/
static void findMinEnclosingTriangle(cv::InputArray points,
                                     CV_OUT cv::OutputArray triangle, CV_OUT double &area) {
    std::vector<cv::Point2f> resultingTriangle, polygon;

    createConvexHull(points, polygon);
    findMinEnclosingTriangle(polygon, resultingTriangle, area);
    copyResultingTriangle(resultingTriangle, triangle);
}

//! Create the convex hull of the given set of points
/*!
* @param points     The provided set of points
* @param polygon    The polygon representing the convex hull of the points
*/
static void createConvexHull(cv::InputArray points, std::vector<cv::Point2f> &polygon) {
    cv::Mat pointsMat = points.getMat();
    std::vector<cv::Point2f> pointsVector;

    CV_Assert((pointsMat.checkVector(2) > 0) &&
              ((pointsMat.depth() == CV_32F) || (pointsMat.depth() == CV_32S)));

    pointsMat.convertTo(pointsVector, CV_32F);

    convexHull(pointsVector, polygon, true, true);
}

//! Find the minimum enclosing triangle and its area
/*!
* The overall complexity of the algorithm is theta(n) where "n" represents the number
* of vertices in the convex polygon
*
* @param polygon    The polygon representing the convex hull of the points
* @param triangle   Minimum area triangle enclosing the given polygon
* @param area       Area of the minimum area enclosing triangle
*/
static void findMinEnclosingTriangle(const std::vector<cv::Point2f> &polygon,
                                     std::vector<cv::Point2f> &triangle, double &area) {
    initialise(triangle, area);

    if (polygon.size() > 3) {
        findMinimumAreaEnclosingTriangle(polygon, triangle, area);
    } else {
        returnMinimumAreaEnclosingTriangle(polygon, triangle, area);
    }
}

//! Copy resultingTriangle to the OutputArray triangle
/*!
* @param resultingTriangle  Minimum area triangle enclosing the given polygon found by the algorithm
* @param triangle           Minimum area triangle enclosing the given polygon returned to the user
*/
static void copyResultingTriangle(const std::vector<cv::Point2f> &resultingTriangle,
                                  cv::OutputArray triangle) {
    cv::Mat(resultingTriangle).copyTo(triangle);
}

//! Initialisation function
/*!
* @param triangle       Minimum area triangle enclosing the given polygon
* @param area           Area of the minimum area enclosing triangle
*/
static void initialise(std::vector<cv::Point2f> &triangle, double &area) {
    area = std::numeric_limits<double>::max();

    // Clear all points previously stored in the vector
    triangle.clear();
}

//! Find the minimum area enclosing triangle for the given polygon
/*!
* @param polygon    The polygon representing the convex hull of the points
* @param triangle   Minimum area triangle enclosing the given polygon
* @param area       Area of the minimum area enclosing triangle
*/
static void findMinimumAreaEnclosingTriangle(const std::vector<cv::Point2f> &polygon,
                                             std::vector<cv::Point2f> &triangle, double &area) {
    // Algorithm specific variables

    unsigned int validationFlag;

    cv::Point2f vertexA, vertexB, vertexC;

    cv::Point2f sideAStartVertex, sideAEndVertex;
    cv::Point2f sideBStartVertex, sideBEndVertex;
    cv::Point2f sideCStartVertex, sideCEndVertex;

    unsigned int a, b, c;
    unsigned int nrOfPoints;

    // Variables initialisation

    nrOfPoints = static_cast<unsigned int>(polygon.size());

    a = 1;
    b = 2;
    c = 0;

    // Main algorithm steps

    for (c = 0; c < nrOfPoints; c++) {
        advanceBToRightChain(polygon, nrOfPoints, b, c);
        moveAIfLowAndBIfHigh(polygon, nrOfPoints, a, b, c);
        searchForBTangency(polygon, nrOfPoints, a ,b, c);

        updateSidesCA(polygon, nrOfPoints, a, c, sideAStartVertex, sideAEndVertex,
                      sideCStartVertex, sideCEndVertex);

        if (isNotBTangency(polygon, nrOfPoints, a, b, c)) {
            updateSidesBA(polygon, nrOfPoints, a, b, c, validationFlag, sideAStartVertex,
                          sideAEndVertex, sideBStartVertex, sideBEndVertex,
                          sideCStartVertex, sideCEndVertex);
        } else {
            updateSideB(polygon, nrOfPoints, a, b, c, validationFlag,
                        sideBStartVertex,  sideBEndVertex);
        }

        if (isLocalMinimalTriangle(vertexA, vertexB, vertexC, polygon, nrOfPoints, a, b,
                                   validationFlag, sideAStartVertex, sideAEndVertex,
                                   sideBStartVertex, sideBEndVertex, sideCStartVertex,
                                   sideCEndVertex)) {
            updateMinimumAreaEnclosingTriangle(triangle, area, vertexA, vertexB, vertexC);
        }
    }
}

//! Return the minimum area enclosing (pseudo-)triangle in case the convex polygon has at most three points
/*!
* @param polygon    The polygon representing the convex hull of the points
* @param triangle   Minimum area triangle enclosing the given polygon
* @param area       Area of the minimum area enclosing triangle
*/
static void returnMinimumAreaEnclosingTriangle(const std::vector<cv::Point2f> &polygon,
                                               std::vector<cv::Point2f> &triangle, double &area) {
    unsigned int nrOfPoints = static_cast<unsigned int>(polygon.size());

    for (int i = 0; i < 3; i++) {
        triangle.push_back(polygon[i % nrOfPoints]);
    }

    area = areaOfTriangle(triangle[0], triangle[1], triangle[2]);
}

//! Advance b to the right chain
/*!
* See paper [2] for more details
*
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param b                  Index b
* @param c                  Index c
*/
static void advanceBToRightChain(const std::vector<cv::Point2f> &polygon,
                                 unsigned int nrOfPoints, unsigned int &b,
                                 unsigned int c) {
    while (greaterOrEqual(height(successor(b, nrOfPoints), polygon, nrOfPoints, c),
                          height(b, polygon, nrOfPoints, c))) {
        advance(b, nrOfPoints);
    }
}

//! Move "a" if it is low and "b" if it is high
/*!
* See paper [2] for more details
*
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param c                  Index c
*/
static void moveAIfLowAndBIfHigh(const std::vector<cv::Point2f> &polygon,
                                 unsigned int nrOfPoints, unsigned int &a, unsigned int &b,
                                 unsigned int c) {
    cv::Point2f gammaOfA;

    while(height(b, polygon, nrOfPoints, c) > height(a, polygon, nrOfPoints, c)) {
        if ((gamma(a, gammaOfA, polygon, nrOfPoints, a, c)) && (intersectsBelow(gammaOfA, b, polygon, nrOfPoints, c))) {
            advance(b, nrOfPoints);
        } else {
            advance(a, nrOfPoints);
        }
    }
}

//! Search for the tangency of side B
/*!
* See paper [2] for more details
*
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param c                  Index c
*/
static void searchForBTangency(const std::vector<cv::Point2f> &polygon,
                               unsigned int nrOfPoints, unsigned int a, unsigned int &b,
                               unsigned int c) {
    cv::Point2f gammaOfB;

    while (((gamma(b, gammaOfB, polygon, nrOfPoints, a, c)) &&
            (intersectsBelow(gammaOfB, b, polygon, nrOfPoints, c))) &&
           (greaterOrEqual(height(b, polygon, nrOfPoints, c),
                           height(predecessor(a, nrOfPoints), polygon, nrOfPoints, c)))
          ) {
        advance(b, nrOfPoints);
    }
}

//! Check if tangency for side B was not obtained
/*!
* See paper [2] for more details
*
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param c                  Index c
*/
static bool isNotBTangency(const std::vector<cv::Point2f> &polygon,
                           unsigned int nrOfPoints, unsigned int a, unsigned int b,
                           unsigned int c) {
    cv::Point2f gammaOfB;

    if (((gamma(b, gammaOfB, polygon, nrOfPoints, a, c)) &&
         (intersectsAbove(gammaOfB, b, polygon, nrOfPoints, c))) ||
        (height(b, polygon, nrOfPoints, c) < height(predecessor(a, nrOfPoints), polygon, nrOfPoints, c))) {
        return true;
    }

    return false;
}

//! Update sides A and C
/*!
* Side C will have as start and end vertices the polygon points "c" and "c-1"
* Side A will have as start and end vertices the polygon points "a" and "a-1"
*
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param c                  Index c
* @param sideAStartVertex   Start vertex for defining side A
* @param sideAEndVertex     End vertex for defining side A
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static void updateSidesCA(const std::vector<cv::Point2f> &polygon,
                          unsigned int nrOfPoints, unsigned int a, unsigned int c,
                          cv::Point2f &sideAStartVertex, cv::Point2f &sideAEndVertex,
                          cv::Point2f &sideCStartVertex, cv::Point2f &sideCEndVertex) {
    sideCStartVertex = polygon[predecessor(c, nrOfPoints)];
    sideCEndVertex = polygon[c];

    sideAStartVertex = polygon[predecessor(a, nrOfPoints)];
    sideAEndVertex = polygon[a];
}

//! Update sides B and possibly A if tangency for side B was not obtained
/*!
* See paper [2] for more details
*
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param c                  Index c
* @param validationFlag     Flag used for validation
* @param sideAStartVertex   Start vertex for defining side A
* @param sideAEndVertex     End vertex for defining side A
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static void updateSidesBA(const std::vector<cv::Point2f> &polygon,
                          unsigned int nrOfPoints, unsigned int a, unsigned int b,
                          unsigned int c, unsigned int &validationFlag,
                          cv::Point2f &sideAStartVertex, cv::Point2f &sideAEndVertex,
                          cv::Point2f &sideBStartVertex, cv::Point2f &sideBEndVertex,
                          const cv::Point2f &sideCStartVertex, const cv::Point2f &sideCEndVertex) {
    // Side B is flush with edge [b, b-1]
    sideBStartVertex = polygon[predecessor(b, nrOfPoints)];
    sideBEndVertex = polygon[b];

    // Find middle point of side B
    cv::Point2f sideBMiddlePoint;

    if ((middlePointOfSideB(sideBMiddlePoint, sideAStartVertex, sideAEndVertex, sideBStartVertex,
                            sideBEndVertex, sideCStartVertex, sideCEndVertex)) &&
        (height(sideBMiddlePoint, polygon, nrOfPoints, c) <
         height(predecessor(a, nrOfPoints), polygon, nrOfPoints, c))) {
        sideAStartVertex = polygon[predecessor(a, nrOfPoints)];
        sideAEndVertex = findVertexCOnSideB(polygon, nrOfPoints, a, c,
                                            sideBStartVertex, sideBEndVertex,
                                            sideCStartVertex, sideCEndVertex);

        validationFlag = VALIDATION_SIDE_A_TANGENT;
    } else {
        validationFlag = VALIDATION_SIDES_FLUSH;
    }
}

//! Set side B if tangency for side B was obtained
/*!
* See paper [2] for more details
*
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param c                  Index c
* @param validationFlag     Flag used for validation
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
*/
static void updateSideB(const std::vector<cv::Point2f> &polygon,
                        unsigned int nrOfPoints, unsigned int a, unsigned int b,
                        unsigned int c, unsigned int &validationFlag,
                        cv::Point2f &sideBStartVertex, cv::Point2f &sideBEndVertex) {
    if (!gamma(b, sideBStartVertex, polygon, nrOfPoints, a, c)) {
        CV_Error(cv::Error::StsInternal, ERR_SIDE_B_GAMMA);
    }

    sideBEndVertex = polygon[b];

    validationFlag = VALIDATION_SIDE_B_TANGENT;
}

//! Update the triangle vertices after all sides were set and check if a local minimal triangle was found or not
/*!
* See paper [2] for more details
*
* @param vertexA            Vertex A of the enclosing triangle
* @param vertexB            Vertex B of the enclosing triangle
* @param vertexC            Vertex C of the enclosing triangle
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param validationFlag     Flag used for validation
* @param sideAStartVertex   Start vertex for defining side A
* @param sideAEndVertex     End vertex for defining side A
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static bool isLocalMinimalTriangle(cv::Point2f &vertexA, cv::Point2f &vertexB,
                                   cv::Point2f &vertexC, const std::vector<cv::Point2f> &polygon,
                                   unsigned int nrOfPoints, unsigned int a, unsigned int b,
                                   unsigned int validationFlag, const cv::Point2f &sideAStartVertex,
                                   const cv::Point2f &sideAEndVertex, const cv::Point2f &sideBStartVertex,
                                   const cv::Point2f &sideBEndVertex, const cv::Point2f &sideCStartVertex,
                                   const cv::Point2f &sideCEndVertex) {
    if ((!lineIntersection(sideAStartVertex, sideAEndVertex,
                           sideBStartVertex, sideBEndVertex, vertexC)) ||
        (!lineIntersection(sideAStartVertex, sideAEndVertex,
                           sideCStartVertex, sideCEndVertex, vertexB)) ||
        (!lineIntersection(sideBStartVertex, sideBEndVertex,
                           sideCStartVertex, sideCEndVertex, vertexA))) {
        return false;
    }

    return isValidMinimalTriangle(vertexA, vertexB, vertexC, polygon, nrOfPoints, a, b,
                                  validationFlag, sideAStartVertex, sideAEndVertex,
                                  sideBStartVertex, sideBEndVertex, sideCStartVertex,
                                  sideCEndVertex);
}

//! Check if the found minimal triangle is valid
/*!
* This means that all midpoints of the triangle should touch the polygon
*
* See paper [2] for more details
*
* @param vertexA            Vertex A of the enclosing triangle
* @param vertexB            Vertex B of the enclosing triangle
* @param vertexC            Vertex C of the enclosing triangle
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param b                  Index b
* @param validationFlag     Flag used for validation
* @param sideAStartVertex   Start vertex for defining side A
* @param sideAEndVertex     End vertex for defining side A
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static bool isValidMinimalTriangle(const cv::Point2f &vertexA, const cv::Point2f &vertexB,
                                   const cv::Point2f &vertexC, const std::vector<cv::Point2f> &polygon,
                                   unsigned int nrOfPoints, unsigned int a, unsigned int b,
                                   unsigned int validationFlag, const cv::Point2f &sideAStartVertex,
                                   const cv::Point2f &sideAEndVertex, const cv::Point2f &sideBStartVertex,
                                   const cv::Point2f &sideBEndVertex, const cv::Point2f &sideCStartVertex,
                                   const cv::Point2f &sideCEndVertex) {
    cv::Point2f midpointSideA = middlePoint(vertexB, vertexC);
    cv::Point2f midpointSideB = middlePoint(vertexA, vertexC);
    cv::Point2f midpointSideC = middlePoint(vertexA, vertexB);

    bool sideAValid = (validationFlag == VALIDATION_SIDE_A_TANGENT)
                        ? (areEqualPoints(midpointSideA, polygon[predecessor(a, nrOfPoints)]))
                        : (isPointOnLineSegment(midpointSideA, sideAStartVertex, sideAEndVertex));

    bool sideBValid = (validationFlag == VALIDATION_SIDE_B_TANGENT)
                          ? (areEqualPoints(midpointSideB, polygon[b]))
                          : (isPointOnLineSegment(midpointSideB, sideBStartVertex, sideBEndVertex));

    //bool sideCValid = isPointOnLineSegment(midpointSideC, sideCStartVertex, sideCEndVertex);
    bool sideCValid = (validationFlag == VALIDATION_SIDES_FLUSH) || isPointOnLineSegment(midpointSideC, sideCStartVertex, sideCEndVertex);

    return (sideAValid && sideBValid && sideCValid);
}

//! Update the current minimum area enclosing triangle if the newly obtained one has a smaller area
/*!
* @param triangle   Minimum area triangle enclosing the given polygon
* @param area       Area of the minimum area triangle enclosing the given polygon
* @param vertexA    Vertex A of the enclosing triangle
* @param vertexB    Vertex B of the enclosing triangle
* @param vertexC    Vertex C of the enclosing triangle
*/
static void updateMinimumAreaEnclosingTriangle(std::vector<cv::Point2f> &triangle, double &area,
                                               const cv::Point2f &vertexA, const cv::Point2f &vertexB,
                                               const cv::Point2f &vertexC) {
    double triangleArea = areaOfTriangle(vertexA, vertexB, vertexC);

    if (triangleArea < area) {
        triangle.clear();

        triangle.push_back(vertexA);
        triangle.push_back(vertexB);
        triangle.push_back(vertexC);

        area = triangleArea;
    }
}

//! Return the middle point of side B
/*!
* @param middlePointOfSideB Middle point of side B
* @param sideAStartVertex   Start vertex for defining side A
* @param sideAEndVertex     End vertex for defining side A
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static bool middlePointOfSideB(cv::Point2f &middlePointOfSideB, const cv::Point2f &sideAStartVertex,
                               const cv::Point2f &sideAEndVertex, const cv::Point2f &sideBStartVertex,
                               const cv::Point2f &sideBEndVertex, const cv::Point2f &sideCStartVertex,
                               const cv::Point2f &sideCEndVertex) {
    cv::Point2f vertexA, vertexC;

    if ((!lineIntersection(sideBStartVertex, sideBEndVertex, sideCStartVertex, sideCEndVertex, vertexA)) ||
        (!lineIntersection(sideBStartVertex, sideBEndVertex, sideAStartVertex, sideAEndVertex, vertexC))) {
        return false;
    }

    middlePointOfSideB = middlePoint(vertexA, vertexC);

    return true;
}

//! Check if the line intersects below
/*!
* Check if the line determined by gammaPoint and polygon[polygonPointIndex] intersects
* the polygon below the point polygon[polygonPointIndex]
*
* @param gammaPoint         Gamma(p)
* @param polygonPointIndex  Index of the polygon point which is considered when determining the line
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param c                  Index c
*/
static bool intersectsBelow(const cv::Point2f &gammaPoint, unsigned int polygonPointIndex,
                            const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                            unsigned int c) {
    double angleOfGammaAndPoint = angleOfLineWrtOxAxis(polygon[polygonPointIndex], gammaPoint);

    return (intersects(angleOfGammaAndPoint, polygonPointIndex, polygon, nrOfPoints, c) == INTERSECTS_BELOW);
}

//! Check if the line intersects above
/*!
* Check if the line determined by gammaPoint and polygon[polygonPointIndex] intersects
* the polygon above the point polygon[polygonPointIndex]
*
* @param gammaPoint         Gamma(p)
* @param polygonPointIndex  Index of the polygon point which is considered when determining the line
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param c                  Index c
*/
static bool intersectsAbove(const cv::Point2f &gammaPoint, unsigned int polygonPointIndex,
                            const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                            unsigned int c) {
    double angleOfGammaAndPoint = angleOfLineWrtOxAxis(gammaPoint, polygon[polygonPointIndex]);

    return (intersects(angleOfGammaAndPoint, polygonPointIndex, polygon, nrOfPoints, c) == INTERSECTS_ABOVE);
}

//! Check if/where the line determined by gammaPoint and polygon[polygonPointIndex] intersects the polygon
/*!
* @param angleGammaAndPoint     Angle determined by gammaPoint and polygon[polygonPointIndex] wrt Ox axis
* @param polygonPointIndex      Index of the polygon point which is considered when determining the line
* @param polygon                The polygon representing the convex hull of the points
* @param nrOfPoints             Number of points defining the convex polygon
* @param c                      Index c
*/
static unsigned int intersects(double angleGammaAndPoint, unsigned int polygonPointIndex,
                               const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                               unsigned int c) {
    double anglePointPredecessor = angleOfLineWrtOxAxis(polygon[predecessor(polygonPointIndex, nrOfPoints)],
                                                        polygon[polygonPointIndex]);
    double anglePointSuccessor   = angleOfLineWrtOxAxis(polygon[successor(polygonPointIndex, nrOfPoints)],
                                                        polygon[polygonPointIndex]);
    double angleFlushEdge        = angleOfLineWrtOxAxis(polygon[predecessor(c, nrOfPoints)],
                                                        polygon[c]);

    if (isFlushAngleBtwPredAndSucc(angleFlushEdge, anglePointPredecessor, anglePointSuccessor)) {
        if ((isGammaAngleBtw(angleGammaAndPoint, anglePointPredecessor, angleFlushEdge)) ||
            (almostEqual(angleGammaAndPoint, anglePointPredecessor))) {
            return intersectsAboveOrBelow(predecessor(polygonPointIndex, nrOfPoints),
                                          polygonPointIndex, polygon, nrOfPoints, c);
        } else if ((isGammaAngleBtw(angleGammaAndPoint, anglePointSuccessor, angleFlushEdge)) ||
                  (almostEqual(angleGammaAndPoint, anglePointSuccessor))) {
            return intersectsAboveOrBelow(successor(polygonPointIndex, nrOfPoints),
                                          polygonPointIndex, polygon, nrOfPoints, c);
        }
    } else {
        if (
            (isGammaAngleBtw(angleGammaAndPoint, anglePointPredecessor, anglePointSuccessor)) ||
            (
                (isGammaAngleEqualTo(angleGammaAndPoint, anglePointPredecessor)) &&
                (!isGammaAngleEqualTo(angleGammaAndPoint, angleFlushEdge))
            ) ||
            (
                (isGammaAngleEqualTo(angleGammaAndPoint, anglePointSuccessor)) &&
                (!isGammaAngleEqualTo(angleGammaAndPoint, angleFlushEdge))
            )
           ) {
            return INTERSECTS_BELOW;
        }
    }

    return INTERSECTS_CRITICAL;
}

//! If (gamma(x) x) intersects P between successorOrPredecessorIndex and pointIntex is it above/below?
/*!
* @param succPredIndex  Index of the successor or predecessor
* @param pointIndex     Index of the point x in the polygon
* @param polygon        The polygon representing the convex hull of the points
* @param nrOfPoints     Number of points defining the convex polygon
* @param c              Index c
*/
static unsigned int intersectsAboveOrBelow(unsigned int succPredIndex, unsigned int pointIndex,
                                           const std::vector<cv::Point2f> &polygon,
                                           unsigned int nrOfPoints, unsigned int c) {
    if (height(succPredIndex, polygon, nrOfPoints, c) > height(pointIndex, polygon, nrOfPoints, c)) {
        return INTERSECTS_ABOVE;
    } else {
        return INTERSECTS_BELOW;
    }
}

//! Find gamma for a given point "p" specified by its index
/*!
* The function returns true if gamma exists i.e. if lines (a a-1) and (x y) intersect
* and false otherwise. In case the two lines intersect in point intersectionPoint, gamma is computed.
*
* Considering that line (x y) is a line parallel to (c c-1) and that the distance between the lines is equal
* to 2 * height(p), we can have two possible (x y) lines.
*
* Therefore, we will compute two intersection points between the lines (x y) and (a a-1) and take the
* point which is on the same side of line (c c-1) as the polygon.
*
* See paper [2] and formula for distance from point to a line for more details
*
* @param polygonPointIndex Index of the polygon point
* @param gammaPoint        Point gamma(polygon[polygonPointIndex])
* @param polygon           The polygon representing the convex hull of the points
* @param nrOfPoints        Number of points defining the convex polygon
* @param a                 Index a
* @param c                 Index c
*/
static bool gamma(unsigned int polygonPointIndex, cv::Point2f &gammaPoint,
                  const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                  unsigned int a, unsigned int c) {
    cv::Point2f intersectionPoint1, intersectionPoint2;

    // Get intersection points if they exist
    if (!findGammaIntersectionPoints(polygon, nrOfPoints, c, polygonPointIndex,
                                     polygon[a], polygon[predecessor(a, nrOfPoints)],
                                     polygon[c], polygon[predecessor(c, nrOfPoints)],
                                     intersectionPoint1, intersectionPoint2)) {
        return false;
    }

    // Select the point which is on the same side of line C as the polygon
    if (areOnTheSameSideOfLine(intersectionPoint1, polygon[successor(c, nrOfPoints)],
                               polygon[c], polygon[predecessor(c, nrOfPoints)])) {
        gammaPoint = intersectionPoint1;
    } else {
        gammaPoint = intersectionPoint2;
    }

    return true;
}

//! Find vertex C which lies on side B at a distance = 2 * height(a-1) from side C
/*!
* Considering that line (x y) is a line parallel to (c c-1) and that the distance between the lines is equal
* to 2 * height(a-1), we can have two possible (x y) lines.
*
* Therefore, we will compute two intersection points between the lines (x y) and (b b-1) and take the
* point which is on the same side of line (c c-1) as the polygon.
*
* See paper [2] and formula for distance from point to a line for more details
*
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param a                  Index a
* @param c                  Index c
* @param sideBStartVertex   Start vertex for defining side B
* @param sideBEndVertex     End vertex for defining side B
* @param sideCStartVertex   Start vertex for defining side C
* @param sideCEndVertex     End vertex for defining side C
*/
static cv::Point2f findVertexCOnSideB(const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                                      unsigned int a, unsigned int c,
                                      const cv::Point2f &sideBStartVertex,
                                      const cv::Point2f &sideBEndVertex,
                                      const cv::Point2f &sideCStartVertex,
                                      const cv::Point2f &sideCEndVertex) {
    cv::Point2f intersectionPoint1, intersectionPoint2;

    // Get intersection points if they exist
    if (!findGammaIntersectionPoints(polygon, nrOfPoints, c, predecessor(a, nrOfPoints),
                                     sideBStartVertex, sideBEndVertex,
                                     sideCStartVertex, sideCEndVertex,
                                     intersectionPoint1, intersectionPoint2)) {
        CV_Error(cv::Error::StsInternal, ERR_VERTEX_C_ON_SIDE_B);
    }

    // Select the point which is on the same side of line C as the polygon
    if (areOnTheSameSideOfLine(intersectionPoint1, polygon[successor(c, nrOfPoints)],
                               polygon[c], polygon[predecessor(c, nrOfPoints)])) {
        return intersectionPoint1;
    } else {
        return intersectionPoint2;
    }
}

//! Find the intersection points to compute gamma(point)
/*!
* @param polygon                The polygon representing the convex hull of the points
* @param nrOfPoints             Number of points defining the convex polygon
* @param c                      Index c
* @param polygonPointIndex      Index of the polygon point for which the distance is known
* @param side1StartVertex       Start vertex for side 1
* @param side1EndVertex         End vertex for side 1
* @param side2StartVertex       Start vertex for side 2
* @param side2EndVertex         End vertex for side 2
* @param intersectionPoint1     First intersection point between one pair of lines
* @param intersectionPoint2     Second intersection point between other pair of lines
*/
static bool findGammaIntersectionPoints(const std::vector<cv::Point2f> &polygon, unsigned int nrOfPoints,
                                        unsigned int c, unsigned int polygonPointIndex,
                                        const cv::Point2f &side1StartVertex, const cv::Point2f &side1EndVertex,
                                        const cv::Point2f &side2StartVertex, const cv::Point2f &side2EndVertex,
                                        cv::Point2f &intersectionPoint1, cv::Point2f &intersectionPoint2) {
    std::vector<double> side1Params = lineEquationParameters(side1StartVertex, side1EndVertex);
    std::vector<double> side2Params = lineEquationParameters(side2StartVertex, side2EndVertex);

    // Compute side C extra parameter using the formula for distance from a point to a line
    double polygonPointHeight = height(polygonPointIndex, polygon, nrOfPoints, c);
    double distFormulaDenom = sqrt((side2Params[0] * side2Params[0]) + (side2Params[1] * side2Params[1]));
    double sideCExtraParam = 2 * polygonPointHeight * distFormulaDenom;

    // Get intersection points if they exist or if lines are identical
    if (!areIntersectingLines(side1Params, side2Params, sideCExtraParam, intersectionPoint1, intersectionPoint2)) {
        return false;
    } else if (areIdenticalLines(side1Params, side2Params, sideCExtraParam)) {
        intersectionPoint1 = side1StartVertex;
        intersectionPoint2 = side1EndVertex;
    }

    return true;
}

//! Check if the given lines are identical or not
/*!
* The lines are specified as:
*      ax + by + c = 0
*  OR
*      ax + by + c (+/-) sideCExtraParam = 0
*
* @param side1Params       Vector containing the values of a, b and c for side 1
* @param side2Params       Vector containing the values of a, b and c for side 2
* @param sideCExtraParam   Extra parameter for the flush edge C
*/
static bool areIdenticalLines(const std::vector<double> &side1Params,
                              const std::vector<double> &side2Params, double sideCExtraParam) {
    return (
        (areIdenticalLines(side1Params[0], side1Params[1], -(side1Params[2]),
                           side2Params[0], side2Params[1], -(side2Params[2]) - sideCExtraParam)) ||
        (areIdenticalLines(side1Params[0], side1Params[1], -(side1Params[2]),
                           side2Params[0], side2Params[1], -(side2Params[2]) + sideCExtraParam))
    );
}

//! Check if the given lines intersect or not. If the lines intersect find their intersection points.
/*!
* The lines are specified as:
*      ax + by + c = 0
*  OR
*      ax + by + c (+/-) sideCExtraParam = 0
*
* @param side1Params           Vector containing the values of a, b and c for side 1
* @param side2Params           Vector containing the values of a, b and c for side 2
* @param sideCExtraParam       Extra parameter for the flush edge C
* @param intersectionPoint1    The first intersection point, if it exists
* @param intersectionPoint2    The second intersection point, if it exists
*/
static bool areIntersectingLines(const std::vector<double> &side1Params,
                                 const std::vector<double> &side2Params,
                                 double sideCExtraParam, cv::Point2f &intersectionPoint1,
                                 cv::Point2f &intersectionPoint2) {
    return (
        (lineIntersection(side1Params[0], side1Params[1], -(side1Params[2]),
                          side2Params[0], side2Params[1], -(side2Params[2]) - sideCExtraParam,
                          intersectionPoint1)) &&
        (lineIntersection(side1Params[0], side1Params[1], -(side1Params[2]),
                          side2Params[0], side2Params[1], -(side2Params[2]) + sideCExtraParam,
                          intersectionPoint2))
    );
}

//! Get the line equation parameters "a", "b" and "c" for the line determined by points "p" and "q"
/*!
* The equation of the line is considered in the general form:
* ax + by + c = 0
*
* @param p One point for defining the equation of the line
* @param q Second point for defining the equation of the line
*/
static std::vector<double> lineEquationParameters(const cv::Point2f& p, const cv::Point2f &q) {
    std::vector<double> lineEquationParameters;
    double a, b, c;

    lineEquationDeterminedByPoints(p, q, a, b, c);

    lineEquationParameters.push_back(a);
    lineEquationParameters.push_back(b);
    lineEquationParameters.push_back(c);

    return lineEquationParameters;
}

//! Compute the height of the point
/*!
* See paper [2] for more details
*
* @param polygonPoint       Polygon point
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param c                  Index c
*/
static double height(const cv::Point2f &polygonPoint, const std::vector<cv::Point2f> &polygon,
                     unsigned int nrOfPoints, unsigned int c) {
    cv::Point2f pointC = polygon[c];
    cv::Point2f pointCPredecessor = polygon[predecessor(c, nrOfPoints)];

    return distanceFromPointToLine(polygonPoint, pointC, pointCPredecessor);
}

//! Compute the height of the point specified by the given index
/*!
* See paper [2] for more details
*
* @param polygonPointIndex  Index of the polygon point
* @param polygon            The polygon representing the convex hull of the points
* @param nrOfPoints         Number of points defining the convex polygon
* @param c                  Index c
*/
static double height(unsigned int polygonPointIndex, const std::vector<cv::Point2f> &polygon,
                     unsigned int nrOfPoints, unsigned int c) {
    cv::Point2f pointC = polygon[c];
    cv::Point2f pointCPredecessor = polygon[predecessor(c, nrOfPoints)];

    cv::Point2f polygonPoint = polygon[polygonPointIndex];

    return distanceFromPointToLine(polygonPoint, pointC, pointCPredecessor);
}

//! Advance the given index with one position
/*!
* @param index          Index of the point
* @param nrOfPoints     Number of points defining the convex polygon
*/
static void advance(unsigned int &index, unsigned int nrOfPoints) {
    index = successor(index, nrOfPoints);
}

//! Return the succesor of the provided point index
/*!
* The succesor of the last polygon point is the first polygon point
* (circular referencing)
*
* @param index          Index of the point
* @param nrOfPoints     Number of points defining the convex polygon
*/
static unsigned int successor(unsigned int index, unsigned int nrOfPoints) {
    return ((index + 1) % nrOfPoints);
}

//! Return the predecessor of the provided point index
/*!
* The predecessor of the first polygon point is the last polygon point
* (circular referencing)
*
* @param index          Index of the point
* @param nrOfPoints     Number of points defining the convex polygon
*/
static unsigned int predecessor(unsigned int index, unsigned int nrOfPoints) {
    return (index == 0) ? (nrOfPoints - 1)
                        : (index - 1);
}

//! Check if the flush edge angle/opposite angle lie between the predecessor and successor angle
/*!
* Check if the angle of the flush edge or its opposite angle lie between the angle of
* the predecessor and successor
*
* @param angleFlushEdge    Angle of the flush edge
* @param anglePred         Angle of the predecessor
* @param angleSucc         Angle of the successor
*/
static bool isFlushAngleBtwPredAndSucc(double &angleFlushEdge, double anglePred, double angleSucc) {
    if (isAngleBetweenNonReflex(angleFlushEdge, anglePred, angleSucc)) {
        return true;
    } else if (isOppositeAngleBetweenNonReflex(angleFlushEdge, anglePred, angleSucc)) {
        angleFlushEdge = oppositeAngle(angleFlushEdge);

        return true;
    }

    return false;
}

//! Check if the angle of the line (gamma(p) p) or its opposite angle is equal to the given angle
/*!
* @param gammaAngle    Angle of the line (gamma(p) p)
* @param angle         Angle to compare against
*/
static bool isGammaAngleEqualTo(double &gammaAngle, double angle) {
    return (almostEqual(gammaAngle, angle));
}

//! Check if the angle of the line (gamma(p) p) or its opposite angle lie between angle1 and angle2
/*!
* @param gammaAngle    Angle of the line (gamma(p) p)
* @param angle1        One of the boundary angles
* @param angle2        Another boundary angle
*/
static bool isGammaAngleBtw(double &gammaAngle, double angle1, double angle2) {
    return (isAngleBetweenNonReflex(gammaAngle, angle1, angle2));
}

//! Get the angle of the line measured from the Ox axis in counterclockwise direction
/*!
* The line is specified by points "a" and "b". The value of the angle is expressed in degrees.
*
* @param a Point a
* @param b Point b
*/
static double angleOfLineWrtOxAxis(const cv::Point2f &a, const cv::Point2f &b) {
    double y = b.y - a.y;
    double x = b.x - a.x;

    double angle = (std::atan2(y, x) * 180 / CV_PI);

    return (angle < 0) ? (angle + 360)
                       : angle;
}

//! Check if angle1 lies between non reflex angle determined by angles 2 and 3
/*!
* @param angle1 The angle which lies between angle2 and angle3 or not
* @param angle2 One of the boundary angles
* @param angle3 The other boundary angle
*/
static bool isAngleBetweenNonReflex(double angle1, double angle2, double angle3) {
    if (std::abs(angle2 - angle3) > 180) {
        if (angle2 > angle3) {
            return (((angle2 < angle1) && (lessOrEqual(angle1, 360))) ||
                    ((lessOrEqual(0, angle1)) && (angle1 < angle3)));
        } else {
            return (((angle3 < angle1) && (lessOrEqual(angle1, 360))) ||
                    ((lessOrEqual(0, angle1)) && (angle1 < angle2)));
        }
    } else {
        return isAngleBetween(angle1, angle2, angle3);
    }
}

//! Check if the opposite of angle1, ((angle1 + 180) % 360), lies between non reflex angle determined by angles 2 and 3
/*!
* @param angle1 The angle which lies between angle2 and angle3 or not
* @param angle2 One of the boundary angles
* @param angle3 The other boundary angle
*/
static bool isOppositeAngleBetweenNonReflex(double angle1, double angle2, double angle3) {
    double angle1Opposite = oppositeAngle(angle1);

    return (isAngleBetweenNonReflex(angle1Opposite, angle2, angle3));
}

//! Check if angle1 lies between angles 2 and 3
/*!
* @param angle1 The angle which lies between angle2 and angle3 or not
* @param angle2 One of the boundary angles
* @param angle3 The other boundary angle
*/
static bool isAngleBetween(double angle1, double angle2, double angle3) {
    if ((((int)(angle2 - angle3)) % 180) > 0) {
        return ((angle3 < angle1) && (angle1 < angle2));
    } else {
        return ((angle2 < angle1) && (angle1 < angle3));
    }
}

//! Return the angle opposite to the given angle
/*!
* if (angle < 180) then
*      return (angle + 180);
* else
*      return (angle - 180);
* endif
*
* @param angle Angle
*/
static double oppositeAngle(double angle) {
    return (angle > 180) ? (angle - 180)
                         : (angle + 180);
}

//! Compute the distance from a point "a" to a line specified by two points "B" and "C"
/*!
* Formula used:
*
*     |(x_c - x_b) * (y_b - y_a) - (x_b - x_a) * (y_c - y_b)|
* d = -------------------------------------------------------
*            sqrt(((x_c - x_b)^2) + ((y_c - y_b)^2))
*
* Reference: http://mathworld.wolfram.com/Point-LineDistance2-Dimensional.html
* (Last access: 15.09.2013)
*
* @param a             Point from which the distance is measures
* @param linePointB    One of the points determining the line
* @param linePointC    One of the points determining the line
*/
static double distanceFromPointToLine(const cv::Point2f &a, const cv::Point2f &linePointB,
                                      const cv::Point2f &linePointC) {
    double term1 = linePointC.x - linePointB.x;
    double term2 = linePointB.y - a.y;
    double term3 = linePointB.x - a.x;
    double term4 = linePointC.y - linePointB.y;

    double nominator = std::abs((term1 * term2) - (term3 * term4));
    double denominator = std::sqrt((term1 * term1) + (term4 * term4));

    return (denominator != 0) ? (nominator / denominator)
                              : 0;
}

//! Compute the distance between two points
/*! Compute the Euclidean distance between two points
*
* @param a Point a
* @param b Point b
*/
static double distanceBtwPoints(const cv::Point2f &a, const cv::Point2f &b) {
    double xDiff = a.x - b.x;
    double yDiff = a.y - b.y;

    return std::sqrt((xDiff * xDiff) + (yDiff * yDiff));
}

//! Compute the area of a triangle defined by three points
/*!
* The area is computed using the determinant method.
* An example is depicted at http://demonstrations.wolfram.com/TheAreaOfATriangleUsingADeterminant/
* (Last access: 15.09.2013)
*
* @param a Point a
* @param b Point b
* @param c Point c
*/
static double areaOfTriangle(const cv::Point2f &a, const cv::Point2f &b, const cv::Point2f &c) {
    double posTerm = (a.x * b.y) + (a.y * c.x) + (b.x * c.y);
    double negTerm = (b.y * c.x) + (a.x * c.y) + (a.y * b.x);

    double determinant = posTerm - negTerm;

    return std::abs(determinant) / 2;
}

//! Get the point in the middle of the segment determined by points "a" and "b"
/*!
* @param a Point a
* @param b Point b
*/
static cv::Point2f middlePoint(const cv::Point2f &a, const cv::Point2f &b) {
    double middleX = static_cast<double>((a.x + b.x) / 2);
    double middleY = static_cast<double>((a.y + b.y) / 2);

    return cv::Point2f(static_cast<float>(middleX), static_cast<float>(middleY));
}

//! Determine the intersection point of two lines, if this point exists
/*! Two lines intersect if they are not parallel (Parallel lines intersect at
* +/- infinity, but we do not consider this case here).
*
* The lines are specified in the following form:
*      A1x + B1x = C1
*      A2x + B2x = C2
*
* If det (= A1*B2 - A2*B1) == 0, then lines are parallel
*                                else they intersect
*
* If they intersect, then let us denote the intersection point with P(x, y) where:
*      x = (C1*B2 - C2*B1) / (det)
*      y = (C2*A1 - C1*A2) / (det)
*
* @param a1 A1
* @param b1 B1
* @param c1 C1
* @param a2 A2
* @param b2 B2
* @param c2 C2
* @param intersection The intersection point, if this point exists
*/
static bool lineIntersection(double a1, double b1, double c1, double a2, double b2, double c2,
                             cv::Point2f &intersection) {
    double det = (a1 * b2) - (a2 * b1);

    if (!(almostEqual(det, 0))) {
        intersection.x = static_cast<float>(((c1 * b2) - (c2 * b1)) / (det));
        intersection.y = static_cast<float>(((c2 * a1) - (c1 * a2)) / (det));

        return true;
    }

    return false;
}

//! Determine the intersection point of two lines, if this point exists
/*! Two lines intersect if they are not parallel (Parallel lines intersect at
* +/- infinity, but we do not consider this case here).
*
* The lines are specified by a pair of points each. If they intersect, then
* the function returns true, else it returns false.
*
* Lines can be specified in the following form:
*      A1x + B1x = C1
*      A2x + B2x = C2
*
* If det (= A1*B2 - A2*B1) == 0, then lines are parallel
*                                else they intersect
*
* If they intersect, then let us denote the intersection point with P(x, y) where:
*      x = (C1*B2 - C2*B1) / (det)
*      y = (C2*A1 - C1*A2) / (det)
*
* @param a1 First point for determining the first line
* @param b1 Second point for determining the first line
* @param a2 First point for determining the second line
* @param b2 Second point for determining the second line
* @param intersection The intersection point, if this point exists
*/
static bool lineIntersection(const cv::Point2f &a1, const cv::Point2f &b1, const cv::Point2f &a2,
                             const cv::Point2f &b2, cv::Point2f &intersection) {
    double A1 = b1.y - a1.y;
    double B1 = a1.x - b1.x;
    double C1 = (a1.x * A1) + (a1.y * B1);

    double A2 = b2.y - a2.y;
    double B2 = a2.x - b2.x;
    double C2 = (a2.x * A2) + (a2.y * B2);

    double det = (A1 * B2) - (A2 * B1);

    if (!almostEqual(det, 0)) {
        intersection.x = static_cast<float>(((C1 * B2) - (C2 * B1)) / (det));
        intersection.y = static_cast<float>(((C2 * A1) - (C1 * A2)) / (det));

        return true;
    }

    return false;
}

//! Get the values of "a", "b" and "c" of the line equation ax + by + c = 0 knowing that point "p" and "q" are on the line
/*!
* a = q.y - p.y
* b = p.x - q.x
* c = - (p.x * a) - (p.y * b)
*
* @param p Point p
* @param q Point q
* @param a Parameter "a" from the line equation
* @param b Parameter "b" from the line equation
* @param c Parameter "c" from the line equation
*/
static void lineEquationDeterminedByPoints(const cv::Point2f &p, const cv::Point2f &q,
                                           double &a, double &b, double &c) {
    CV_Assert(areEqualPoints(p, q) == false);

    a = q.y - p.y;
    b = p.x - q.x;
    c = ((-p.y) * b) - (p.x * a);
}

//! Check if p1 and p2 are on the same side of the line determined by points a and b
/*!
* @param p1    Point p1
* @param p2    Point p2
* @param a     First point for determining line
* @param b     Second point for determining line
*/
static bool areOnTheSameSideOfLine(const cv::Point2f &p1, const cv::Point2f &p2,
                                   const cv::Point2f &a, const cv::Point2f &b) {
    double a1, b1, c1;

    lineEquationDeterminedByPoints(a, b, a1, b1, c1);

    double p1OnLine = (a1 * p1.x) + (b1 * p1.y) + c1;
    double p2OnLine = (a1 * p2.x) + (b1 * p2.y) + c1;

    return (sign(p1OnLine) == sign(p2OnLine));
}

//! Check if one point lies between two other points
/*!
* @param point             Point lying possibly outside the line segment
* @param lineSegmentStart  First point determining the line segment
* @param lineSegmentEnd    Second point determining the line segment
*/
static bool isPointOnLineSegment(const cv::Point2f &point, const cv::Point2f &lineSegmentStart,
                                 const cv::Point2f &lineSegmentEnd) {
    double d1 = distanceBtwPoints(point, lineSegmentStart);
    double d2 = distanceBtwPoints(point, lineSegmentEnd);
    double lineSegmentLength = distanceBtwPoints(lineSegmentStart, lineSegmentEnd);

    return (almostEqual(d1 + d2, lineSegmentLength));
}

//! Check if two lines are identical
/*!
* Lines are be specified in the following form:
*      A1x + B1x = C1
*      A2x + B2x = C2
*
* If (A1/A2) == (B1/B2) == (C1/C2), then the lines are identical
*                                   else they are not
*
* @param a1 A1
* @param b1 B1
* @param c1 C1
* @param a2 A2
* @param b2 B2
* @param c2 C2
*/
static bool areIdenticalLines(double a1, double b1, double c1, double a2, double b2, double c2) {
    double a1B2 = a1 * b2;
    double a2B1 = a2 * b1;
    double a1C2 = a1 * c2;
    double a2C1 = a2 * c1;
    double b1C2 = b1 * c2;
    double b2C1 = b2 * c1;

    return ((almostEqual(a1B2, a2B1)) && (almostEqual(b1C2, b2C1)) && (almostEqual(a1C2, a2C1)));
}

//! Check if points point1 and point2 are equal or not
/*!
* @param point1 One point
* @param point2 The other point
*/
static bool areEqualPoints(const cv::Point2f &point1, const cv::Point2f &point2) {
    return (almostEqual(point1.x, point2.x) && almostEqual(point1.y, point2.y));
}

//! Return the sign of the number
/*!
* The sign function returns:
*  -1, if number < 0
*  +1, if number > 0
*  0, otherwise
*/
static int sign(double number) {
    return (number > 0) ? 1 : ((number < 0) ? -1 : 0);
}

//! Return the maximum of the provided numbers
static double maximum(double number1, double number2, double number3) {
    return std::max(std::max(number1, number2), number3);
}

//! Check if the two numbers are equal (almost)
/*!
* The expression for determining if two real numbers are equal is:
* if (Abs(x - y) <= EPSILON * Max(1.0f, Abs(x), Abs(y))).
*
* @param number1 First number
* @param number2 Second number
*/
static bool almostEqual(double number1, double number2) {
    return (std::abs(number1 - number2) <= (EPSILON * maximum(1.0, std::abs(number1), std::abs(number2))));
}

//! Check if the first number is greater than or equal to the second number
/*!
* @param number1 First number
* @param number2 Second number
*/
static bool greaterOrEqual(double number1, double number2) {
    return ((number1 > number2) || (almostEqual(number1, number2)));
}

//! Check if the first number is less than or equal to the second number
/*!
* @param number1 First number
* @param number2 Second number
*/
static bool lessOrEqual(double number1, double number2) {
    return ((number1 < number2) || (almostEqual(number1, number2)));
}

}

using namespace cv;
using namespace std;

int main( int argc, char* argv[] )
{
    Mat img(500, 500, CV_8UC3);

    RNG& rng = theRNG();
    vector<vector<Point> > testpoints;
    vector<Point> points;

    int pointarray[] =
    {
        127,124,127,140,138,217,149,240,153,241,156,122,0,0,
        127,197,168,369,326,358,353,331,364,150,298,139,131,147,0,0,
        127,242,187,242,364,186,266,42,186,39,178,40,150,49,0,0,
        139,331,237,369,360,358,320,150,316,147,264,139,164,197,0,0,
        140,42,180,242,350,242,322,186,231,49,186,39,0,0,
        143,331,153,369,175,358,354,150,339,147,253,139,0,0,
        144,138,152,178,169,203,179,110,149,113,0,0,
        145,358,312,369,317,150,299,147,225,139,167,197,0,0,
        145,42,188,242,209,242,292,186,223,58,216,49,186,39,0,0,
        148,255,188,253,189,239,185,157,168,135,0,0,
        150,42,163,186,273,242,303,242,350,186,260,49,186,39,0,0,
        167,178,169,186,184,242,199,242,214,186,223,58,223,49,213,42,186,39,178,40,0,0,
        167,180,184,285,214,363,223,369,223,58,186,39,178,40,0,0,
        167,224,184,368,214,262,223,168,223,58,186,39,178,40,169,136,0,0,
        169,173,189,219,214,210,219,188,210,141,0,0,
        173,186,223,242,321,242,336,49,253,42,186,39,178,40,0,0,
        175,81,175,116,227,136,222,75,183,78,0,0,
        178,40,245,242,313,242,313,186,229,42,186,39,0,0,
        195,369,374,358,291,150,202,139,0,0,
        242,358,323,369,346,331,373,139,262,147,0,0,
        88,113,88,115,100,193,111,204,109,127,93,114,0,0,

        100,160,102,168,105,166,114,153,111,98,104,133,0,0,
        104,160,113,181,114,183,132,126,107,96,0,0,
        109,143,125,168,132,171,128,135,116,116,0,0,
        109,203,114,224,124,155,113,120,0,0,
        110,128,112,165,127,176,136,155,134,109,0,0,
        110,150,123,224,129,215,134,178,139,117,113,121,0,0,
        118,174,150,193,150,147,134,102,121,124,0,0,
        121,198,126,226,134,213,137,191,141,117,127,142,0,0,
        122,181,122,207,130,213,153,174,139,123,123,152,0,0,
        123,133,128,176,142,168,147,115,142,100,129,108,0,0,
        125,130,137,173,151,222,162,212,164,205,165,165,133,126,0,0,
        126,173,149,182,153,146,153,139,152,128,140,118,127,155,0,0,
        127,131,129,203,147,232,149,204,148,190,138,133,0,0,
        127,171,127,332,146,360,371,330,371,205,0,0,
        128,232,206,318,217,320,336,337,364,267,374,173,357,155,251,127,154,136,138,193,0,0,
        130,366,368,366,362,322,318,250,281,190,260,179,152,152,141,190,0,0,
        132,190,140,231,165,236,174,129,150,127,133,135,0,0,
        133,235,159,223,151,166,141,134,0,0,
        134,108,137,153,145,187,155,179,163,136,154,127,0,0,
        135,147,184,324,350,333,372,249,276,156,190,144,0,0,
        135,186,137,197,142,170,150,121,139,137,138,143,0,0,
        137,177,143,199,155,139,151,128,142,107,141,106,0,0,
        139,59,145,87,154,111,166,72,149,62,0,0,
        142,164,143,190,146,232,153,230,166,201,183,129,150,135,0,0,
        143,247,176,232,179,218,178,179,175,128,158,171,0,0,
        144,167,146,229,162,248,179,156,176,142,152,140,0,0,
        144,207,148,210,178,158,182,137,176,113,156,169,0,0,
        145,181,158,202,160,198,161,122,153,150,0,0,
        145,193,152,226,161,250,172,181,174,147,148,134,0,0,
        145,84,147,94,178,119,182,88,174,84,147,77,0,0,
        149,123,150,172,151,200,158,216,193,170,151,123,0,0,
        149,150,150,193,182,122,164,114,0,0,
        152,129,161,166,173,200,197,161,193,136,163,130,0,0,
        155,167,158,191,160,191,168,179,172,154,170,110,156,134,0,0,
        156,254,176,247,195,195,201,139,184,136,173,164,0,0,
        158,85,174,98,195,107,187,67,170,70,164,73,0,0,
        163,106,163,160,165,188,183,210,209,190,0,0,
        165,154,177,184,211,227,212,146,170,118,0,0,
        165,258,209,264,219,178,200,149,167,214,0,0,
        166,138,167,191,189,244,193,218,201,142,0,0,
        170,137,171,224,222,202,225,147,215,124,209,116,0,0,
        174,167,175,211,185,261,195,270,220,229,218,216,182,169,0,0,
        174,171,176,214,203,194,223,133,224,120,178,126,0,0,
        174,202,198,254,224,268,221,225,209,170,175,187,0,0,
        175,203,196,226,215,159,216,117,200,126,0,0,
        177,173,180,224,205,156,194,116,181,137,0,0,
        178,187,198,198,228,129,215,121,185,114,0,0,
        183,191,215,205,214,164,199,117,190,155,185,180,0,0,
        184,148,203,231,236,373,321,342,351,316,358,273,273,166,192,136,0,0,
        193,225,211,232,225,226,217,153,203,118,0,0,
        203,295,225,342,323,296,296,143,247,166,223,206,0,0,
        241,133,249,179,286,144,290,126,255,102,0,0,
        243,174,281,134,271,92,256,134,0,0,
        251,97,273,172,320,164,294,129,0,0,
        31,88,35,131,40,138,40,128,39,105,38,88,32,80,0,0,
        49,119,55,175,56,184,60,178,57,148,50,104,0,0,
        58,115,58,148,60,148,70,122,66,98,60,82,0,0,
        66,97,77,154,78,159,81,153,80,132,79,115,73,88,0,0,
        68,131,68,152,73,190,78,180,78,143,69,108,0,0,
        73,128,74,159,78,195,84,201,84,185,82,118,75,108,0,0,
        78,95,79,130,89,165,97,160,98,140,80,95,0,0,
        81,108,83,127,89,166,96,189,95,157,87,128,0,0,
        85,174,93,165,101,138,106,113,95,117,0,0,
        96,191,110,209,107,117,101,139,0,0,
        98,175,105,179,118,179,126,169,128,118,99,139,0,0,
        99,121,110,168,116,166,116,164,110,103,0,0,
        130,191,154,337,301,329,362,315,350,242,321,156,293,127,194,165,0,0
    };

    for( int i = 0; i < 1219; i+=2 )
    {
        int x=pointarray[i];
        int y=pointarray[i+1];

        if( x+y > 0 )
            points.push_back(Point(x,y));
        else
        {
            testpoints.push_back(points);
            points.clear();
        }
    }

    int i = -1;

    for(;;)
    {
        if(i < 86)
            i++;
        else
        {
            // Generate a random set of points
            testpoints[i].clear();
            int count = rng.uniform(4, 30);
            for( int j = 0; j < count; j++ )
            {
                Point pt;
                pt.x = rng.uniform(img.cols/4, img.cols*3/4);
                pt.y = rng.uniform(img.rows/4, img.rows*3/4);

                testpoints[i].push_back(pt);
            }
        }

        vector<Point2f> triangle1;
        vector<Point2f> triangle2;

        double area1,area2;

        double triangle1point = 0;
        double triangle2point = 0;

        img = Scalar::all(0);

        area1 = cv::minEnclosingTriangle(testpoints[i], triangle1);


        if( triangle1.size() < 3 )
        {
            cout << "cv::minEnclosingTriangle FAILED!! \n";
        }
        else
            // Draw the triangle
            for( int k = 0; k < 3; k++ )
            {
                triangle1point+=triangle1[k].x+triangle1[k].y;
                line(img, triangle1[k], triangle1[(k+1)%3], Scalar(0, 255, 0), 3, LINE_AA);
            }

        std::vector<cv::Point2f> polygon;

        minEnclosingTriangle::createConvexHull(testpoints[i], polygon);

        minEnclosingTriangle::findMinEnclosingTriangle(polygon, triangle2, area2);

        if( triangle2.size() < 3 )
        {
            cout << "minEnclosingTriangle::findMinEnclosingTriangle FAILED!! \n";
        }
        else
            // Draw the triangle
            for( int k = 0; k < 3; k++ )
            {
                triangle2point+=triangle2[k].x+triangle2[k].y;
                line(img, triangle2[k], triangle2[(k+1)%3], Scalar(255, 0, 0), 1, LINE_AA);
            }

        // Draw the points
        for( int j = 0; j < testpoints[i].size(); j++ )
            circle( img, testpoints[i][j], 2, Scalar(0, 0, 255), FILLED, LINE_AA );

        if( abs( triangle1point - triangle2point)> 0.01)
        {
            cout << "area1 : " << area1 << "  - computed with current minEnclosingTriangle function\n";
            cout << "area2 : " << area2 << "  - computed with fixed minEnclosingTriangle function \n";
            cout << triangle1 << "\n";
            cout << triangle2 << "\n";
            cout << polygon << "\n";
            imshow( "minEnclosingTriangle test - OpenCV", img );
            waitKey(3000);
        }

        imshow( "minEnclosingTriangle test - OpenCV", img );
        char key = (char)waitKey(1);
        if( key == 27 || key == 'q' || key == 'Q' ) // 'ESC'
            break;
    }

    return 0;
}
