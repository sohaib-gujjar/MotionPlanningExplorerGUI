#include "roadmap.h"
#include "gui/drawMotionPlanner.h"
#include "gui/colors.h"
#include "planner/validitychecker/validity_checker_ompl.h"

using namespace GLDraw;
//using NO_VERTEX = ob::PlannerData::NO_VERTEX;
//using NO_EDGE = ob::PlannerData::NO_EDGE;
//typedef ob::PlannerData::NO_VERTEX NO_VERTEX;

Roadmap::Roadmap(){
  cVertex = green;
  cEdge = green;
  cEdgeRemoved = grey;
}

std::vector<Config> Roadmap::GetVertices(){
  return V;
}
std::vector<std::pair<Config,Config>> Roadmap::GetEdges(){
  return E;
}
void Roadmap::SetVertices(const std::vector<Config> &V_){
  V = V_;
}
void Roadmap::SetEdges(const std::vector<std::pair<Config,Config>> &E_){
  E = E_;
}

void Roadmap::CreateFromPlannerDataOnlySufficient(const ob::PlannerDataPtr pd, CSpaceOMPL *cspace_){
  cspace = cspace_;
  ob::SpaceInformationPtr si = pd->getSpaceInformation();
  std::cout << "roadmap from planner data with " << pd->numVertices() << " vertices and " << pd->numEdges() << " edges" << std::endl;
  OMPLValidityCheckerPtr validity_checker = std::static_pointer_cast<OMPLValidityChecker>(cspace->StateValidityCheckerPtr(si));

  pds = std::make_shared<ob::PlannerData>(si);

  for(uint i = 0; i < pd->numVertices(); i++){
    uint vidx = i;
    ob::PlannerDataVertex v = pd->getVertex(vidx);
    const ob::State* s = v.getState();

    Config q1 = cspace->OMPLStateToConfig(s);
    //ob::StateValidityCheckerPtr validity_checker = cspace->StateValidityCheckerPtr(si);

    bool isVertexSufficient = validity_checker->isSufficient(s);

    if(isVertexSufficient){
      V.push_back(q1);
      uint vsidx = pds->addVertex(v);
      if(pd->isStartVertex(vidx)){
        pds->markStartState(s);
      }
      if(pd->isGoalVertex(vidx)){
        pds->markGoalState(s);
      }
      std::vector<uint> edgeList;
      pd->getEdges(vidx, edgeList);
      for(uint j = 0; j < edgeList.size(); j++){
        uint widx = edgeList.at(j);
        ob::PlannerDataVertex w = pd->getVertex(widx);
        const ob::State* sj = w.getState();
        Config q2 = cspace->OMPLStateToConfig(sj);
        Config qedge  = q2 - q1;
        bool isEdgeSufficient = validity_checker->isSufficient(s) && validity_checker->isSufficient(sj);

        if(isEdgeSufficient){
          if(!pds->vertexExists(w)){
            pds->addVertex(w);
          }
          uint wsidx = pds->vertexIndex(w);
          ob::PlannerDataEdge e = pd->getEdge(vidx, widx);
          ob::Cost weight;
          pd->getEdgeWeight(vidx,widx,&weight);

          pds->addEdge(vsidx,wsidx,e,weight);
          E.push_back(std::make_pair(q1,q2));
        }
      }
    }

  }
  std::cout << "sufficient roadmap construction: v=" << V.size() << "/" << pd->numVertices() << " e=" << E.size() << "/" << pd->numEdges() << std::endl;
  if(pds->getStartIndex(0)==ob::PlannerData::INVALID_INDEX || pds->getGoalIndex(0)==ob::PlannerData::INVALID_INDEX){
    std::cout << "start or goal is not sufficient. Not handled yet!" << std::endl;
    exit(0);
  }

  lemon = new LemonInterface(pds);
}

