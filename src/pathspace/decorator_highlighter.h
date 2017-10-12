#pragma once
#include "pathspace/decorator.h"

class PathSpaceDecoratorHighlighter: public PathSpaceDecorator{
  public:
    PathSpaceDecoratorHighlighter(PathSpace* space_);
    void DrawGL(GUIState& state);
};

