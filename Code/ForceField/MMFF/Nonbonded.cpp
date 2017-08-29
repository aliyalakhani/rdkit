// $Id$
//
//  Copyright (C) 2013 Paolo Tosco
//
//  Copyright (C) 2004-2008 Greg Landrum and Rational Discovery LLC
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//
#include "Nonbonded.h"
#include "Params.h"
#include <cmath>
#include <ForceField/ForceField.h>
#include <RDGeneral/Invariant.h>
#include <RDGeneral/utils.h>
#include <GraphMol/ForceFieldHelpers/MMFF/AtomTyper.h>
#ifdef RDK_BUILD_WITH_OPENMM
#include <OpenMM.h>
#include <OpenMMMMFF.h>
#endif

namespace ForceFields {
namespace MMFF {
namespace Utils {
double calcUnscaledVdWMinimum(MMFFVdWCollection *mmffVdW,
                              const MMFFVdW *mmffVdWParamsIAtom,
                              const MMFFVdW *mmffVdWParamsJAtom) {
  double gamma_ij = (mmffVdWParamsIAtom->R_star - mmffVdWParamsJAtom->R_star) /
                    (mmffVdWParamsIAtom->R_star + mmffVdWParamsJAtom->R_star);
  bool haveDonor = ((mmffVdWParamsIAtom->DA == 'D') || (mmffVdWParamsJAtom->DA == 'D'));
  double sigma = 0.5 * (mmffVdWParamsIAtom->R_star + mmffVdWParamsJAtom->R_star) *
                (1.0 + (haveDonor ? 0.0 : mmffVdW->B *
                (1.0 - exp(-(mmffVdW->Beta) * gamma_ij * gamma_ij))));
#if 0
  std::cerr << "ForceFields::MMFF::calcUnscaledVdWMinimum() "
    << "haveDonor = " << haveDonor << ", sigmaI = " << mmffVdWParamsIAtom->R_star
    << ", sigmaJ = " << mmffVdWParamsJAtom->R_star
    << ", gammaIJ = " << gamma_ij << ", sigma = " << sigma << std::endl;
#endif
  return sigma;
}

double calcUnscaledVdWWellDepth(double R_star_ij,
                                const MMFFVdW *mmffVdWParamsIAtom,
                                const MMFFVdW *mmffVdWParamsJAtom) {
  double R_star_ij2 = R_star_ij * R_star_ij;
  static const double c4 = 181.16;
  double epsilon = mmffVdWParamsIAtom->G_i * mmffVdWParamsJAtom->G_i *
          mmffVdWParamsIAtom->alpha_i * mmffVdWParamsJAtom->alpha_i /
          ((sqrt(mmffVdWParamsIAtom->alpha_i / mmffVdWParamsIAtom->N_i) +
            sqrt(mmffVdWParamsJAtom->alpha_i / mmffVdWParamsJAtom->N_i)) *
           R_star_ij2 * R_star_ij2 * R_star_ij2);
#if 0
  std::cerr << "ForceFields::MMFF::calcUnscaledVdWWellDepth() "
    << "GI * alphaI = " << mmffVdWParamsIAtom->G_i * mmffVdWParamsIAtom->alpha_i
    << ", GJ * alphaJ = " << mmffVdWParamsJAtom->G_i * mmffVdWParamsJAtom->alpha_i
    << ", alphaI / NI = " << mmffVdWParamsIAtom->alpha_i / mmffVdWParamsIAtom->N_i
    << ", alphaJ / NJ = " << mmffVdWParamsJAtom->alpha_i / mmffVdWParamsJAtom->N_i
    << ", epsilon = " << epsilon << std::endl;
#endif
  return (c4 * epsilon);
}

double calcVdWEnergy(const double dist, const double R_star_ij,
                     const double wellDepth) {
  double const vdw1 = 1.07;
  double const vdw1m1 = vdw1 - 1.0;
  double const vdw2 = 1.12;
  double const vdw2m1 = vdw2 - 1.0;
  double dist2 = dist * dist;
  double dist7 = dist2 * dist2 * dist2 * dist;
  double aTerm = vdw1 * R_star_ij / (dist + vdw1m1 * R_star_ij);
  double aTerm2 = aTerm * aTerm;
  double aTerm7 = aTerm2 * aTerm2 * aTerm2 * aTerm;
  double R_star_ij2 = R_star_ij * R_star_ij;
  double R_star_ij7 = R_star_ij2 * R_star_ij2 * R_star_ij2 * R_star_ij;
  double bTerm = vdw2 * R_star_ij7 / (dist7 + vdw2m1 * R_star_ij7) - 2.0;
  double res = wellDepth * aTerm7 * bTerm;
#if 0
  std::cerr << "ForceFields::MMFF::calcVdWEnergy() "
    << "dist2 = " << dist2 << ", res = " << res * 4.184 << std::endl;
#endif

  return res;
}

void scaleVdWParams(double &R_star_ij, double &wellDepth,
                    MMFFVdWCollection *mmffVdW,
                    const MMFFVdW *mmffVdWParamsIAtom,
                    const MMFFVdW *mmffVdWParamsJAtom) {
  if (((mmffVdWParamsIAtom->DA == 'D') && (mmffVdWParamsJAtom->DA == 'A')) ||
      ((mmffVdWParamsIAtom->DA == 'A') && (mmffVdWParamsJAtom->DA == 'D'))) {
#if 0
    std::cerr << "RDKit::scaleVdWParams() scaling" << std::endl;
#endif
    R_star_ij *= mmffVdW->DARAD;
    wellDepth *= mmffVdW->DAEPS;
  }
}

double calcEleEnergy(unsigned int idx1, unsigned int idx2, double dist,
                     double chargeTerm, boost::uint8_t dielModel, bool is1_4) {
  RDUNUSED_PARAM(idx1);
  RDUNUSED_PARAM(idx2);
  static const double diel = 332.0716;
  static const double sc1_4 = 0.75;
  static const double delta = 0.05;
  double corr_dist = dist + delta;
  if (dielModel == RDKit::MMFF::DISTANCE) {
    corr_dist *= corr_dist;
  }
  return (diel * chargeTerm / corr_dist * (is1_4 ? sc1_4 : 1.0));
}

#ifdef RDK_BUILD_WITH_OPENMM
OpenMM::MMFFVdwForce *getOpenMMVdWForce() {
  return new OpenMM::MMFFVdwForce();
}

OpenMM::CustomNonbondedForce *getOpenMMEleForce(boost::uint8_t dielModel, double dielConst, bool is1_4) {
  static const double sc1_4 = 0.75;
  static const double cOMM = 332.0716 * OpenMM::KJPerKcal;
  static const double delta = 0.05;
  // we need to use different global const names for the two ele forces or OpenMM gets confused
  const std::string cName = (is1_4 ? "cDamp" : "c");
  const std::string ef = cName + "*q1*q2/(D*(ntoa*r+delta)^n)";
  OpenMM::CustomNonbondedForce *res = new OpenMM::CustomNonbondedForce(ef);
  res->addGlobalParameter(cName, cOMM * (is1_4 ? sc1_4 : 1.0));
  res->addGlobalParameter("D", dielConst);
  res->addGlobalParameter("n", ((dielModel == RDKit::MMFF::DISTANCE) ? 2.0 : 1.0));
  res->addGlobalParameter("ntoa", OpenMM::AngstromsPerNm);
  res->addGlobalParameter("delta", delta);
  res->addPerParticleParameter("q");
  
  return res;
}
#endif
}  // end of namespace utils

VdWContrib::VdWContrib(ForceField *owner, unsigned int idx1, unsigned int idx2,
                       const MMFFVdWRijstarEps *mmffVdWConstants) {
  PRECONDITION(owner, "bad owner");
  PRECONDITION(mmffVdWConstants, "bad MMFFVdW parameters");
  URANGE_CHECK(idx1, owner->positions().size() - 1);
  URANGE_CHECK(idx2, owner->positions().size() - 1);

  dp_forceField = owner;
  d_at1Idx = idx1;
  d_at2Idx = idx2;
  d_R_ij_star = mmffVdWConstants->R_ij_star;
  d_wellDepth = mmffVdWConstants->epsilon;
}

double VdWContrib::getEnergy(double *pos) const {
  PRECONDITION(dp_forceField, "no owner");
  PRECONDITION(pos, "bad vector");

  double dist = dp_forceField->distance(d_at1Idx, d_at2Idx, pos);
  return Utils::calcVdWEnergy(dist, d_R_ij_star, d_wellDepth);
}

void VdWContrib::getGrad(double *pos, double *grad) const {
  PRECONDITION(dp_forceField, "no owner");
  PRECONDITION(pos, "bad vector");
  PRECONDITION(grad, "bad vector");

  double const vdw1 = 1.07;
  double const vdw1m1 = vdw1 - 1.0;
  double const vdw2 = 1.12;
  double const vdw2m1 = vdw2 - 1.0;
  double const vdw2t7 = vdw2 * 7.0;
  double dist = dp_forceField->distance(d_at1Idx, d_at2Idx, pos);
  double *at1Coords = &(pos[3 * d_at1Idx]);
  double *at2Coords = &(pos[3 * d_at2Idx]);
  double *g1 = &(grad[3 * d_at1Idx]);
  double *g2 = &(grad[3 * d_at2Idx]);
  double q = dist / d_R_ij_star;
  double q2 = q * q;
  double q6 = q2 * q2 * q2;
  double q7 = q6 * q;
  double q7pvdw2m1 = q7 + vdw2m1;
  double t = vdw1 / (q + vdw1 - 1.0);
  double t2 = t * t;
  double t7 = t2 * t2 * t2 * t;
  double dE_dr = d_wellDepth / d_R_ij_star * t7 *
                 (-vdw2t7 * q6 / (q7pvdw2m1 * q7pvdw2m1) +
                  ((-vdw2t7 / q7pvdw2m1 + 14.0) / (q + vdw1m1)));
  for (unsigned int i = 0; i < 3; ++i) {
    double dGrad;
    dGrad = ((dist > 0.0) ? (dE_dr * (at1Coords[i] - at2Coords[i]) / dist)
                          : d_R_ij_star * 0.01);
    g1[i] += dGrad;
    g2[i] -= dGrad;
  }
}

EleContrib::EleContrib(ForceField *owner, unsigned int idx1, unsigned int idx2,
                       double chargeTerm, boost::uint8_t dielModel,
                       bool is1_4) {
  PRECONDITION(owner, "bad owner");
  URANGE_CHECK(idx1, owner->positions().size() - 1);
  URANGE_CHECK(idx2, owner->positions().size() - 1);

  dp_forceField = owner;
  d_at1Idx = idx1;
  d_at2Idx = idx2;
  d_chargeTerm = chargeTerm;
  d_dielModel = dielModel;
  d_is1_4 = is1_4;
}

double EleContrib::getEnergy(double *pos) const {
  PRECONDITION(dp_forceField, "no owner");
  PRECONDITION(pos, "bad vector");

  return Utils::calcEleEnergy(d_at1Idx, d_at2Idx,
                              dp_forceField->distance(d_at1Idx, d_at2Idx, pos),
                              d_chargeTerm, d_dielModel, d_is1_4);
}

void EleContrib::getGrad(double *pos, double *grad) const {
  PRECONDITION(dp_forceField, "no owner");
  PRECONDITION(pos, "bad vector");
  PRECONDITION(grad, "bad vector");

  double dist = dp_forceField->distance(d_at1Idx, d_at2Idx, pos);
  double *at1Coords = &(pos[3 * d_at1Idx]);
  double *at2Coords = &(pos[3 * d_at2Idx]);
  double *g1 = &(grad[3 * d_at1Idx]);
  double *g2 = &(grad[3 * d_at2Idx]);
  double corr_dist = dist + 0.05;
  corr_dist *= ((d_dielModel == RDKit::MMFF::DISTANCE) ? corr_dist * corr_dist
                                                       : corr_dist);
  double dE_dr = -332.0716 * (double)(d_dielModel)*d_chargeTerm / corr_dist *
                 (d_is1_4 ? 0.75 : 1.0);
  for (unsigned int i = 0; i < 3; ++i) {
    double dGrad;
    dGrad =
        ((dist > 0.0) ? (dE_dr * (at1Coords[i] - at2Coords[i]) / dist) : 0.02);
    g1[i] += dGrad;
    g2[i] -= dGrad;
  }
}
}
}