void Roadmap::CreateFromPlannerDataOnlyNecessary(const ob::PlannerDataPtr pd, CSpaceOMPL *cspace_){
  cspace = cspace_;
  ob::SpaceInformationPtr si = pd->getSpaceInformation();
  std::cout << "roadmap from planner data with " << pd->numVertices() << " vertices and " << pd->numEdges() << " edges" << std::endl;
  OMPLValidityCheckerPtr validity_checker = std::static_pointer_cast<OMPLValidityChecker>(cspace->StateValidityCheckerPtr(si));

  pds = std::make_shared<ob::PlannerData>(si);

  for(uint i = 0; i < pd->numVertices(); i++){
    uint vidx = i;

    ob::PlannerDataVertex v = pd->getVertex(vidx);
    const ob::State* s = v.getState();
    Config q1 = cspace->OMPLStateToConfig(s);

    bool isVertexSufficient = validity_checker->isSufficient(s);

    if(!isVertexSufficient){
      V.push_back(q1);
      uint vsidx = pds->addVertex(v);
      if(pd->isStartVertex(vidx)){
        pds->markStartState(s);
      }
      if(pd->isGoalVertex(vidx)){
        pds->markGoalState(s);
      }
      std::vector<uint> edgeList;
      pd->getEdges(vidx, edgeList);
      for(uint j = 0; j < edgeList.size(); j++){

        uint widx = edgeList.at(j);
        ob::PlannerDataVertex w = pd->getVertex(widx);
        const ob::State* sj = w.getState();
        Config q2 = cspace->OMPLStateToConfig(sj);
        Config qedge  = q2 - q1;
        bool isEdgeSufficient = validity_checker->isSufficient(sj);

        if(!isEdgeSufficient){
          if(!pds->vertexExists(w)){
            pds->addVertex(w);
          }
          uint wsidx = pds->vertexIndex(w);
          ob::PlannerDataEdge e = pd->getEdge(vidx, widx);
          ob::Cost weight;
          pd->getEdgeWeight(vidx,widx,&weight);

          pds->addEdge(vsidx,wsidx,e,weight);
          E.push_back(std::make_pair(q1,q2));
        }
      }
    }

  }
  std::cout << "necessary roadmap construction: v=" << V.size() << "/" << pd->numVertices() << " e=" << E.size() << "/" << pd->numEdges() << std::endl;

  lemon = new LemonInterface(pds);
}

void Roadmap::CreateFromPlannerData(const ob::PlannerDataPtr pd, CSpaceOMPL *cspace_){
  cspace = cspace_;
  ob::SpaceInformationPtr si = pd->getSpaceInformation();
  std::cout << "roadmap from planner data with " << pd->numVertices() << " vertices and " << pd->numEdges() << " edges" << std::endl;
  OMPLValidityCheckerPtr validity_checker = std::static_pointer_cast<OMPLValidityChecker>(cspace->StateValidityCheckerPtr(si));

  for(uint i = 0; i < pd->numVertices(); i++){
    ob::PlannerDataVertex v = pd->getVertex(i);
    const ob::State* s = v.getState();
    Config q1 = cspace->OMPLStateToConfig(s);

    V.push_back(q1);

    std::vector<uint> edgeList;
    pd->getEdges(i, edgeList);
    for(uint j = 0; j < edgeList.size(); j++){
      ob::PlannerDataVertex w = pd->getVertex(edgeList.at(j));
      const ob::State* sj = w.getState();
      Config q2 = cspace->OMPLStateToConfig(sj);
      E.push_back(std::make_pair(q1,q2));
      uint vi = pd->vertexIndex(v);
      uint wi = pd->vertexIndex(w);
      //if(pd->edgeExists(vi,wi)){
      //  std::cout << "trying to set non existing edge!" << std::endl;
      //  std::cout << "edge: " << q1 << "<->" << q2 << std::endl;
      //}
    }
  }
  Nvertices = pd->numVertices();
  Nedges = pd->numEdges();
  pds = pd;
  pds->decoupleFromPlanner();
  lemon = new LemonInterface(pds);

}

using Graph = ob::PlannerData::Graph;
using Vertex = Graph::Vertex;

std::vector<Config> Roadmap::GetShortestPath(){
  shortest_path_idxs = lemon->GetShortestPath();
  shortest_path = VertexPathToConfigPath(shortest_path_idxs);
  return shortest_path;
}


