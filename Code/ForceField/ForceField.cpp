// $Id$
//
//  Copyright (C) 2004-2006 Rational Discovery LLC
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//
#include "ForceField.h"
#include "Contrib.h"

#include <RDGeneral/Invariant.h>
#include <Numerics/Optimizer/BFGSOpt.h>

#ifdef RDK_BUILD_WITH_OPENMM
#include <OpenMM.h>
#include <openmm/Units.h>
#include <openmm/internal/lbfgs.h>
#endif

namespace ForceFieldsHelper {
class calcEnergy {
 public:
  calcEnergy(ForceFields::ForceField *ffHolder) : mp_ffHolder(ffHolder){};
  double operator()(double *pos) const { return mp_ffHolder->calcEnergy(pos); }

 private:
  ForceFields::ForceField *mp_ffHolder;
};

class calcGradient {
 public:
  calcGradient(ForceFields::ForceField *ffHolder) : mp_ffHolder(ffHolder){};
  double operator()(double *pos, double *grad) const {
    double res = 1.0;
    // the contribs to the gradient function use +=, so we need
    // to zero the grad out before moving on:
    for (unsigned int i = 0;
         i < mp_ffHolder->numPoints() * mp_ffHolder->dimension(); i++) {
      grad[i] = 0.0;
    }
    mp_ffHolder->calcGrad(pos, grad);

#if 1
    // FIX: this hack reduces the gradients so that the
    // minimizer is more efficient.
    double maxGrad = -1e8;
    double gradScale = 0.1;
    for (unsigned int i = 0;
         i < mp_ffHolder->numPoints() * mp_ffHolder->dimension(); i++) {
      grad[i] *= gradScale;
      if (grad[i] > maxGrad) maxGrad = grad[i];
    }
    // this is a continuation of the same hack to avoid
    // some potential numeric instabilities:
    if (maxGrad > 10.0) {
      while (maxGrad * gradScale > 10.0) {
        gradScale *= .5;
      }
      for (unsigned int i = 0;
           i < mp_ffHolder->numPoints() * mp_ffHolder->dimension(); i++) {
        grad[i] *= gradScale;
      }
    }
    res = gradScale;
#endif
    return res;
  }

 private:
  ForceFields::ForceField *mp_ffHolder;
};
}

