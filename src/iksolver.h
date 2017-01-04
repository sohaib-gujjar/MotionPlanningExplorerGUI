#pragma once
#include <Interface/SimTestGUI.h>
#include <KrisLibrary/robotics/IK.h>
#include <KrisLibrary/robotics/IKFunctions.h>
#include <Modeling/World.h>
#include <View/ViewIK.h>

//Interface to Klampt IKSolver 
class IKSolver{
  protected:
    Real _tolerance;
    int _iters;
    int _verbose;
    vector<IKGoal> _constraints;
    Config q_solution;
    Config q_initial;

    bool _isSolved;
    Robot *_robot;
    RobotWorld *_world;

    bool _isInitialized;

    virtual string GetRobotName() = 0;
    virtual bool solveIKconstraints();
    virtual vector<IKGoal> GetConstraints() = 0;
  private:
    void preSolve();
    void postSolve();
    void init();
  public:
    IKSolver(RobotWorld *world);

    string GetIKRobotName();
    vector<IKGoal> GetIKGoalConstraints();
    virtual bool solve_default();
    bool solve();
    void visualize();
    ///Set IK solution to real robot
    void SetConfigSimulatedRobot(WorldSimulation &sim);
    IKGoal LinkToGoalTransRot( const char *linkName, double x, double y, double z, Matrix3 &rotation);
    IKGoal LinkToGoalTrans( const char *linkName, double x, double y, double z);
    IKGoal LinkToGoalRot( const char *linkName, Matrix3 &rotation);
};

