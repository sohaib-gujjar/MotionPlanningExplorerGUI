#include "pathspace_ompl.h"
#include "pathspace_atomic.h"

#include "planner/cspace_factory.h"
#include "planner/strategy_geometric.h"
#include "drawMotionPlanner.h"

PathSpaceOMPL::PathSpaceOMPL(RobotWorld *world_, PlannerInput& input_):
  PathSpace(world_, input_)
{
}

bool PathSpaceOMPL::isAtomic() const{
  return false;
}
std::vector<PathSpace*> PathSpaceOMPL::Decompose(){
  WorldPlannerSettings worldsettings;
  worldsettings.InitializeDefault(*world);

  CSpaceFactory factory(input);
  CSpaceOMPL* cspace;
  SingleRobotCSpace* kcspace;

  int robot_idx = input.robot_idx;

  Robot *robot = world->robots[robot_idx];

  kcspace = new SingleRobotCSpace(*world,robot_idx,&worldsettings);

  cspace = factory.MakeGeometricCSpace(robot, kcspace);

  cspace->print();

  StrategyGeometric strategy;
  strategy.DisableOnetopicReduction();

  PlannerOutput output;
  strategy.plan(input, cspace, output);

  std::vector<PathSpace*> decomposedspace;

  if(output.success){
    decomposedspace.push_back( new PathSpaceAtomic(world, input) );
    decomposedspace.at(0)->SetShortestPath( output.GetKeyframes()  );
  }else{
    std::cout << "Error: Path could not be expanded" << std::endl;
  }
  return decomposedspace;

}
void PathSpaceOMPL::DrawGL(const GUIState& state){
  uint ridx = input.robot_idx;
  Robot* robot = world->robots[ridx];
  const Config qi = input.q_init;
  const Config qg = input.q_goal;

  GLColor lightGrey(0.4,0.4,0.4,0.2);
  GLColor lightGreen(0.2,0.9,0.2,0.2);
  GLColor lightRed(0.9,0.2,0.2,0.2);
  GLDraw::drawRobotAtConfig(robot, qi, lightGreen);
  GLDraw::drawRobotAtConfig(robot, qg, lightRed);

  std::vector<Config> init_path; 
  init_path.push_back(qi);
  init_path.push_back(qg);
  GLDraw::drawPath(init_path, lightGreen, 20);
}