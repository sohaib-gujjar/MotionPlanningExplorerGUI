#include "planner_ompl.h"
#include "cspace_sentinel.h"

GeometricCSpaceOMPL::GeometricCSpaceOMPL(Robot *robot)
{
  //###########################################################################
  // Create OMPL state space
  //   Create an SE(3) x R^n state space
  //###########################################################################
  if(!(robot->joints[0].type==RobotJoint::Floating))
  {
    std::cout << "[MotionPlanner] only supports robots with a configuration space equal to SE(3) x R^n" << std::endl;
    exit(0);
  }

  double N = robot->q.size()-6;
  std::cout << "[MotionPlanner] Robot CSpace: SE(3) x R^"<<N<<std::endl;

  ob::StateSpacePtr SE3(std::make_shared<ob::SE3StateSpace>());
  ob::StateSpacePtr Rn(std::make_shared<ob::RealVectorStateSpace>(N));

  space_ = SE3 + Rn;
  ob::SE3StateSpace *cspaceSE3 = space_->as<ob::CompoundStateSpace>()->as<ob::SE3StateSpace>(0);
  ob::RealVectorStateSpace *cspaceRn = space_->as<ob::CompoundStateSpace>()->as<ob::RealVectorStateSpace>(1);

  //std::cout << startRn->getDimension() << std::endl;

  //###########################################################################
  // Set bounds
  //###########################################################################
  std::vector<double> minimum, maximum;
  //PropertyMap props;
  //space->Properties(props);
  //props.getArray("minimum",minimum);
  //props.getArray("maximum",maximum);
  minimum = robot->qMin;
  maximum = robot->qMax;

  assert(minimum.size() == 6+N);
  assert(maximum.size() == 6+N);

  vector<double> lowSE3;
  lowSE3.push_back(minimum.at(0));
  lowSE3.push_back(minimum.at(1));
  lowSE3.push_back(minimum.at(2));
  vector<double> highSE3;
  highSE3.push_back(maximum.at(0));
  highSE3.push_back(maximum.at(1));
  highSE3.push_back(maximum.at(2));

  ob::RealVectorBounds boundsSE3(3);
  boundsSE3.low = lowSE3;
  boundsSE3.high = highSE3;
  cspaceSE3->setBounds(boundsSE3);

  vector<double> lowRn, highRn;
  for(int i = 0; i < N; i++){
    lowRn.push_back(minimum.at(i+6));
    highRn.push_back(maximum.at(i+6));
  }
  ob::RealVectorBounds boundsRn(N);

  //ompl does only accept dimensions with strictly positive measure, adding some epsilon space
  double epsilonSpacing=1e-8;
  for(int i = 0; i < N; i++){
    if(abs(lowRn.at(i)-highRn.at(i))<epsilonSpacing){
      highRn.at(i)+=epsilonSpacing;
    }
  }

  boundsRn.low = lowRn;
  boundsRn.high = highRn;
  cspaceRn->setBounds(boundsRn);

}
ob::ScopedState<> ConfigToOMPLState(const Config &q, const ob::StateSpacePtr &s){
  ob::ScopedState<> qompl(s);

  ob::SE3StateSpace::StateType *qomplSE3 = qompl->as<ob::CompoundState>()->as<ob::SE3StateSpace::StateType>(0);
  ob::SO3StateSpace::StateType *qomplSO3 = &qomplSE3->rotation();
  ob::RealVectorStateSpace::StateType *qomplRnSpace = qompl->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(1);
  double* qomplRn = static_cast<ob::RealVectorStateSpace::StateType*>(qomplRnSpace)->values;

  qomplSE3->setXYZ(q[0],q[1],q[2]);
  qomplSO3->setIdentity();

  //q SE3: X Y Z yaw pitch roll
  //double yaw = q[3];
  //double pitch = q[4];
  //double roll = q[5];

  Math3D::EulerAngleRotation Reuler(q(5),q(4),q(3));
  Matrix3 R;
  Reuler.getMatrixXYZ(R);

  QuaternionRotation qr;
  qr.setMatrix(R);

  double qx,qy,qz,qw;
  qr.get(qw,qx,qy,qz);

  qomplSO3->x = qx;
  qomplSO3->y = qy;
  qomplSO3->z = qz;
  qomplSO3->w = qw;

  //Math3D::Matrix3 qrM;
  //qr.getMatrix(qrM);

  for(int i = 0; i < q.size()-6; i++){
    qomplRn[i]=q(6+i);
  }
  return qompl;
}
Config OMPLStateToConfig(const ob::SE3StateSpace::StateType *qomplSE3, const ob::RealVectorStateSpace::StateType *qomplRnState, const ob::StateSpacePtr &s){
  const ob::SO3StateSpace::StateType *qomplSO3 = &qomplSE3->rotation();

  //std::vector<double> reals;
  //s->copyToReals(reals, qomplRnState);
  uint N =  s->getDimension() - 6;
  Config q;
  q.resize(6+N);

  for(int i = 0; i < N; i++){
    q(i+6) = qomplRnState->values[i];
  }

  q(0) = qomplSE3->getX();
  q(1) = qomplSE3->getY();
  q(2) = qomplSE3->getZ();

  double qx = qomplSO3->x;
  double qy = qomplSO3->y;
  double qz = qomplSO3->z;
  double qw = qomplSO3->w;

  Math3D::QuaternionRotation qr(qw, qx, qy, qz);
  Math3D::Matrix3 qrM;
  qr.getMatrix(qrM);
  Math3D::EulerAngleRotation R;
  R.setMatrixXYZ(qrM);

  q(3) = R[2];
  q(4) = R[1];
  q(5) = R[0];

  for(int i = 3; i < 6; i++){
    if(q(i)<0) q(i)+=2*M_PI;
    if(q(i)>2*M_PI) q(i)-=2*M_PI;
  }

  return q;
}
Config OMPLStateToConfig(const ob::State *qompl, const ob::StateSpacePtr &s){
  const ob::SE3StateSpace::StateType *qomplSE3 = qompl->as<ob::CompoundState>()->as<ob::SE3StateSpace::StateType>(0);
  const ob::RealVectorStateSpace::StateType *qomplRnState = qompl->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(1);
  return OMPLStateToConfig(qomplSE3, qomplRnState, s);

}

