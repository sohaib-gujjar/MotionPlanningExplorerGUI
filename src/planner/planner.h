#pragma once

#include "planner/planner_input.h"
//#include "planner/planner_output.h"
#include "elements/hierarchy.h"
#include "pathspace/pathspace.h"
#include "gui_state.h"
#include "ViewHierarchy.h"

#include <KrisLibrary/robotics/RobotKinematics3D.h> //Config
#include <Modeling/World.h> //RobotWorld
#include <Planning/RobotCSpace.h> //SingleRobotCSpace
#include <tinyxml.h>
#include <vector>
#include <memory>

class MotionPlanner{

  public:

    explicit MotionPlanner(RobotWorld *world_, PlannerInput& input_);

    const PlannerInput& GetInput();

    //folder-like operations on hierarchical path space
    virtual void Expand();
    virtual void Collapse();
    virtual void Next();
    virtual void Previous();
    
    virtual void DrawGL(const GUIState&);
    virtual void DrawGLScreen(double x_ =0.0, double y_=0.0);

    //planner will only be active if input exists and contains a valid algorithm
    bool isActive();
    void Print();

  protected:
    //virtual bool solve(std::vector<int> path_idxs);
    //virtual std::vector< std::vector<Config> > GetSiblingPaths();
    void RaiseError();

    uint current_level;
    uint current_level_node;
    std::vector<int> current_path;

    //PathSpaceHierarchyPtr hierarchy;
    Hierarchy<PathSpace*> *hierarchy;
    void UpdateHierarchy();
    bool isHierarchical();
    ViewHierarchy viewHierarchy;

    bool active;

    void CreateShallowHierarchy();
    void CreateSinglePathHierarchy();

    RobotWorld *world;
    PlannerInput input;
    //PlannerOutput output;

};

