#include <cstdlib>
#include "g2o/stuff/command_args.h"
#include "g2o/stuff/sampler.h"
#include "simulator2d.h"
#include "g2o/core/optimizable_graph.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace g2o;
using namespace std;


int main(int argc, char** argv) {
  CommandArgs arg;
  int nlandmarks;
  int simSteps;
  double worldSize;
  bool hasOdom;
  bool hasPoseSensor;
  bool hasPointSensor;
  bool hasPointBearingSensor;
  bool hasCompass;
  bool hasGPS;

  std::string outputFilename;
  arg.param("simSteps", simSteps, 100, "number of simulation steps");
  arg.param("nLandmarks", nlandmarks, 1000, "number of landmarks");
  arg.param("worldSize", worldSize, 25.0, "size of the world");
  arg.param("hasOdom",        hasOdom, false,  "the robot has an odometry" );
  arg.param("hasPointSensor", hasPointSensor, false, "the robot has a point sensor" );
  arg.param("hasPointBearingSensor", hasPointBearingSensor, false, "the robot has a point bearing sensor" );
  arg.param("hasPoseSensor",  hasPoseSensor, false,  "the robot has a pose sensor" );
  arg.param("hasCompass",     hasCompass, false, "the robot has a compass");
  arg.param("hasGPS",         hasGPS, false, "the robot has a GPS");
  arg.paramLeftOver("graph-output", outputFilename, "simulator_out.g2o", "graph file which will be written", true);


  arg.parseArgs(argc, argv);
  
  std::tr1::ranlux_base_01 generator;
  OptimizableGraph graph;
  World world(&graph);
  for (int i=0; i<nlandmarks; i++){
    WorldObjectPointXY * landmark = new WorldObjectPointXY;
    double x = sampleUniform(-.5,.5, &generator)*worldSize;
    double y = sampleUniform(-.5,.5, &generator)*worldSize;
    landmark->vertex()->setEstimate(Vector2d(x,y));
    world.addWorldObject(landmark);
  }
  Robot2D robot(&world, "myRobot");
  world.addRobot(&robot);


  stringstream ss;
  ss << "-ws" << worldSize;
  ss << "-nl" << nlandmarks;
  ss << "-steps" << simSteps;

  if (hasOdom) {
    SensorOdometry2D* odometrySensor=new SensorOdometry2D("odometry");
    robot.addSensor(odometrySensor);
    Matrix3d odomInfo = odometrySensor->information();
    odomInfo.setIdentity();
    odomInfo*=100;
    odomInfo(2,2)=1000;
    odometrySensor->setInformation(odomInfo);
    ss << "-odom";
  }

  if (hasPoseSensor) {
    SensorPose2D* poseSensor = new SensorPose2D("poseSensor");
    robot.addSensor(poseSensor);
    Matrix3d poseInfo = poseSensor->information();
    poseInfo.setIdentity();
    poseInfo*=100;
    poseInfo(2,2)=1000;
    poseSensor->setInformation(poseInfo);
    ss << "-pose";
  }
  
  if (hasPointSensor) {
    SensorPointXY* pointSensor = new SensorPointXY("pointSensor");
    robot.addSensor(pointSensor);
    Matrix2d pointInfo = pointSensor->information();
    pointInfo.setIdentity();
    pointInfo*=100;
    pointSensor->setInformation(pointInfo);
    ss << "-pointXY";
  }

  if (hasPointBearingSensor) {
    SensorPointXYBearing* bearingSensor = new SensorPointXYBearing("bearingSensor");
    robot.addSensor(bearingSensor);
    bearingSensor->setInformation(bearingSensor->information()*1000);
    ss << "-pointBearing";
  }
  

  
  robot.move(SE2());
  double pStraight=0.7;
  SE2 moveStraight; moveStraight.setTranslation(Vector2d(1., 0.));
  double pLeft=0.15;
  SE2 moveLeft; moveLeft.setRotation(Rotation2Dd(M_PI/2));
  //double pRight=0.15;
  SE2 moveRight; moveRight.setRotation(Rotation2Dd(-M_PI/2));

  for (int i=0; i<simSteps; i++){
    cerr << "m";
    SE2 move;
    SE2 pose=robot.pose();
    double dtheta=-100;
    Vector2d dt;
    if (pose.translation().x() < -.5*worldSize){
      dtheta = 0;
    }

    if (pose.translation().x() >  .5*worldSize){
      dtheta = -M_PI;
    }
    
    if (pose.translation().y() < -.5*worldSize){
      dtheta = M_PI/2;
    }

    if (pose.translation().y() >  .5*worldSize){
      dtheta = -M_PI/2;
    }
    if (dtheta< -M_PI) {
      // select a random move of the robot
      double sampled=sampleUniform(0.,1.,&generator);
      if (sampled<pStraight)
        move=moveStraight;
      else if (sampled<pStraight+pLeft)
        move=moveLeft;
      else
        move=moveRight;
    } else {
      double mTheta=dtheta-pose.rotation().angle();
      move.setRotation(Rotation2Dd(mTheta));
      if (move.rotation().angle()<std::numeric_limits<double>::epsilon()){
        move.setTranslation(Vector2d(1., 0.));
      }
    }
    robot.relativeMove(move);
    // do a sense
    cerr << "s";
    robot.sense();
  }
  //string fname=outputFilename + ss.str() + ".g2o";
  ofstream testStream(outputFilename.c_str());
  graph.save(testStream);
 
}