Config OMPLStateToConfig(const ob::ScopedState<> &qompl, const ob::StateSpacePtr &s){

  const ob::SE3StateSpace::StateType *qomplSE3 = qompl->as<ob::CompoundState>()->as<ob::SE3StateSpace::StateType>(0);
  const ob::RealVectorStateSpace::StateType *qomplRnState = qompl->as<ob::CompoundState>()->as<ob::RealVectorStateSpace::StateType>(1);
  return OMPLStateToConfig(qomplSE3, qomplRnState, s);

}

MotionPlannerOMPLValidityChecker::MotionPlannerOMPLValidityChecker(const ob::SpaceInformationPtr &si, CSpace* space):
  ob::StateValidityChecker(si),_space(space)
{
}
bool MotionPlannerOMPLValidityChecker::isValid(const ob::State* state) const
{
  const ob::StateSpacePtr ssp = si_->getStateSpace();
  Config q = OMPLStateToConfig(state, ssp);
  bool isFeasible = _space->IsFeasible(q);
  return isFeasible;
}


MotionPlannerOMPL::MotionPlannerOMPL(RobotWorld *world, WorldSimulation *sim):
  MotionPlanner(world,sim)
{
}
void MotionPlannerOMPL::test()
{
  std::cout << "Testing OMPL<->Klampt conversion" << std::endl;

  Srand(0);
  for(int i = 0; i < 10; i++){
    uint N = Rand(0,4);
    ob::StateSpacePtr SE3(std::make_shared<ob::SE3StateSpace>());
    ob::StateSpacePtr Rn(std::make_shared<ob::RealVectorStateSpace>(N));
    ob::StateSpacePtr stateSpace = SE3 + Rn;

    Config q;
    q.resize(6+N);
    q.setZero();
    q[0]=Rand(-3,3);
    q[1]=Rand(-3,3);
    q[2]=Rand(-3,3);
    q[3] = Rand(0,2*M_PI);
    q[4] = Rand(0,2*M_PI);
    q[5] = Rand(0,2*M_PI);
    for(int j = 0; j < N; j++){
      q[j+6] = Rand(0,3);
    }
    std::cout << "testing " << i << " : " << std::endl;
    test_conversion(q, stateSpace);
  }


}
void MotionPlannerOMPL::test_conversion(Config &q, ob::StateSpacePtr &stateSpace)
{
  ob::ScopedState<> qm = ConfigToOMPLState(q, stateSpace);
  Config qq = OMPLStateToConfig(qm, stateSpace);

  double epsilon = 1e-6;
  //if((q-qq).norm()>epsilon){
  //  std::cout << "Test failed" << std::endl;
  //  std::cout << std::string(80, '-') << std::endl;
  //  std::cout << q << std::endl;
  //  std::cout << qm << std::endl;
  //  std::cout << qq << std::endl;
  //  std::cout << std::string(80, '-') << std::endl;
  //}
  std::cout << std::string(80, '-') << std::endl;
  std::cout << q << std::endl;
  //std::cout << qm << std::endl;
  std::cout << qq << std::endl;
  std::cout << std::string(80, '-') << std::endl;

}