namespace ForceFields {

ForceField::~ForceField() {
  d_numPoints = 0;
  d_positions.clear();
  d_contribs.clear();
  delete[] dp_distMat;
  dp_distMat = 0;
}

ForceField::ForceField(const ForceField &other)
    : d_dimension(other.d_dimension),
      df_init(false),
      d_numPoints(other.d_numPoints),
      dp_distMat(0) {
  d_contribs.clear();
  BOOST_FOREACH (const ContribPtr &contrib, other.d_contribs) {
    ForceFieldContrib *ncontrib = contrib->copy();
    ncontrib->dp_forceField = this;
    d_contribs.push_back(ContribPtr(ncontrib));
  }
};

double ForceField::distance(unsigned int i, unsigned int j, double *pos) {
  PRECONDITION(df_init, "not initialized");
  URANGE_CHECK(i, d_numPoints);
  URANGE_CHECK(j, d_numPoints);
  if (j < i) {
    int tmp = j;
    j = i;
    i = tmp;
  }
  unsigned int idx = i + j * (j + 1) / 2;
  CHECK_INVARIANT(idx < d_matSize, "Bad index");
  double &res = dp_distMat[idx];
  if (res < 0.0) {
    // we need to calculate this distance:
    if (!pos) {
      res = 0.0;
      for (unsigned int idx = 0; idx < d_dimension; ++idx) {
        double tmp =
            (*(this->positions()[i]))[idx] - (*(this->positions()[j]))[idx];
        res += tmp * tmp;
      }
    } else {
      res = 0.0;
#if 0
      for (unsigned int idx = 0; idx < d_dimension; idx++) {
        double tmp = pos[d_dimension * i + idx] - pos[d_dimension * j + idx];
        res += tmp * tmp;
      }
#else
      double *pi = &(pos[d_dimension * i]), *pj = &(pos[d_dimension * j]);
      for (unsigned int idx = 0; idx < d_dimension; ++idx, ++pi, ++pj) {
        double tmp = *pi - *pj;
        res += tmp * tmp;
      }
#endif
    }
    res = sqrt(res);
  }
  return res;
}

double ForceField::distance(unsigned int i, unsigned int j, double *pos) const {
  PRECONDITION(df_init, "not initialized");
  URANGE_CHECK(i, d_numPoints);
  URANGE_CHECK(j, d_numPoints);
  if (j < i) {
    int tmp = j;
    j = i;
    i = tmp;
  }
  double res;
  if (!pos) {
    res = 0.0;
    for (unsigned int idx = 0; idx < d_dimension; ++idx) {
      double tmp =
          (*(this->positions()[i]))[idx] - (*(this->positions()[j]))[idx];
      res += tmp * tmp;
    }
  } else {
    res = 0.0;
#if 0
    for (unsigned int idx = 0; idx < d_dimension; idx++) {
      double tmp = pos[d_dimension * i + idx] - pos[d_dimension * j + idx];
      res += tmp * tmp;
    }
#else
    double *pi = &(pos[d_dimension * i]), *pj = &(pos[d_dimension * j]);
    for (unsigned int idx = 0; idx < d_dimension; ++idx, ++pi, ++pj) {
      double tmp = *pi - *pj;
      res += tmp * tmp;
    }
#endif
  }
  res = sqrt(res);
  return res;
}

void ForceField::initialize() {
  // clean up if we have used this already:
  df_init = false;
  delete[] dp_distMat;
  dp_distMat = 0;

  d_numPoints = d_positions.size();
  d_matSize = d_numPoints * (d_numPoints + 1) / 2;
  dp_distMat = new double[d_matSize];
  this->initDistanceMatrix();
  df_init = true;
}

int ForceField::minimize(unsigned int maxIts, double forceTol,
                         double energyTol) {
  return minimize(0, NULL, maxIts, forceTol, energyTol);
}

int ForceField::minimize(unsigned int snapshotFreq,
                         RDKit::SnapshotVect *snapshotVect, unsigned int maxIts,
                         double forceTol, double energyTol) {
  PRECONDITION(df_init, "not initialized");
  PRECONDITION(static_cast<unsigned int>(d_numPoints) == d_positions.size(),
               "size mismatch");
  if (d_contribs.empty()) return 0;

  unsigned int numIters = 0;
  unsigned int dim = this->d_numPoints * d_dimension;
  double finalForce;
  double *points = new double[dim];

  this->scatter(points);
  ForceFieldsHelper::calcEnergy eCalc(this);
  ForceFieldsHelper::calcGradient gCalc(this);

  int res =
      BFGSOpt::minimize(dim, points, forceTol, numIters, finalForce, eCalc,
                        gCalc, snapshotFreq, snapshotVect, energyTol, maxIts);
  this->gather(points);

  delete[] points;
  return res;
}

double ForceField::calcEnergy(std::vector<double> *contribs) const {
  PRECONDITION(df_init, "not initialized");
  double res = 0.0;
  if (d_contribs.empty()) return res;
  if (contribs) {
    contribs->clear();
    contribs->reserve(d_contribs.size());
  }

  unsigned int N = d_positions.size();
  double *pos = new double[d_dimension * N];
  this->scatter(pos);
  // now loop over the contribs
  for (ContribPtrVect::const_iterator contrib = d_contribs.begin();
       contrib != d_contribs.end(); contrib++) {
    double e = (*contrib)->getEnergy(pos);
    res += e;
    if (contribs) {
      contribs->push_back(e);
    }
  }
  delete[] pos;
  return res;
}

double ForceField::calcEnergy(double *pos) {
  PRECONDITION(df_init, "not initialized");
  PRECONDITION(pos, "bad position vector");
  double res = 0.0;

  this->initDistanceMatrix();
  if (d_contribs.empty()) return res;

  // now loop over the contribs
  for (ContribPtrVect::const_iterator contrib = d_contribs.begin();
       contrib != d_contribs.end(); contrib++) {
    double E = (*contrib)->getEnergy(pos);
    res += E;
  }
  return res;
}

void ForceField::calcGrad(double *grad) const {
  PRECONDITION(df_init, "not initialized");
  PRECONDITION(grad, "bad gradient vector");
  if (d_contribs.empty()) return;

  unsigned int N = d_positions.size();
  double *pos = new double[d_dimension * N];
  this->scatter(pos);
  for (ContribPtrVect::const_iterator contrib = d_contribs.begin();
       contrib != d_contribs.end(); contrib++) {
    (*contrib)->getGrad(pos, grad);
  }
  // zero out gradient values for any fixed points:
  for (INT_VECT::const_iterator it = d_fixedPoints.begin();
       it != d_fixedPoints.end(); it++) {
    CHECK_INVARIANT(static_cast<unsigned int>(*it) < d_numPoints,
                    "bad fixed point index");
    unsigned int idx = d_dimension * (*it);
    for (unsigned int di = 0; di < this->dimension(); ++di) {
      grad[idx + di] = 0.0;
    }
  }
  delete[] pos;
}
void ForceField::calcGrad(double *pos, double *grad) {
  PRECONDITION(df_init, "not initialized");
  PRECONDITION(pos, "bad position vector");
  PRECONDITION(grad, "bad gradient vector");
  if (d_contribs.empty()) return;

  for (ContribPtrVect::const_iterator contrib = d_contribs.begin();
       contrib != d_contribs.end(); contrib++) {
    (*contrib)->getGrad(pos, grad);
  }

  for (INT_VECT::const_iterator it = d_fixedPoints.begin();
       it != d_fixedPoints.end(); it++) {
    CHECK_INVARIANT(static_cast<unsigned int>(*it) < d_numPoints,
                    "bad fixed point index");
    unsigned int idx = d_dimension * (*it);
    for (unsigned int di = 0; di < this->dimension(); ++di) {
      grad[idx + di] = 0.0;
    }
  }
}

void ForceField::scatter(double *pos) const {
  PRECONDITION(df_init, "not initialized");
  PRECONDITION(pos, "bad position vector");

  unsigned int tab = 0;
  for (unsigned int i = 0; i < d_positions.size(); i++) {
    for (unsigned int di = 0; di < this->dimension(); ++di) {
      pos[tab + di] = (*d_positions[i])[di];  //->x;
    }
    tab += this->dimension();
  }
  POSTCONDITION(tab == this->dimension() * d_positions.size(), "bad index");
}

void ForceField::gather(double *pos) {
  PRECONDITION(df_init, "not initialized");
  PRECONDITION(pos, "bad position vector");

  unsigned int tab = 0;
  for (unsigned int i = 0; i < d_positions.size(); i++) {
    for (unsigned int di = 0; di < this->dimension(); ++di) {
      (*d_positions[i])[di] = pos[tab + di];
    }
    tab += this->dimension();
  }
}

void ForceField::initDistanceMatrix() {
  PRECONDITION(d_numPoints, "no points");
  PRECONDITION(dp_distMat, "no distance matrix");
  PRECONDITION(static_cast<unsigned int>(d_numPoints * (d_numPoints + 1) / 2) <=
               d_matSize, "matrix size mismatch");
  for (unsigned int i = 0; i < d_numPoints * (d_numPoints + 1) / 2; i++) {
    dp_distMat[i] = -1.0;
  }
}

#ifdef RDK_BUILD_WITH_OPENMM
const double OpenMMForceField::DEFAULT_STEP_SIZE = 2.0;

OpenMMForceField::OpenMMForceField() :
  ForceField::ForceField(3),
  d_openmmSystem(new OpenMM::System()),
  d_openmmContext(NULL),
  d_stepSize(DEFAULT_STEP_SIZE) {
  d_openmmIntegrator = new OpenMM::VerletIntegrator(d_stepSize * OpenMM::PsPerFs);
}

OpenMMForceField::~OpenMMForceField() {
  delete d_openmmSystem;
  delete d_openmmIntegrator;
  delete d_openmmContext;
}

void OpenMMForceField::initializeContext() {
  if (!d_openmmContext)
    d_openmmContext = (d_platformName.size()
      ? new OpenMM::Context(*d_openmmSystem, *d_openmmIntegrator,
        OpenMM::Platform::getPlatformByName(d_platformName), d_platformProperties)
      : new OpenMM::Context(*d_openmmSystem, *d_openmmIntegrator));
  else
    d_openmmContext->reinitialize();
}

void OpenMMForceField::initializeContext(const std::string& platformName,
  const std::map<std::string, std::string> &properties) {
  initializeContext(OpenMM::Platform::getPlatformByName(platformName), properties);
}

void OpenMMForceField::initializeContext(OpenMM::Platform& platform,
  const std::map<std::string, std::string> &properties) {
  bool needNewContext = (!d_openmmContext || (platform.getName() != d_platformName));
  for (std::map<std::string, std::string>::const_iterator it = d_platformProperties.begin();
    (!needNewContext) && (it != d_platformProperties.end()); ++it) {
    std::map<std::string, std::string>::const_iterator f = properties.find(it->first);
    needNewContext = (f == properties.end());
    if (!needNewContext)
      needNewContext = (it->second != f->second);
  }
  if (needNewContext) {
    d_storedPos.clear();
    if (d_openmmContext) {
      OpenMM::State state(d_openmmContext->getState(OpenMM::State::Positions));
      d_storedPos = state.getPositions();
      delete d_openmmContext;
    }
    d_openmmContext = new OpenMM::Context(*d_openmmSystem,
      *d_openmmIntegrator, platform, properties);
    if (d_storedPos.size()) {
      d_openmmContext->setPositions(d_storedPos);
      d_storedPos.clear();
    }
    d_platformName = platform.getName();
    d_platformProperties = properties;
  }
  else
    d_openmmContext->reinitialize();
}

OpenMM::Context *OpenMMForceField::getContext(bool throwIfNull) const {
  if (throwIfNull)
    PRECONDITION(d_openmmContext, "OpenMM::Context has not been created yet");
  return d_openmmContext;
}

void OpenMMForceField::cloneSystemTo(OpenMM::System& other) const {
  PRECONDITION(!other.getNumParticles(), "The supplied system must be empty");
}

void OpenMMForceField::copyPositionsTo(OpenMM::Context& other) const {
  OpenMM::State state(d_openmmContext->getState(OpenMM::State::Positions));
  other.setPositions(state.getPositions());
}

void OpenMMForceField::copyPositionsFrom(const OpenMM::Context& other) {
  OpenMM::State stateOther(other.getState(OpenMM::State::Positions));
  const std::vector<OpenMM::Vec3> posOther = stateOther.getPositions();
  d_openmmContext->setPositions(posOther);
  updatePositions(posOther);
}

void OpenMMForceField::replacePositions(double *pos) {
  OpenMM::State state(d_openmmContext->getState(OpenMM::State::Positions));
  d_storedPos.clear();
  d_storedPos = state.getPositions();
  std::vector<OpenMM::Vec3> newPos(d_storedPos.size());
  unsigned int n = 0;
  for (unsigned int i = 0; i < newPos.size(); ++i) {
    newPos[i][0] = pos[n++] * OpenMM::NmPerAngstrom;
    newPos[i][1] = pos[n++] * OpenMM::NmPerAngstrom;
    newPos[i][2] = pos[n++] * OpenMM::NmPerAngstrom;
  }
  d_openmmContext->setPositions(newPos);
}

void OpenMMForceField::restorePositions() {
  d_openmmContext->setPositions(d_storedPos);
}

double OpenMMForceField::calcEnergy(std::vector<double> *contribs) const {
  RDUNUSED_PARAM(contribs);
  return OpenMM::KcalPerKJ * d_openmmContext->getState(
    OpenMM::State::Energy).getPotentialEnergy();
}

double OpenMMForceField::calcEnergy(double *pos) {
  replacePositions(pos);
  double res = d_openmmContext->getState(
    OpenMM::State::Energy).getPotentialEnergy();
  restorePositions();

  return res;
}

void OpenMMForceField::calcGrad(double *forces) const {
  const std::vector<OpenMM::Vec3> forceVec = d_openmmContext->getState(
    OpenMM::State::Forces).getForces();
  unsigned int n = 0;
  for (unsigned int i = 0; i < forceVec.size(); ++i) {
    forces[n++] = -forceVec[i][0] * OpenMM::NmPerAngstrom * OpenMM::KcalPerKJ;
    forces[n++] = -forceVec[i][1] * OpenMM::NmPerAngstrom * OpenMM::KcalPerKJ;
    forces[n++] = -forceVec[i][2] * OpenMM::NmPerAngstrom * OpenMM::KcalPerKJ;
  }
}

void OpenMMForceField::calcGrad(double *pos, double *forces) {
  replacePositions(pos);
  calcGrad(forces);
  restorePositions();
}

void OpenMMForceField::updatePositions(const std::vector<OpenMM::Vec3> &openmmPos) {
  for (unsigned int i = 0; i < openmmPos.size(); ++i) {
    (*(d_positions[i]))[0] = openmmPos[i][0] * OpenMM::AngstromsPerNm;
    (*(d_positions[i]))[1] = openmmPos[i][1] * OpenMM::AngstromsPerNm;
    (*(d_positions[i]))[2] = openmmPos[i][2] * OpenMM::AngstromsPerNm;
  }
}

void OpenMMForceField::updatePositions() {
  updatePositions(d_openmmContext->getState(OpenMM::State::Positions).getPositions());
}

int OpenMMForceField::minimize(unsigned int maxIts,
  double forceTol, double energyTol) {
  RDUNUSED_PARAM(energyTol);
#if 0
  int res = ((OpenMM::LocalEnergyMinimizer::minimize(*d_openmmContext,
    forceTol * OpenMM::KJPerKcal * OpenMM::AngstromsPerNm, maxIts)
    == LBFGSERR_MAXIMUMITERATION) ? 1 : 0);
#else
  int res = OpenMM::LocalEnergyMinimizer::minimize(*d_openmmContext,
    forceTol * OpenMM::KJPerKcal * OpenMM::AngstromsPerNm, maxIts);
#endif
  updatePositions();
  
  return res;
}
#endif

}