std::vector<Config> Roadmap::VertexPathToConfigPath( const std::vector<Vertex> &path){
  const ob::SpaceInformationPtr si = pds->getSpaceInformation();

  std::vector<const ob::State*> states;
  for(uint k = 0; k < path.size(); k++){
    Vertex v = path.at(k);
    const ob::State *sk = pds->getVertex(v).getState();
    shortest_path_vertex.push_back(pds->getVertex(v));
    states.push_back(sk);
  }

  og::PathGeometric omplpath(si);
  for(uint k = 0; k < states.size(); k++){
    omplpath.append(states.at(k));
  }
  //og::PathSimplifier shortcutter(si);
  //shortcutter.shortcutPath(omplpath);

  //omplpath.interpolate();

  std::vector<ob::State *> interpolated_states = omplpath.getStates();
  std::vector<Config> keyframes;
  for(uint k = 0; k < interpolated_states.size(); k++)
  {
    ob::State *state = interpolated_states.at(k);
    Config q = cspace->OMPLStateToConfig(state);
    keyframes.push_back(q);
  }
  return keyframes;
}
int Roadmap::numEdges(){
  return pds->numEdges();
}
int Roadmap::numVertices(){
  return pds->numVertices();
}
void Roadmap::removeInfeasibleEdgeAlongShortestPath(uint index){
  //move along shortest_path_idxs remove all infeasible edges

  Vertex vidx = shortest_path_idxs.at(index);
  Vertex widx = shortest_path_idxs.at(index+1);
  if(!pds->removeEdge(vidx,widx)){
    //PlannerData is implemented as DG
    pds->removeEdge(widx,vidx);
    //std::cout << std::string(80, '-') << std::endl; 
    //std::cout << "unable to remove edge between vertices " << vidx << " " << widx << std::endl;
    //std::cout << std::string(80, '-') << std::endl; 

    //if(pds->getVertex(vidx)==ob::PlannerData::NO_VERTEX){
    //  std::cout << "vertex " << vidx << " does not exists" << std::endl;
    //}
    //if(pds->getVertex(widx)==ob::PlannerData::NO_VERTEX){
    //  std::cout << "vertex " << widx << " does not exists" << std::endl;
    //}
    //Config q1 = cspace->OMPLStateToConfig(pds->getVertex(vidx).getState());
    //Config q2 = cspace->OMPLStateToConfig(pds->getVertex(widx).getState());
    //std::cout << "vertex " << vidx << " " << q1 << std::endl;
    //std::cout << "vertex " << widx << " " << q2 << std::endl;
    //std::cout << "should be:" << std::endl;

    //std::cout << shortest_path.at(index) << std::endl;
    //std::cout << shortest_path.at(index+1) << std::endl;

    //ob::PlannerDataVertex v = shortest_path_vertex.at(index);
    //ob::PlannerDataVertex w = shortest_path_vertex.at(index+1);
    //Config q3 = cspace->OMPLStateToConfig(v.getState());
    //Config q4 = cspace->OMPLStateToConfig(w.getState());

    //uint vvi = pds->vertexIndex(v);
    //uint wwi = pds->vertexIndex(w);
    //std::cout << "vertex " << vvi << " : " << q3 << std::endl;
    //std::cout << "vertex " << wwi << " : " << q4 << std::endl;
    //std::cout << "edge: " << pds->edgeExists(vvi,wwi) << std::endl;
    //std::cout << "edge: " << pds->edgeExists(wwi,vvi) << std::endl;
    // for(uint k = 0; k < E.size(); k++){
    //   std::cout << E.at(k).first << "<->" << E.at(k).second << std::endl;
    // }

    //exit(0);
  }

  Config q1 = cspace->OMPLStateToConfig(pds->getVertex(vidx).getState());
  Config q2 = cspace->OMPLStateToConfig(pds->getVertex(widx).getState());
  E_removed.push_back(std::make_pair(q1,q2));

  delete lemon;
  lemon = new LemonInterface(pds);
}

void Roadmap::DrawGL(GUIState& state)
{
  if(!pds) return;

  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND); 

  glPointSize(10);
  //move along PD and visualize

  if(state("draw_roadmap_vertices")){
    setColor(cVertex);
    //for(uint k = 0; k < V.size(); k++){
    //  Config q = V.at(k);
    //  Vector3 v(q(0),q(1),q(2));
    //  drawPoint(v);
    //}
    for(uint vidx = 0; vidx < Nvertices; vidx++){
      ob::PlannerDataVertex v = pds->getVertex(vidx);
      if(v!=ob::PlannerData::NO_VERTEX){
        Config q = cspace->OMPLStateToConfig(v.getState());
        Vector3 v(q(0),q(1),q(2));
        drawPoint(v);
      }
    }
  }
  glLineWidth(5);
  if(state("draw_roadmap_edges")){
    setColor(cEdge);
    for(uint vidx = 0; vidx < Nvertices; vidx++){
      ob::PlannerDataVertex v = pds->getVertex(vidx);
      if(v==ob::PlannerData::NO_VERTEX) continue;
      Config q1 = cspace->OMPLStateToConfig(v.getState());

      std::vector<uint> edgeList;
      pds->getEdges(vidx, edgeList);
      for(uint j = 0; j < edgeList.size(); j++){
        ob::PlannerDataVertex w = pds->getVertex(edgeList.at(j));
        if(w==ob::PlannerData::NO_VERTEX) continue;
        Config q2 = cspace->OMPLStateToConfig(w.getState());
        Vector3 v1(q1(0),q1(1),q1(2));
        Vector3 v2(q2(0),q2(1),q2(2));
        drawLineSegment(v1,v2);
      }
    }
    setColor(cEdgeRemoved);
    for(uint k = 0; k < E_removed.size(); k++){
      std::pair<Config,Config> e = E_removed.at(k);
      Config q1 = e.first;
      Config q2 = e.second;
      Vector3 v1(q1(0),q1(1),q1(2));
      Vector3 v2(q2(0),q2(1),q2(2));
      drawLineSegment(v1,v2);

    }
  }

  glEnable(GL_LIGHTING);
  glDisable(GL_BLEND); 
}