void propagate(const ob::State *start, const oc::Control *control, const double duration, ob::State *result)
{
  const ob::SE2StateSpace::StateType *se2state = start->as<ob::SE2StateSpace::StateType>();
  const double* pos = se2state->as<ob::RealVectorStateSpace::StateType>(0)->values;
  const double rot = se2state->as<ob::SO2StateSpace::StateType>(1)->value;
  const double* ctrl = control->as<oc::RealVectorControlSpace::ControlType>()->values;

  result->as<ob::SE2StateSpace::StateType>()->setXY(
      pos[0] + ctrl[0] * duration * cos(rot),
      pos[1] + ctrl[0] * duration * sin(rot));
  result->as<ob::SE2StateSpace::StateType>()->setYaw(
      rot    + ctrl[1] * duration);
}
bool MotionPlannerOMPL::solve(Config &p_init, Config &p_goal)
{
  this->_p_init = p_init;
  this->_p_goal = p_goal;
  Robot *robot = _world->robots[_irobot];
  robot->UpdateConfig(_p_init);

  util::SetSimulatedRobot(robot,*_sim,_p_init);//TODO: outsource sim
  this->_world->InitCollisions();

  std::cout << std::string(80, '-') << std::endl;
  std::cout << "Motion Planner:" << this->getName() << std::endl;
  std::cout << "p_init =" << p_init << std::endl;
  std::cout << "p_goal =" << p_goal << std::endl;
  std::cout << std::string(80, '-') << std::endl;
  //###########################################################################
  // Setup Klampt CSpace
  //###########################################################################

  WorldPlannerSettings worldsettings;
  worldsettings.InitializeDefault(*_world);
  //worldsettings.robotSettings[0].worldBounds = AABB3D(plannersettings.worldboundsMin,plannersettings.worldboundsMax);
  //worldsettings.robotSettings[0].contactEpsilon = 1e-2;
  //worldsettings.robotSettings[0].contactIKMaxIters = 100;

  SingleRobotCSpace geometric_cspace = SingleRobotCSpace(*_world,_irobot,&worldsettings);
  if(!IsFeasible( robot, geometric_cspace, _p_init)) return false;
  if(!IsFeasible( robot, geometric_cspace, _p_goal)) return false;

  KinodynamicCSpaceSentinelAdaptor kcspace(&geometric_cspace);
  PropertyMap pmap;
  kcspace.Properties(pmap);
  std::cout << pmap << std::endl;

  //CSpaceGoalSetEpsilonNeighborhood goalSet(&kcspace, _p_goal, plannersettings.goalRegionConvergence);

  //###########################################################################
  // Config init,goal to OMPL start/goal
  //###########################################################################

  GeometricCSpaceOMPL cspace(robot);

  ob::ScopedState<> start = ConfigToOMPLState(p_init, cspace.getPtr());
  ob::ScopedState<> goal  = ConfigToOMPLState(p_goal, cspace.getPtr());
  std::cout << start << std::endl;
  std::cout << goal << std::endl;


  ////###########################################################################
  //// Geometric planning (tested, works)
  ////###########################################################################

  //og::SimpleSetup ss(cspace.getPtr());
  //const ob::SpaceInformationPtr si = ss.getSpaceInformation();

  ////GEOMETRIC PLANNERS
  //ob::PlannerPtr ompl_planner = std::make_shared<og::RRTConnect>(si);
  ////ob::PlannerPtr ompl_planner = std::make_shared<og::RRT>(si);
  ////ob::PlannerPtr ompl_planner = std::make_shared<og::RRTstar>(si);
  ////ob::PlannerPtr ompl_planner = std::make_shared<og::RRTsharp>(si);
  ////ob::PlannerPtr ompl_planner = std::make_shared<og::LazyRRT>(si);

  //ss.setPlanner(ompl_planner);
  ////ss.setPlanner(std::make_shared<og::RRTConnect>(si));

  //si->setStateValidityChecker(std::make_shared<MotionPlannerOMPLValidityChecker>(si, &kcspace));

  //ss.setStartAndGoalStates(start, goal);
  //ss.setup();
  //ob::PlannerStatus solved = ss.solve(10.0);
  //if (solved)
  //{
  //  std::cout << "Found solution:" << std::endl;
  //  ss.simplifySolution();
  //  og::PathGeometric path = ss.getSolutionPath();
  //  path.interpolate();
  //  std::cout << path.length() << std::endl;
  //  std::cout << path.getStateCount() << std::endl;

  //  vector<Config> keyframes;
  //  for(int i = 0; i < path.getStateCount(); i++)
  //  {
  //    ob::State *state = path.getState(i);
  //    Config cur = OMPLStateToConfig(state, cspace.getPtr());
  //    _keyframes.push_back(cur);
  //  }
  //  std::cout << std::string(80, '-') << std::endl;
  //}else{
  //  std::cout << "No solution found" << std::endl;
  //}

  //###########################################################################
  // Kinodynamic planner
  //###########################################################################
  //KINODYNAMIC PLANNERS
  auto control_cspace(std::make_shared<oc::RealVectorControlSpace>(cspace.getPtr(), 6));
  //oc::ControlSpace

  ob::RealVectorBounds cbounds(6);
  cbounds.setLow(-0.3);
  cbounds.setHigh(0.3);
  control_cspace->setBounds(cbounds);

  oc::SimpleSetup ss(control_cspace);
  const oc::SpaceInformationPtr si = ss.getSpaceInformation();

  ss.setStatePropagator(propagate);

  //ss->setStateValidityChecker(std::make_shared<MotionPlannerOMPLValidityChecker>(si, &kcspace));
  //oc::SimpleSetup ss(cspace);
  //const oc::SpaceInformationPtr si = ss.getSpaceInformation();
  ob::PlannerPtr ompl_planner = std::make_shared<oc::RRT>(si);
  //ob::PlannerPtr ompl_planner = std::make_shared<oc::Syclop>(si);
  //ob::PlannerPtr ompl_planner = std::make_shared<oc::SyclopRRT>(si);
  //ob::PlannerPtr ompl_planner = std::make_shared<oc::SyclopEST>(si);
  //ob::PlannerPtr ompl_planner = std::make_shared<oc::SST>(si);
  //ob::PlannerPtr ompl_planner = std::make_shared<oc::PDST>(si);
  //ob::PlannerPtr ompl_planner = std::make_shared<oc::LTLPlanner>(si);
  //ob::PlannerPtr ompl_planner = std::make_shared<oc::KPIECE1>(si);
  //ob::PlannerPtr ompl_planner = std::make_shared<oc::EST>(si);

  ss.setup();
  ob::PlannerStatus solved = ss.solve(10.0);
  if (solved)
  {
    std::cout << "Found solution:" << std::endl;
    oc::PathControl path_control = ss.getSolutionPath();
    og::PathGeometric path = path_control.asGeometric();
    path.interpolate();
    std::cout << path.length() << std::endl;
    std::cout << path.getStateCount() << std::endl;

    vector<Config> keyframes;
    for(int i = 0; i < path.getStateCount(); i++)
    {
      ob::State *state = path.getState(i);
      Config cur = OMPLStateToConfig(state, cspace.getPtr());
      _keyframes.push_back(cur);
    }
    std::cout << std::string(80, '-') << std::endl;
  }else{
    std::cout << "No solution found" << std::endl;
  }

  return solved;
}

