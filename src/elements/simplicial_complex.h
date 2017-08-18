#pragma once
#include <Library/KrisLibrary/math/vector.h>
#include <Library/KrisLibrary/math3d/primitives.h>

//Simplicialcomplex consists of
//  V vertices
//  E edges
//  F faces
//  T tetrahedras
//
struct SimplicialComplex{
  std::vector<Math3D::Vector3> V;
  std::vector<std::pair<Math3D::Vector3,Math3D::Vector3>> E;
  std::vector<std::vector<Math3D::Vector3> > F;
  std::vector<std::vector<Math3D::Vector3> > T;
};
