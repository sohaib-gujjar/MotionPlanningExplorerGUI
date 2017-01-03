#include "iksolver.h"

IKSolver::IKSolver(RobotWorld *world):
  _world(world)
{
  this->_tolerance = 1e-2;
  this->_isSolved = false;
  this->_iters = 100;
  this->_verbose = 1;
}

void IKSolver::preSolve()
{
  this->_robot = _world->GetRobot(this->GetRobotName());
  this->_problem = this->GetProblem();
  this->q_initial = this->_robot->q;
  std::cout << "presolve" << std::endl;
}
void IKSolver::postSolve()
{
  this->q_solution = this->_robot->q;
  double d = (q_initial-q_solution).norm();
  std::cout << "DISTANCE IK" << d << std::endl;
  if(!_isSolved){
    std::cout << "No IK solution" << std::endl;
  }else{
    std::cout << "IK solution iters " << _iters << std::endl;
    std::cout << _robot->q << std::endl;
  }
}

bool IKSolver::solveIKconstraints()
{
  _isSolved = SolveIK(*_robot,_problem,_tolerance,_iters,_verbose);
  return _isSolved;
}
bool IKSolver::solve_default(){
  this->preSolve();
  _isSolved = SolveIK(*_robot,_problem,_tolerance,_iters,_verbose);
  this->postSolve();
  return _isSolved;
}

bool IKSolver::solve(){
  this->preSolve();
  _isSolved = this->solveIKconstraints();
  this->postSolve();
  return _isSolved;
}

///Set IK solution to real robot
void IKSolver::SetConfigSimulatedRobot(WorldSimulation &sim)
{
  if(!_isSolved){
    std::cout << "[ERROR] Robot cannot set to infeasible IK solution" << std::endl;
    return;
  }
  ODERobot *simrobot = sim.odesim.robot(0);
  simrobot->SetConfig(q_solution);
  std::cout << "[WARNING] Setting Simulated Robot to Config. This should be done exactly once!" << std::endl;
}

void IKSolver::visualize()
{
  //this->_world
  this->_problem = this->GetProblem();
  for(size_t i=0;i<this->_problem.size();i++) {
    IKGoal ikgoal = this->_problem[i];

    vector<ViewRobot> viewRobots = _world->robotViews;
    ViewRobot *viewRobot = &viewRobots[0];

    ViewIKGoal viewik = ViewIKGoal();

    //viewik.linkColor.set(1.0,0,0,0.8);
    //viewik.linkColor.setRandom();
    //viewik.lineColor.set(1,0,0);
    //GLDraw::GLColor lineColor;
    //GLDraw::GLColor linkColor;

    GLColor contactColor(0.5,0.8,0.5);

    //doesn't work because renderworld overwrites it!?
    viewRobot->SetColor(ikgoal.link,contactColor);

    viewik.DrawLink(ikgoal, *viewRobot);
    //viewik.Draw(ikgoal, *this->_robot);  
  }
}
IKGoal IKSolver::LinkToGoalTrans( const char *linkName, double x, double y, double z)
{
  std::cout << "linktogoal " << _robot->name << std::endl;
  int linkid = _robot->LinkIndex(linkName);
  std::cout << "linkid " <<linkid << std::endl;
  Vector3 localPosition(0,0,0);
  Vector3 position(x,y,z);

  IKGoal goal;
  goal.link = linkid;
  goal.localPosition = localPosition;
  goal.SetFixedPosition(position);
  return goal;
}
IKGoal IKSolver::LinkToGoalTransRot( const char *linkName, double x, double y, double z, Matrix3 &rotation)
{
  int linkid = _robot->LinkIndex(linkName);
  Vector3 localPosition(0,0,0);
  Vector3 position(x,y,z);

  IKGoal goal;
  goal.link = linkid;
  goal.localPosition = localPosition;
  goal.SetFixedPosition(position);
  goal.SetFixedRotation(rotation);
  return goal;
}
IKGoal IKSolver::LinkToGoalRot( const char *linkName, Matrix3 &rotation)
{
  int linkid = _robot->LinkIndex(linkName);
  Vector3 localPosition(0,0,0);

  IKGoal goal;
  goal.link = linkid;
  goal.localPosition = localPosition;
  goal.SetFixedRotation(rotation);
  return goal;
}
