#pragma once

#include "planner/cspace.h"
#include "gui_state.h"
#include "elements/swept_volume.h"
#include <ompl/base/PlannerData.h>
#include <KrisLibrary/robotics/RobotKinematics3D.h> //Config

class PathSpace{
  public:

    PathSpace(RobotWorld *world_, PlannerInput& input_);

    std::vector<Config> GetShortestPath();
    std::vector<Config> GetVertices();
    void SetShortestPath(std::vector<Config>);
    void SetVertices(std::vector<Config>);

    //split the pathspace up into smaller pieces.
    //Note: this decomposition does not need to be a partition, but could also
    //be a covering.
    virtual std::vector<PathSpace*> Decompose() = 0;
    virtual void DrawGL(const GUIState&) = 0;
    virtual bool isAtomic() = 0;

  protected:

    //ob::PlannerDataPtr pd; //contains local copy of PD

    //CSpaceOMPL *cspace; 

    std::vector<Config> vantage_path;
    std::vector<Config> vertices;

    RobotWorld *world;
    PlannerInput input;

    SweptVolume& GetSweptVolume(Robot *robot);
    SweptVolume *sv;
};

